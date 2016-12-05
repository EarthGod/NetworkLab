require(library /home/comnetsii/elements/lossyrouterport.click);

rp :: LossyRouterPort(DEV $dev, IN_MAC $in_mac , OUT_MAC $out_mac, LOSS 0.9, DELAY 0.2 );

client::BasicClient(MY_ADDRESS 1, OTHER_ADDRESS 2, DELAY 2);
bc::BasicClassifier;
client->rp->bc
bc[0]->[0]client;
bc[1]->Discard;
bc[2]->[1]client;

