require(library /home/comnetsii/elements/routerport.click);

rp :: RouterPort(DEV $dev, IN_MAC $in_mac , OUT_MAC $out_mac );
pp :: PacketPrinter;

Idle->rp->pp;
pp[0]->rp;
pp[1]->Discard();