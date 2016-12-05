FromDevice($dev)
			-> HostEtherFilter($src_mac, DROP_OWN true) // check that the mac address is proper
			-> Print(Received)
       	 	-> Strip(14) //Strip ethernet
			-> Strip(20) //Strip IP
			-> Queue
			-> IPEncap(4, $src_ip, $dst_ip)
			-> EtherEncap(0x0800, $src_mac, $dst_mac)
			-> Print(Sending)
			-> ToDevice($dev)