#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("Homework");

void tracer_CWnd(uint32_t x_old, uint32_t x_new) {
    cout << "TCP_Cwnd" << "," << Simulator::Now().GetNanoSeconds() << "," << x_new << endl;
}
static const uint32_t totalTxBytes = 2000000;
static uint32_t currentTxBytes = 0;
static const uint32_t writeSize = 1040;
uint8_t datap[writeSize];
void WriteUntilBufferFull (Ptr<Socket> localSocket, uint32_t txSpace) {
	while (currentTxBytes < totalTxBytes && localSocket->GetTxAvailable () > 0) {
		uint32_t left = totalTxBytes - currentTxBytes;
		uint32_t dataOffset = currentTxBytes % writeSize;
		uint32_t toWrite = writeSize - dataOffset;
		toWrite = std::min (toWrite, left);
		toWrite = std::min (toWrite, localSocket->GetTxAvailable ());
		int amountSent = localSocket->Send (&datap[dataOffset], toWrite, 0);
		if(amountSent < 0) {
			return;
		}
		currentTxBytes += amountSent;
	}

	if (currentTxBytes >= totalTxBytes) {
		localSocket->Close ();
	}
}
void StartFlow (Ptr<Socket> localSocket, Ipv4Address servAddress, uint16_t servPort) {
	localSocket->Connect (InetSocketAddress (servAddress, servPort));
	localSocket->SetSendCallback (MakeCallback (&WriteUntilBufferFull));
	WriteUntilBufferFull (localSocket, localSocket->GetTxAvailable ());
}

