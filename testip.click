require(library /home/comnetsii/elements/routerport.click);

ip1 :: ip(IP_ADDR 1.2.3.4);
ip2 :: ip(IP_ADDR 2.3.4.5);
ip3 :: ip(IP_ADDR 5.6.7.8);

cl1 :: simipgen(1.2.3.4, 2.3.4.5);

cl1->[0]ip1[1]->[1]ip3[2]->[2]ip2[0]->Print("GOTCHA")->Discard();

Idle()->[0]ip2[2]->[2]ip3[1]->[1]ip1[0]->Discard();

Idle()->[0]ip3[0]->Discard();
Idle()->[1]ip2[1]->Discard();