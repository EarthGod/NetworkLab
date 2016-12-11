require(library /home/comnetsii/elements/routerport.click);

rp1 :: RouterPort(DEV veth1, IN_MAC e2:4f:a6:98:ad:50 , OUT_MAC 5e:0b:6c:58:a2:0f );


cl1 :: PacketGenerator(80, 1.2.3.4);

cl1->rp1->Print()->Discard();
Idle->Print()->Discard();