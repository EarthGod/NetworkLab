

tcp1 :: tcpEntity(MY_PORT_NUM 111, TYPE 1, TIME_OUT 1000, MY_IP_ADDR 1.2.3.4); //server
tcp2 :: tcpEntity(MY_PORT_NUM 222, TYPE 0, TIME_OUT 1000, MY_IP_ADDR 1.2.3.5); //client

ip1 :: ip(IP_ADDR 1.2.3.4);
ip2 :: ip(IP_ADDR 1.2.3.5);

cl1 :: PacketGenerator(222, 1.2.3.5);

cl1->[1]tcp1[0]->Print("after tcp")->[0]ip1[1]->Print("after ip")->[1]ip2[0]->[0]tcp2[1]->Print(1)->Discard();
Idle()->[1]tcp2[0]->[0]ip2[1]->[1]ip1[0]->[0]tcp1[1]->Print(2)->Discard();
