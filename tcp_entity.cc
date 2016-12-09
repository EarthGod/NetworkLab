#ifndef CLICK_BASICCLIENT_HH 
#define CLICK_BASICCLIENT_HH 
#include <click/element.hh>
#include <click/timer.hh>
#include <vector>
#include "tcp_entity.hh"
#include "transmissionControlBlock.hh"

using std::vector;
CLICK_DECLS

tcp_entity::tcp_entity() : _timer(this){	
}

tcp_entity::~tcp_entity(){	
}

int tcp_entity :: configure(Vector<String> &conf, ErrorHandler *errh)
{
	if (cp_va_kparse(conf, this, errh,
                  "MY_PORT_NUM", cpkP+cpkM, cpUnsigned, &_my_port_number,
				  "TYPE", cpkP+cpkM, cpUnsigned, &(_control_block.type),
                  "TIME_OUT", cpkP+cpkM, cpUnsigned, &_time_out,
                  cpEnd) < 0) {
    return -1;
	}
	return 0;

}

/*

*/
int tcp_entity::initialize(ErrorHandler*)
{
	_timer.initialize(this);
	_timer.schedule_now();
	_control_block.nxt_ack = 0;
	_control_block.nxt_seq = 0;
	_control_block.state = CLOSED;
}

void tcp_entity::push (int port, Packet *packet)
{
	
}

CLICK_ENDDECLS
#endif 
