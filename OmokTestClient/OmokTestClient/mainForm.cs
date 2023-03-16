using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace OmokTestClient
{
    public partial class mainForm : Form
    {
        ClientSimpleTcp Network = new ClientSimpleTcp();

        bool IsNetworkThreadRunning = false;
        bool IsBackGroundProcessRunning = false;

        System.Threading.Thread NetworkReadThread = null;
        System.Threading.Thread NetworkSendThread = null;

        PacketBufferManager PacketBuffer = new PacketBufferManager();
        Queue<PacketData> RecvPacketQueue = new Queue<PacketData>();
        Queue<byte[]> SendPacketQueue = new Queue<byte[]>();

        Timer dispatcherUITimer;

        // Omok
        int margin = 20;
        int gridSize = 30;
        int stoneSize = 28;
        int flowerSize = 10;

        Graphics g;
        Pen pen;
        Brush wBrush, bBrush;

        enum STONE { none, black, white };
        STONE[,] goBoard = new STONE[19, 19];
        bool flag = false;  // false = 검은 돌, true = 흰돌

        public mainForm()
        {
            InitializeComponent();

            this.Text = "오목 클라이언트";
            //this.BackColor = Color.Orange;

            pen = new Pen(Color.Black);
            bBrush = new SolidBrush(Color.Black);
            wBrush = new SolidBrush(Color.White);

        }

        private void mainForm_Load(object sender, EventArgs e)
        {
            PacketBuffer.Init((8096 * 10), PacketDef.PACKET_HEADER_SIZE, 1024);

            IsNetworkThreadRunning = true;
            NetworkReadThread = new System.Threading.Thread(this.NetworkReadProcess);
            NetworkReadThread.Start();
            NetworkSendThread = new System.Threading.Thread(this.NetworkSendProcess);
            NetworkSendThread.Start();

            IsBackGroundProcessRunning = true;
            dispatcherUITimer = new Timer();
            dispatcherUITimer.Tick += new EventHandler(BackGroundProcess);
            dispatcherUITimer.Interval = 100;
            dispatcherUITimer.Start();

            btnDisconnect.Enabled = false;

            SetPacketHandler();
            DevLog.Write("프로그램 시작 !!!", LOG_LEVEL.INFO);
        }

        private void mainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            IsNetworkThreadRunning = false;
            IsBackGroundProcessRunning = false;

            Network.Close();
        }

        private void btnConnect_Click(object sender, EventArgs e)
        {
            string address = textBoxIP.Text;

            if (checkBoxLocalHostIP.Checked)
            {
                address = "127.0.0.1";
            }

            int port = Convert.ToInt32(textBoxPort.Text);

            if (Network.Connect(address, port))
            {
                labelStatus.Text = string.Format("{0}. 서버에 접속 중", DateTime.Now);
                btnConnect.Enabled = false;
                btnDisconnect.Enabled = true;

                DevLog.Write($"서버에 접속 중", LOG_LEVEL.INFO);
            }
            else
            {
                labelStatus.Text = string.Format("{0}. 서버에 접속 실패", DateTime.Now);
            }
        }

        private void btnDisconnect_Click(object sender, EventArgs e)
        {
            SetDisconnectd();
            Network.Close();
        }

        void NetworkReadProcess()
        {
            const Int16 PacketHeaderSize = PacketDef.PACKET_HEADER_SIZE;

            while (IsNetworkThreadRunning)
            {
                if (Network.IsConnected() == false)
                {
                    System.Threading.Thread.Sleep(1);
                    continue;
                }

                var recvData = Network.Receive();

                if (recvData != null)
                {
                    PacketBuffer.Write(recvData.Item2, 0, recvData.Item1);

                    while (true)
                    {
                        var data = PacketBuffer.Read();
                        if (data.Count < 1)
                        {
                            break;
                        }

                        var packet = new PacketData();
                        packet.DataSize = (short)(data.Count - PacketHeaderSize);
                        packet.PacketID = BitConverter.ToInt16(data.Array, data.Offset + 2);
                        packet.Type = (SByte)data.Array[(data.Offset + 4)];
                        packet.BodyData = new byte[packet.DataSize];
                        Buffer.BlockCopy(data.Array, (data.Offset + PacketHeaderSize), packet.BodyData, 0, (data.Count - PacketHeaderSize));
                        lock (((System.Collections.ICollection)RecvPacketQueue).SyncRoot)
                        {
                            RecvPacketQueue.Enqueue(packet);
                        }
                    }
                    //DevLog.Write($"받은 데이터: {recvData.Item2}", LOG_LEVEL.INFO);
                }
                else
                {
                    Network.Close();
                    SetDisconnectd();
                    DevLog.Write("서버와 접속 종료 !!!", LOG_LEVEL.INFO);
                }
            }
        }

        void NetworkSendProcess()
        {
            while (IsNetworkThreadRunning)
            {
                System.Threading.Thread.Sleep(1);

                if (Network.IsConnected() == false)
                {
                    continue;
                }

                lock (((System.Collections.ICollection)SendPacketQueue).SyncRoot)
                {
                    if (SendPacketQueue.Count > 0)
                    {
                        var packet = SendPacketQueue.Dequeue();
                        Network.Send(packet);
                    }
                }
            }
        }


        void BackGroundProcess(object sender, EventArgs e)
        {
            ProcessLog();

            try
            {
                var packet = new PacketData();

                lock (((System.Collections.ICollection)RecvPacketQueue).SyncRoot)
                {
                    if (RecvPacketQueue.Count() > 0)
                    {
                        packet = RecvPacketQueue.Dequeue();
                    }
                }

                if (packet.PacketID != 0)
                {
                    PacketProcess(packet);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(string.Format("ReadPacketQueueProcess. error:{0}", ex.Message));
            }
        }

        private void ProcessLog()
        {
            // 너무 이 작업만 할 수 없으므로 일정 작업 이상을 하면 일단 패스한다.
            int logWorkCount = 0;

            while (IsBackGroundProcessRunning)
            {
                System.Threading.Thread.Sleep(1);

                string msg;

                if (DevLog.GetLog(out msg))
                {
                    ++logWorkCount;

                    if (listBoxLog.Items.Count > 512)
                    {
                        listBoxLog.Items.Clear();
                    }

                    listBoxLog.Items.Add(msg);
                    listBoxLog.SelectedIndex = listBoxLog.Items.Count - 1;
                }
                else
                {
                    break;
                }

                if (logWorkCount > 8)
                {
                    break;
                }
            }
        }


        public void SetDisconnectd()
        {
            if (btnConnect.Enabled == false)
            {
                btnConnect.Enabled = true;
                btnDisconnect.Enabled = false;
            }

            SendPacketQueue.Clear();

            listBoxRoomChatMsg.Items.Clear();
            listBoxRoomUserList.Items.Clear();

            labelStatus.Text = "서버 접속이 끊어짐";
        }

        public void PostSendPacket(PACKET_ID packetID, byte[] bodyData)
        {
            if (Network.IsConnected() == false)
            {
                DevLog.Write("서버 연결이 되어 있지 않습니다", LOG_LEVEL.ERROR);
                return;
            }

            Int16 bodyDataSize = 0;
            if (bodyData != null)
            {
                bodyDataSize = (Int16)bodyData.Length;
            }
            var packetSize = bodyDataSize + PacketDef.PACKET_HEADER_SIZE;

            List<byte> dataSource = new List<byte>();
            dataSource.AddRange(BitConverter.GetBytes((Int16)packetSize));
            dataSource.AddRange(BitConverter.GetBytes((Int16)packetID));
            dataSource.AddRange(new byte[] { (byte)0 });

            if (bodyData != null)
            {
                dataSource.AddRange(bodyData);
            }

            SendPacketQueue.Enqueue(dataSource.ToArray());
        }

        void RoomUserListClear()
        {
            listBoxRoomUserList.Items.Clear();
        }

        void AddRoomUserList(string userID)
        {
            var msg = $"{userID}";
            listBoxRoomUserList.Items.Add(msg);
        }

        void RemoveRoomUserList(string userID)
        {
            object removeItem = null;

            foreach (var user in listBoxRoomUserList.Items)
            {
                if (user.ToString() == userID)
                {
                    removeItem = user;
                    break;
                }
            }

            if (removeItem != null)
            {
                listBoxRoomUserList.Items.Remove(removeItem);
            }
        }


        // 로그인 요청
        private void button2_Click(object sender, EventArgs e)
        {
            var loginReq = new LoginReqPacket();
            loginReq.SetValue(textBoxUserID.Text, textBoxUserPW.Text);

            PostSendPacket(PACKET_ID.LOGIN_REQ, loginReq.ToBytes());
            DevLog.Write($"로그인 요청:  {textBoxUserID.Text}, {textBoxUserPW.Text}");
        }

        private void btn_RoomEnter_Click(object sender, EventArgs e)
        {
            var requestPkt = new RoomEnterReqPacket();
            requestPkt.SetValue(Int32.Parse(textBoxRoomNumber.Text));

            PostSendPacket(PACKET_ID.ROOM_ENTER_REQ, requestPkt.ToBytes());
            DevLog.Write($"방 입장 요청:  {textBoxRoomNumber.Text} 번");
        }

        private void btn_RoomLeave_Click(object sender, EventArgs e)
        {
            PostSendPacket(PACKET_ID.ROOM_LEAVE_REQ, null);
            DevLog.Write($"방 입장 요청:  {textBoxRoomNumber.Text} 번");
        }

        private void btnRoomChat_Click(object sender, EventArgs e)
        {
            if (String.IsNullOrEmpty(textBoxRoomSendMsg.Text))
            {
                MessageBox.Show("채팅 메시지를 입력하세요");
                return;
            }

            var requestPkt = new RoomChatReqPacket();
            requestPkt.SetValue(textBoxRoomSendMsg.Text);

            PostSendPacket(PACKET_ID.ROOM_CHAT_REQ, requestPkt.ToBytes());
            DevLog.Write($"방 채팅 요청");
        }

        private void btnRoomRelay_Click(object sender, EventArgs e)
        {
            //if( textBoxRelay.Text.IsEmpty())
            //{
            //    MessageBox.Show("릴레이 할 데이터가 없습니다");
            //    return;
            //}

            //var bodyData = Encoding.UTF8.GetBytes(textBoxRelay.Text);
            //PostSendPacket(PACKET_ID.PACKET_ID_ROOM_RELAY_REQ, bodyData);
            //DevLog.Write($"방 릴레이 요청");
        }

        private void btn_Ready_Click(object sender, EventArgs e)
        {
            PostSendPacket(PACKET_ID.PK_READY_GAME_ROOM_REQ, null);
            DevLog.Write($"게임 준비 요청:  {textBoxRoomNumber.Text} 번");
        }

        private void btn_CancelReady_Click(object sender, EventArgs e)
        {
            PostSendPacket(PACKET_ID.PK_CANCEL_READY_GAME_ROOM_REQ, null);
            DevLog.Write($"게임 준비 취소 요청:  {textBoxRoomNumber.Text} 번");
        }

        private void panel1_Paint(object sender, PaintEventArgs e)
        {
            panel1.Size = new Size(2 * margin + 18 * gridSize, 2 * margin + 18 * gridSize);
            panel1.BackColor = Color.Orange;
            DrawBoard();
            DrawStone();
        }

        private void panel1_MouseDown(object sender, MouseEventArgs e)
        {
            var requestPkt = new PutALGameRoomReqPacket();

            // e.X는 픽셀단위, x는 바둑판 좌표
            int x = (e.X - margin + gridSize / 2) / gridSize;
            int y = (e.Y - margin + gridSize / 2) / gridSize;

            if (goBoard[x, y] != STONE.none)
                return;

            requestPkt.SetValue((short)x, (short)y);

            PostSendPacket(PACKET_ID.PK_PUT_AL_ROOM_REQ, requestPkt.ToBytes());
            DevLog.Write($"바둑돌 놓기 요청");


            /*
            // 바둑판[x,y] 에 돌을 그린다
            Rectangle r = new Rectangle(
              margin + gridSize * x - stoneSize / 2,
              margin + gridSize * y - stoneSize / 2,
              stoneSize, stoneSize);

            // 검은돌 차례
            if (flag == false)
            {
                //g.FillEllipse(bBrush, r);
                Bitmap bmp = new Bitmap("../../Images/black.png");
                g.DrawImage(bmp, r);
                flag = true;
                goBoard[x, y] = STONE.black;
            }
            else
            {
                //g.FillEllipse(wBrush, r);
                Bitmap bmp = new Bitmap("../../Images/white.png");
                g.DrawImage(bmp, r);
                flag = false;
                goBoard[x, y] = STONE.white;
            }

            checkOmok(x, y);
            */
        }

        public void PutAL(short x, short y, short stone)
        {
            Rectangle r = new Rectangle(
              margin + gridSize * x - stoneSize / 2,
              margin + gridSize * y - stoneSize / 2,
              stoneSize, stoneSize);

            // 검은돌 차례
            if (stone == 1)
            {
                //g.FillEllipse(bBrush, r);
                Bitmap bmp = new Bitmap("../../Images/black.png");
                g.DrawImage(bmp, r);
                goBoard[x, y] = STONE.black;
            }
            else if(stone == 2)
            {
                //g.FillEllipse(wBrush, r);
                Bitmap bmp = new Bitmap("../../Images/white.png");
                g.DrawImage(bmp, r);
                goBoard[x, y] = STONE.white;
            }
        }

        private void DrawBoard()
        {
            // panel1에 Graphics 객체 생성
            g = panel1.CreateGraphics();

            // 세로선 19개
            for (int i = 0; i < 19; i++)
            {
                g.DrawLine(pen, new Point(margin + i * gridSize, margin),
                  new Point(margin + i * gridSize, margin + 18 * gridSize));
            }

            // 가로선 19개
            for (int i = 0; i < 19; i++)
            {
                g.DrawLine(pen, new Point(margin, margin + i * gridSize),
                  new Point(margin + 18 * gridSize, margin + i * gridSize));
            }

            // 화점그리기
            for (int x = 3; x <= 15; x += 6)
                for (int y = 3; y <= 15; y += 6)
                {
                    g.FillEllipse(bBrush,
                      margin + gridSize * x - flowerSize / 2,
                      margin + gridSize * y - flowerSize / 2,
                      flowerSize, flowerSize);
                }
        }

        // 자료구조에서 바둑돌의 값을 읽어서 다시 그려준다
        private void DrawStone()
        {
            for (int x = 0; x < 19; x++)
            {
                for (int y = 0; y < 19; y++)
                {
                    if (goBoard[x, y] == STONE.white)
                    {
                        Bitmap bmp = new Bitmap("../../Images/white.png");
                        g.DrawImage(bmp, margin + x * gridSize - stoneSize / 2,
                            margin + y * gridSize - stoneSize / 2, stoneSize, stoneSize);
                    }
                    else if (goBoard[x, y] == STONE.black)
                    {
                        Bitmap bmp = new Bitmap("../../Images/black.png");
                        g.DrawImage(bmp, margin + x * gridSize - stoneSize / 2,
                            margin + y * gridSize - stoneSize / 2, stoneSize, stoneSize);
                    }
                }
            }
        }

        public void EndGame(string win_userID)
        {
            DialogResult res = MessageBox.Show(win_userID + "님이 이겼습니다.", "게임 종료", MessageBoxButtons.OK);

            flag = false;

            for (int x = 0; x < 19; x++)
                for (int y = 0; y < 19; y++)
                    goBoard[x, y] = STONE.none;

            panel1.Refresh();
            DrawBoard();
            DrawStone();
        }
    }
}
