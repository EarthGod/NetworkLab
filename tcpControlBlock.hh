#ifndef TCP_CONTROL_BLOCK_HH
#def TCP_CONTROL_BLOCK_HH

#inlcude "packets.hh"
class TCB{
public:
	int ack;
	int seq;
	packets buff;
	Timer timer_tick;
	
}

#endif