#ifndef CLICK_BASICCLIENT_HH 
#define CLICK_BASICCLIENT_HH 
#include <click/element.hh>
#include <click/timer.hh>
#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
//#include <vector>
#include "tcp_entity.hh"
#include "transmissionControlBlock.hh"

//using std::vector;
CLICK_DECLS

tcp_entity::tcp_entity() : _timer(this){	
}

tcp_entity::~tcp_entity(){	
}

int tcp_entity :: configure(Vector<String> &conf, ErrorHandler *errh)
{
	if (cp_va_kparse(conf, this, errh,
                  "MY_PORT_NUM", cpkP+cpkM, cpUnsigned, &_my_port_number,
				  "TYPE", cpkP+cpkM, cpUnsigned, &tcp_entity_type,
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
	_control_block.tcp_port = 0;
	if(tcp_entity_type == 0)//client
		_control_block.state = CLOSED;
	if(tcp_entity_type == 1)//server
		_control_block.state = LISTEN;
}

void tcp_entity::push (int port, Packet *packet)
{
	
	if(port == 0) // from client
	{
		if(tcp_entity_type == 0)//tcp as cilent role
		{
			// should not recieve anything from client
		}
		
		else if(tcp_entity_type == 1)//tcp as server role
		{
			pbuff.push_back(packet->clone()); //put data in queue
		}
	}
	
	if(port == 1) // from ip
	{
		tcp_types ptype = (MyTCPHeader*)packet->type;
		int seq = (MyTCPHeader*)packet->sequence;
		int ack = (MyTCPHeader*)packet->ack_num;
		int sport = (MyTCPHeader*)packet->source;
		int dport = (MyTCPHeader*)packet->destination;
		int sipaddr = (MyTCPHeader*)packet->source_ip;
		if(tcp_entity_type == 0)//client
		{
			switch(_control_block.state)
			{
			case CLOSED:
			{
				//should not recieve anything from ip when closed
				fprintf(stderr, "wrong packet, tcp client is now closed.\n");
				packet->kill();
				break;
			}
			case SYN_SENT:
			{
				if(ptype != SYN+ACK || ntohl(sipaddr) != _control_block.ipaddr)
				{
					fprintf(stderr, "wrong packet, tcp client is waiting for SYNACK.\n");
					packet->kill();
					break;
				}
				// sending ACK_DATA
				assert(pbuff);
				uint32_t tlen = pbuff->length();
				WritablePacket *ack_pack = pbuff->clone()->push(sizeof(MyTCPHeader));
				(MyTCPHeader*)ack_pack->type = ACK+DATA;
				(MyTCPHeader*)ack_pack->sequence = htons(_control_block.nxt_seq);
				(MyTCPHeader*)ack_pack->ack_num = htons((seq + 1) % 2);
				(MyTCPHeader*)ack_pack->source = htons(_my_port_number);
				(MyTCPHeader*)ack_pack->destination = htons(_control_block.tcpport);
				(MyTCPHeader*)ack_pack->size = htonl(tlen);
				output(1).push(ack_pack);
				pbuff->kill();
				_control_block.state = ESTAB;
				break;
			}
			
			case ESTAB:
			{
				//check the seq, send ack to ip and pass data to client if correct.
				//if recieve fin, send ack+fin to ip, change to FIN_WAIT state
			}
			
			case FIN_WAIT:
			{
				//waiting for ack, if recieved ack or time-out, change to CLOSED state
			}
		
		}
		
		else if(tcp_entity_type == 1)//server
		{
			switch(_control_block.state)
			{
			case LISTEN:
			{
				//Expecting SYN request. When recieved, send SYN_ACK to ip and set the <ip,port> tuple in tcb.
			}
			case SYN_WAIT:
			{
				//Waiting for ACK_DATA. When recieved, pass data to client and send ack to ip, change to ESTAB state.
			}
			
			case ESTAB:
			{
				//Expecting ack, set the seq.
			}
			case FIN_SENT:
			{
				//Waiting for ack of fin. Change to LISTEN when ack recieved or time-out.
			}
		}
}

CLICK_ENDDECLS
#endif 
