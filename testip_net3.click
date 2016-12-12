require(library /home/comnetsii/elements/routerport.click);

ip1 :: ip(IP_ADDR 1.1.1.1);
ip2 :: ip(IP_ADDR 2.2.2.2);
ip3 :: ip(IP_ADDR 3.3.3.3);
ip4 :: ip(IP_ADDR 4.4.4.4);
ip5 :: ip(IP_ADDR 5.5.5.5);
ip6 :: ip(IP_ADDR 6.6.6.6);

cl1 :: simipgen(1.1.1.1, 2.2.2.2);
cl2 :: simipgen(2.2.2.2, 3.3.3.3);
cl3 :: simipgen(3.3.3.3, 1.1.1.1);

cl1->[0]ip1[1]->[1]ip4;
ip1[0]->Print("1.1.1.1 GET:")->Discard();
ip4[1]->[1]ip1;
cl2->[0]ip2[1]->[1]ip5;
ip2[0]->Print("2.2.2.2 GET:")->Discard();
ip5[1]->[1]ip2;
cl3->[0]ip3[1]->[1]ip6;
ip3[0]->Print("3.3.3.3 GET:")->Discard();
ip6[1]->[1]ip3;

ip4[2]->[2]ip5;
ip5[2]->[2]ip4;
ip4[3]->[3]ip6;
ip6[3]->[3]ip4;

Idle()->[0]ip4[0]->Discard();
Idle()->[0]ip5[0]->Discard();
Idle()->[0]ip6[0]->Discard();
Idle()->[2]ip6[2]->Discard();