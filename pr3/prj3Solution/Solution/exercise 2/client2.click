require(library /home/comnetsii/elements/routerport.click);

rp :: RouterPort(DEV $dev, IN_MAC $in_mac , OUT_MAC $out_mac );

client::BasicClient(MY_ADDRESS 2, OTHER_ADDRESS 1);
bc::BasicClassifier;
client->rp->bc
bc[0]->[0]client;
bc[1]->Discard;
bc[2]->[1]client;

