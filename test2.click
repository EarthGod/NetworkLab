require(library /home/comnetsii/elements/routerport.click);

rp2 :: RouterPort(DEV veth2, IN_MAC 5e:0b:6c:58:a2:0f , OUT_MAC e2:4f:a6:98:ad:50 );

rp4 :: RouterPort(DEV veth4, IN_MAC c6:28:3f:f3:aa:a0 , OUT_MAC be:36:a3:21:3e:e8 );

tcp1 :: tcpEntity(MY_PORT_NUM 80, TYPE 1, TIME_OUT 1000, MY_IP_ADDR 1.2.3.4); //server
tcp2 :: tcpEntity(MY_PORT_NUM 80, TYPE 0, TIME_OUT 1000, MY_IP_ADDR 4.3.2.1); //client

ip1 :: ip(IP_ADDR 1.2.3.4);
ip2 :: ip(IP_ADDR 5.6.7.8);
ip3 :: ip(IP_ADDR 4.3.2.1);

cl2 :: PacketGenerator(80, 4.3.2.1);

cl2->[1]tcp2[0]->[0]ip2[1]->rp4;
rp4->[2]ip3;
ip3[2]->rp2;
rp2->[1]ip1[0]->[0]tcp1[1]->Print()->Discard();