require(library /home/comnetsii/elements/routerport.click);

rp1 :: RouterPort(DEV $dev1, IN_MAC $in_mac1 , OUT_MAC $out_mac1 );
rp2 :: RouterPort(DEV $dev2, IN_MAC $in_mac2 , OUT_MAC $out_mac2 );
rp3 :: RouterPort(DEV $dev3, IN_MAC $in_mac3 , OUT_MAC $out_mac3 );

router::BasicRouter;
rp1->[0]router[0]->rp1;
rp2->[1]router[1]->rp2;
rp3->[2]router[2]->rp3;