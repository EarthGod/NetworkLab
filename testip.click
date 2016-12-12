require(library /home/comnetsii/elements/routerport.click);

rp1 :: RouterPort(DEV veth1, IN_MAC e2:4f:a6:98:ad:50 , OUT_MAC 5e:0b:6c:58:a2:0f );

rp2 :: RouterPort(DEV veth2, IN_MAC 5e:0b:6c:58:a2:0f , OUT_MAC e2:4f:a6:98:ad:50 );

ip1 :: ip(IP_ADDR 1.2.3.4);
ip2 :: ip(IP_ADDR 2.3.4.5);
ip3 :: ip(IP_ADDR 5.6.7.8);

cl1 :: simipgen(1.2.3.4, 2.3.4.5);

cl1->Print("SENDING")->[0]ip1[1]->Print("before rp1")->rp1->Print("after rp1")->[1]ip3[2]->[2]ip2[0]->Print("GOTCHA")->Discard();

Idle()->[0]ip2[2]->[2]ip3[1]->Print("before rp2")->rp2->Print("after rp2")->[1]ip1[0]->Discard();

Idle()->[0]ip3[0]->Discard();
Idle()->[1]ip2[1]->Discard();