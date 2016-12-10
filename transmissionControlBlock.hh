#ifndef TCP_CONTROL_BLOCK_HH
#define TCP_CONTROL_BLOCK_HH

#include "tcp_packet.hh"
#include <deque>
using std::deque;

typedef enum{
	CLOSED,
	LISTEN,
	SYN_SENT,
	SYN_WAIT,
	ESTAB,
	FIN_SENT
	FIN_WAIT,
} stat;

class TCB{
public:
	 
	uint32_t ipaddr;
	uint16_t tcpport;
	int nxt_ack; //0 or 1
	int nxt_seq; //0 or 1
	stat state;
	deque<WritablePacket*> pbuff;
}

#endif