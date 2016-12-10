#ifndef TCP_CONTROL_BLOCK_HH
#define TCP_CONTROL_BLOCK_HH

#include "tcp_packet.hh"

typedef enum{
	CLOSED,
	LISTEN,
	SYN_SENT,
	ESTAB,
	FIN_WAIT,
	CLOSE_WAIT
} stat;

class TCB{
public:
	int type; //0 for client, 1 for server 
	uint32t ipaddr;
	int nxt_ack; //0 or 1
	int nxt_seq; //0 or 1
	stat state;
	WritablePacket pbuff;
}

#endif