int main(int argc, char *argv[]) 
{
  CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv);
  LogComponentEnable("Homework", LOG_LEVEL_INFO);

  // 创建节点
  NodeContainer green, nodes, blue, purple, gray;
  // Ptr<Node> gray1 = CreateObject<Node>();
  green.Create(1);
  nodes.Create(4);
  blue.Create(1);
  purple.Create(1);
  gray.Create(1);

  // 创建点对点链接
  PointToPointHelper p2pHelper;
  p2pHelper.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  p2pHelper.SetChannelAttribute("Delay", StringValue("5ms"));

  //green to blue
  NetDeviceContainer greenToR1 = p2pHelper.Install(green.Get(0), nodes.Get(0));
  NetDeviceContainer r1ToR2 = p2pHelper.Install(nodes.Get(0), nodes.Get(1));
  NetDeviceContainer r2ToR3 = p2pHelper.Install(nodes.Get(1), nodes.Get(2));
  NetDeviceContainer r3ToR4 = p2pHelper.Install(nodes.Get(2), nodes.Get(3));
  NetDeviceContainer r4ToBlue = p2pHelper.Install(nodes.Get(3), blue.Get(0));
  // purple to gray
  NetDeviceContainer purpleToR2 = p2pHelper.Install(purple.Get(0), nodes.Get(1));
  NetDeviceContainer r3ToGray = p2pHelper.Install(nodes.Get(2), gray.Get(0));


  InternetStackHelper stack;
  stack.Install(green);
  stack.Install(nodes);
  stack.Install(blue);
  stack.Install(purple);
  stack.Install(gray);

  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer greenToR1_intf = address.Assign(greenToR1);
  address.SetBase("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer r1ToR2_intf = address.Assign(r1ToR2);
  address.SetBase("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer r2ToR3_intf = address.Assign(r2ToR3);
  address.SetBase("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer r3ToR4_intf = address.Assign(r3ToR4);
  address.SetBase("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer r4ToBlue_intf = address.Assign(r4ToBlue);

  address.SetBase("10.1.6.0", "255.255.255.0");
  Ipv4InterfaceContainer purpleToR2_intf = address.Assign(purpleToR2);
  address.SetBase("10.1.7.0", "255.255.255.0");
  Ipv4InterfaceContainer r3ToGray_intf = address.Assign(r3ToGray);

  // 配置路由
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  // 设置TCP协议栈的拥塞控制算法为NewReno
  Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpNewReno::GetTypeId()));

  //创建 绿-蓝 的TCP连接
  // uint16_t port = 8080;
  // Ptr<Socket> localSocket = Socket::CreateSocket (green.Get(0), TcpSocketFactory::GetTypeId ());
  // localSocket->Bind ();

  // Simulator::ScheduleNow (&StartFlow, localSocket, r4ToBlue_intf.GetAddress(1), port);

  // PacketSinkHelper sink ("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny(), port));
  // ApplicationContainer sinkApps = sink.Install (blue.Get(0));
  // sinkApps.Start (Seconds (0.0));
  // sinkApps.Stop (Seconds (10.0));

  // Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&tracer_CWnd));



  //创建TCP服务器
  uint16_t tcpPort = 8080;
  Address sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), tcpPort));  
  PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApp = sinkHelper.Install(blue.Get(0));
  sinkApp.Start(Seconds(0.0));
  sinkApp.Stop(Seconds(10.0));

  // Ptr<Socket> socket = Socket::CreateSocket(nodes.Get(0), TcpSocketFactory::GetTypeId());
  // // 设置拥塞窗口跟踪器
  // socket->TraceConnectWithoutContext("CongestionWindow", MakeCallback(&tracer_CWnd));
  // cout << r1ToR2_intf.GetAddress(1) << "*****************************" << InetSocketAddress("10.1.2.2", tcpPort) << endl;
  // socket->Connect(InetSocketAddress("10.1.2.2", tcpPort));

  // // 创建TCP客户端
  // Ptr<Socket> clientSocket = Socket::CreateSocket(green.Get(0), TcpSocketFactory::GetTypeId());
  // // clientSocket->Bind();
  // clientSocket->Connect(InetSocketAddress(r4ToBlue_intf.GetAddress(1), tcpPort));
  // clientSocket->Connect(sinkLocalAddress);

  // 创建TCP数据流
  uint32_t packetSize = 1024;
  // Time interPacketInterval = Seconds(1.0);
  OnOffHelper onoff("ns3::TcpSocketFactory", InetSocketAddress (r4ToBlue_intf.GetAddress(1), tcpPort));
  onoff.SetAttribute("PacketSize", UintegerValue(packetSize));
  onoff.SetAttribute("DataRate", StringValue("1Mbps"));
  onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
  onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
  // AddressValue remote (InetSocketAddress (r4ToBlue_intf.GetAddress(1), tcpPort));
  // onoff.SetAttribute ("Remote", remote);
  ApplicationContainer clientApps = onoff.Install(green.Get(0));
  clientApps.Start(Seconds(0.0));
  clientApps.Stop(Seconds(10.0));


  // 创建UDP服务器
  uint16_t udpServerPort = 50000;
  UdpEchoServerHelper udpServer(udpServerPort);
  ApplicationContainer udpServerApps = udpServer.Install(gray.Get(0));
  udpServerApps.Start(Seconds(0.0));
  udpServerApps.Stop(Seconds(10.0));
  // 创建UDP客户端
  UdpEchoClientHelper udpClient( r3ToGray_intf.GetAddress(1), udpServerPort);
  udpClient.SetAttribute("MaxPackets", UintegerValue(500)); // 设置要发送的UDP数据包总数量
  udpClient.SetAttribute("Interval", TimeValue(Seconds(0.01))); // 设置发送间隔
  udpClient.SetAttribute("PacketSize", UintegerValue(1024)); // 设置单个数据包的大小
  ApplicationContainer udpClientApps = udpClient.Install(purple.Get(0));
  udpClientApps.Start(Seconds(0.0));
  udpClientApps.Stop(Seconds(10.0));



	// Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&tracer_CWnd));
  p2pHelper.EnablePcapAll("homework", true);
  // 启动仿真
  NS_LOG_INFO("Starting simulation");
  Simulator::Run();
  Simulator::Destroy();
  NS_LOG_INFO("Simulation completed");

  return 0;
}
