require(library /home/comnetsii/elements/routerport.click);

rp2 :: RouterPort(DEV veth2, IN_MAC 5e:0b:6c:58:a2:0f , OUT_MAC e2:4f:a6:98:ad:50 );

rp4 :: RouterPort(DEV veth4, IN_MAC c6:28:3f:f3:aa:a0 , OUT_MAC be:36:a3:21:3e:e8 );

tcp1 :: tcpEntity(MY_PORT_NUM 111, TYPE 1, TIME_OUT 1000, MY_IP_ADDR 1.2.3.4); //server
tcp2 :: tcpEntity(MY_PORT_NUM 222, TYPE 0, TIME_OUT 1000, MY_IP_ADDR 1.2.3.5); //client



cl1 :: PacketGenerator(222, 4.3.2.1);

cl2->[1]tcp1[0]->[0]tcp2[1]->Print()->Discard();