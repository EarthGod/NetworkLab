require(library /home/comnetsii/elements/routerport.click);

rp1 :: RouterPort(DEV veth1, IN_MAC e2:4f:a6:98:ad:50 , OUT_MAC 5e:0b:6c:58:a2:0f );

rp2 :: RouterPort(DEV veth2, IN_MAC 5e:0b:6c:58:a2:0f , OUT_MAC e2:4f:a6:98:ad:50 );

rp3 :: RouterPort(DEV veth3, IN_MAC be:36:a3:21:3e:e8 , OUT_MAC c6:28:3f:f3:aa:a0 );

rp4 :: RouterPort(DEV veth4, IN_MAC c6:28:3f:f3:aa:a0 , OUT_MAC be:36:a3:21:3e:e8 );

tcp1 :: tcpEntity(MY_PORT_NUM 80, TYPE 1, TIME_OUT 1000, MY_IP_ADDR 1.2.3.4); //server
tcp2 :: tcpEntity(MY_PORT_NUM 80, TYPE 0, TIME_OUT 1000, MY_IP_ADDR 2.3.4.5); //client

ip1 :: ip(IP_ADDR 1.2.3.4);
ip2 :: ip(IP_ADDR 2.3.4.5);
ip3 :: ip(IP_ADDR 5.6.7.8);

cl1 :: PacketGenerator(80, 1.2.3.4);

cl1->[1]tcp1[0]->[0]ip1[1]->rp1;
rp1->[1]ip3;
ip3[1]->rp3;
rp3->[1]ip2[0]->[0]tcp2[1]->Discard();

Idle()->[1]tcp2[0]->[0]ip2[1]->rp4;
rp4->[2]ip3;
ip3[2]->rp2;
rp2->[1]ip1[0]->[0]tcp1[1]->Discard();

Idle()->[0]ip3[0]->Discard();
