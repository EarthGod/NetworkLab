require(library /home/comnetsii/elements/routerport.click);

rp1 :: RouterPort(DEV $dev1, IN_MAC $in_mac1 , OUT_MAC $out_mac1 );

rp2 :: RouterPort(DEV $dev2, IN_MAC $in_mac2 , OUT_MAC $out_mac2 );

rp3 :: RouterPort(DEV $dev3, IN_MAC $in_mac3 , OUT_MAC $out_mac3 );

rp4 :: RouterPort(DEV $dev4, IN_MAC $in_mac4 , OUT_MAC $out_mac4 );

tcp1 :: tcpEntity(MY_PORT_NUM $port_num1, TYPE 1, TIME_OUT $timeout1, MY_IP_ADDR $ipaddr1); //server
tcp2 :: tcpEntity(MY_PORT_NUM $port_num2, TYPE 0, TIME_OUT $timeout2, MY_IP_ADDR $ipaddr2); //client

ip1 :: ip(IP_ADDR $ipaddr1);
ip2 :: ip(IP_ADDR $ipaddr2);
ip3 :: ip(IP_ADDR $ipaddr3);

cl1 :: PacketGenerator();

cl1->[1]tcp1[0]->[0]ip1[1]->rp1;
rp2->[1]ip3[1]->rp3;
rp4->[1]ip2[0]->[0]tcp2[1]->Print();
