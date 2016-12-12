require(library /home/comnetsii/elements/lossyrouterport.click);

tcp1 :: tcpEntity(MY_PORT_NUM 111, TYPE 1, TIME_OUT 400, MY_IP_ADDR 1.2.3.4); //server
tcp2 :: tcpEntity(MY_PORT_NUM 222, TYPE 0, TIME_OUT 400, MY_IP_ADDR 1.2.3.5); //client

lrp1 :: LossyRouterPort(DEV veth1, IN_MAC 2a:bc:e1:e9:4d:40, OUT_MAC 32:cf:9e:96:9f:43,LOSS 0.9,DELAY 0.05);
lrp2 :: LossyRouterPort(DEV veth2, IN_MAC 32:cf:9e:96:9f:43, OUT_MAC 2a:bc:e1:e9:4d:40,LOSS 0.9,DELAY 0.05);
cl1 :: PacketGenerator(222, 1.2.3.5);

cl1->[1]tcp1[0]->lrp1;
lrp2->[0]tcp2[1]->Print("Reciever got:")->Discard();
tcp1[1]->[1]tcp2;
tcp2[0]->lrp2;
lrp1->[0]tcp1;