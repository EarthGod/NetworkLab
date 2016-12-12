require(library /home/comnetsii/elements/routerport.click);

//path between ip1&ip4
rp1 :: RouterPort(DEV veth1, IN_MAC 3e:a9:5f:81:9c:5b, OUT_MAC a2:bb:d3:ab:8f:f5);
rp2 :: RouterPort(DEV veth2, IN_MAC a2:bb:d3:ab:8f:f5, OUT_MAC 3e:a9:5f:81:9c:5b);
//path between ip4&ip5
rp3 :: RouterPort(DEV veth3, IN_MAC 0a:65:70:78:0f:51, OUT_MAC 46:c8:b7:40:e9:50);
rp4 :: RouterPort(DEV veth4, IN_MAC 46:c8:b7:40:e9:50, OUT_MAC 0a:65:70:78:0f:51);
//path between ip5&ip2
rp5 :: RouterPort(DEV veth5, IN_MAC 5a:e8:03:37:31:70, OUT_MAC da:bd:ae:e6:2c:95);
rp6 :: RouterPort(DEV veth6, IN_MAC da:bd:ae:e6:2c:95, OUT_MAC 5a:e8:03:37:31:70);
//path between ip4&ip6
rp7 :: RouterPort(DEV veth7, IN_MAC 82:fe:1c:cd:97:dc, OUT_MAC fa:5e:dc:98:4f:a9);
rp8 :: RouterPort(DEV veth8, IN_MAC fa:5e:dc:98:4f:a9, OUT_MAC 82:fe:1c:cd:97:dc);
//path between ip6&ip3
rp9 :: RouterPort(DEV veth9, IN_MAC 66:2f:8b:49:96:12, OUT_MAC 4e:11:c8:00:de:28);
rp10 :: RouterPort(DEV veth10, IN_MAC 4e:11:c8:00:de:28, OUT_MAC 66:2f:8b:49:96:12);

ip1 :: ip(IP_ADDR 1.1.1.1);
ip2 :: ip(IP_ADDR 2.2.2.2);
ip3 :: ip(IP_ADDR 3.3.3.3);
ip4 :: ip(IP_ADDR 4.4.4.4);
ip5 :: ip(IP_ADDR 5.5.5.5);
ip6 :: ip(IP_ADDR 6.6.6.6);

cl1 :: simipgen(1.1.1.1, 2.2.2.2);
cl2 :: simipgen(2.2.2.2, 3.3.3.3);
cl3 :: simipgen(3.3.3.3, 1.1.1.1);

cl1->[0]ip1[1]->rp1;
rp2->[1]ip4;
ip1[0]->Print("CLIENT1-1.1.1.1 GET")->Discard();
ip4[1]->rp2;
rp1->[1]ip1;

cl2->[0]ip2[1]->rp6;
rp5->[1]ip5;
ip2[0]->Print("CLIENT2-2.2.2.2 GET")->Discard();
ip5[1]->rp5;
rp6->[1]ip2;

cl3->[0]ip3[1]->rp10;
rp9->[1]ip6;
ip3[0]->Print("CLIENT3-3.3.3.3 GET")->Discard();
ip6[1]->rp9;
rp10->[1]ip3;

ip4[2]->rp3;
rp4->[2]ip5;
ip5[2]->rp4;
rp3->[2]ip4;

ip4[3]->rp7;
rp8->[3]ip6;
ip6[3]->rp8;
rp7->[3]ip4;

Idle()->[0]ip4[0]->Discard();
Idle()->[0]ip5[0]->Discard();
Idle()->[0]ip6[0]->Discard();
Idle()->[2]ip6[2]->Discard();