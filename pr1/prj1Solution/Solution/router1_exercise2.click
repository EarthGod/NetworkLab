//
// Sends 10 "hello" messages, 1 per second to specified destination address and interface
// Receive messages from given interface and print
//
// $src_ip
// $dst_ip
// $src_mac
// $dst_mac
// $dev
//

RatedSource(DATA "hello", RATE 1, LIMIT 10)
						-> IPEncap(4, $src_ip, $dst_ip)
						-> EtherEncap(0x0800, $src_mac, $dst_mac)
						-> Print(Sending)
						-> Queue
						-> ToDevice($dev)

FromDevice($dev)
			-> HostEtherFilter($src_mac, DROP_OWN true) // check that the mac address is proper
			-> Print(Received)
       	 	-> Strip(14) //Strip ethernet
			-> Strip(20) //Strip IP
			-> Discard
