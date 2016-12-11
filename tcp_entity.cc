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

tcp_entity::tcp_entity() : _fin_timer(this), _ack_timer(this){	
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
	_fin_timer.initialize(this);
	_ack_timer.initialize(this);
	_control_block.nxt_ack = 0;
	_control_block.nxt_seq = 0;
	_control_block.tcp_port = 0;
	if(tcp_entity_type == 0)//client
		_control_block.state = CLOSED;
	if(tcp_entity_type == 1)//server
		_control_block.state = LISTEN;
}


void tcp_entity::set_resend_timer(Timer* _timer, TimerInfo tif)
{
	if(_timer == _ack_timer)
	{
		_ack_tif = tif;
		_ack_timer.schedule_after_msec(_time_out);
	}
	else if(_timer == _fin_timer)
	{
		_fin_tif = tif;
		_fin_timer.schedule_after_msec(2 * _time_out);
	}
}


void tcp_entity::cancel_timer(Timer* _timer, TimerInfo tif)
{
	if(_timer == _ack_timer)
	{
		if(_ack_tif.tsat == tif.tsat && _ack_tif.tseq == tif.tseq && _ack_tif.tack == tif.tack)
				_ack_timer.unschedule();
		return;
	}
	else if(_timer == _fin_timer)
	{
		if(_seq_tif.tsat == tif.tsat && _seq_tif.tseq == tif.tseq && _seq_tif.tack == tif.tack)
				_ack_timer.unschedule();
		return;
	}
}

void tcp_entity::push (int port, Packet *packet)
{
	
	if(port == 0) // from client
	{
		if(tcp_entity_type == 0)//tcp as cilent role
		{
			fprintf(stderr, "Wrong input, tcp client could not send data actively.\n");
			packet->kill();
			// should not recieve anything from client
		}
		
		else if(tcp_entity_type == 1)//tcp as server role
		{
			pbuff.push_back(packet->clone()); //put data in queue
		}
	}
	
	if(port == 1) // from ip
	{
		tcp_types ptype = (tcp_header*)packet->type;
		int seq = ntohs((tcp_header*)packet->sequence);
		int ack = ntohs((tcp_header*)packet->ack_num);
		int sport = ntohs((tcp_header*)packet->source);
		int dport = ntohs((tcp_header*)packet->destination);
		int sipaddr = ntohl((tcp_header*)packet->source_ip);
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
				//expecting SYN_ACK
				if(ptype != SYN+ACK || ntohl(sipaddr) != _control_block.ipaddr)
				{
					fprintf(stderr, "wrong packet, tcp client is waiting for SYNACK.\n");
					packet->kill();
					break;
				}
				//send ACK
				nxt_ack = (nxt_ack + 1) % 2;
				WritablePacket *ack_pack = Packet::make(0,0,sizeof(tcp_header) + sizeof(ip_header),0);//leaving the space for ip_header
				memset(ack_pack,0,ack_pack->length);
				(tcp_header*)ack_pack->type = ACK;
				(tcp_header*)ack_pack->ack_num = htons(nxt_ack);
				(tcp_header*)ack_pack->source = htons(_my_port_number);
				(tcp_header*)ack_pack->destination = htons(_control_block.tcpport);
				(tcp_header*)ack_pack->source_ip = htonl(_my_ipaddr);
				(tcp_header*)ack_pack->dest_ip = htonl(sipaddr);
				set_resend_timer(_ack_timer, TimerInfo(SYN_SENT,nxt_seq,nxt_ack,ack_pack));
				output(1).push(ack_pack);
				break;
			}
			
			case ESTAB:
			{
				//check the seq, send ack to ip and pass data to client if correct.
				//if recieve fin, send ack+fin to ip, change to FIN_WAIT state
				if(ptype != FIN+DATA && ptye != DATA)
				{
					fprintf(stderr, "wrong packet, tcp client is waiting for DATA or FIN.\n");
					packet->kill();
					break;
				}
				
				if(seq != nxt_ack)
				{
					packet->kill();
					break;
				}
				
				if(ptype == FIN+DATA)
				{
					nxt_ack = (nxt_ack + 1) % 2;
					WritablePacket *ack_pack = Packet::make(0,0,sizeof(tcp_header) + sizeof(ip_header),0);//leaving the space for ip_header
					memset(ack_pack,0,ack_pack->length);
					(tcp_header*)ack_pack->type = FIN+ACK;
					(tcp_header*)ack_pack->ack_num = htons(nxt_ack);
					(tcp_header*)ack_pack->source = htons(_my_port_number);
					(tcp_header*)ack_pack->destination = htons(_control_block.tcpport);
					(tcp_header*)ack_pack->source_ip = htonl(_my_ipaddr);
					(tcp_header*)ack_pack->dest_ip = htonl(sipaddr);
					output(1).push(ack_pack);
					set_resend_timer(_ack_timer, TimerInfo(ESTAB,nxt_seq,nxt_ack,ack_pack));
					set_fin_timer(_fin_timer, TimerInfo(ESTAB,nxt_seq,nxt_ack,ack_pack));
					_control_block.stat = FIN_WAIT;
					break;
				}
				
				
				else if(ptype == DATA)
				{
					cancel_timer(_ack_timer, _ack_tif);
					nxt_ack = (nxt_ack + 1) % 2;
					WritablePacket *data_pack = Packet::make(0,0,packet.length() - sizeof(tcp_header),0)
					memcpy(data_pack, (char*)packet + sizeof(tcp_header), packet.length() - sizeof(tcp_header));
					output(0).push(data_pack);
					WritablePacket *ack_pack = Packet::make(0,0,sizeof(tcp_header) + sizeof(ip_header),0);//leaving the space for ip_header
					memset(ack_pack,0,ack_pack->length);
					(tcp_header*)ack_pack->type = ACK;
					(tcp_header*)ack_pack->ack_num = htons(nxt_ack);
					(tcp_header*)ack_pack->source = htons(_my_port_number);
					(tcp_header*)ack_pack->destination = htons(_control_block.tcpport);
					(tcp_header*)ack_pack->source_ip = htonl(_my_ipaddr);
					(tcp_header*)ack_pack->dest_ip = htonl(sipaddr);
					set_resend_timer(_ack_timer, TimerInfo(ESTAB,nxt_seq,nxt_ack,ack_pack));
					output(1).push(ack_pack);
					break;
					
				}
			}
				
			
			
			case FIN_WAIT:
			{
				//waiting for ack, if recieved ack or time-out, change to CLOSED state
				if(ptype != ACK || ack != _control_block.nxt_seq)
				{
					fprintf(stderr, "wrong packet, tcp client is waiting for ACK of FIN.\n");
					packet->kill();
					break;
				}
				else
				{
					_control_block.nxt_ack = _control_block.nxt_seq = 0;
					_control_block.state = CLOSED;
					break;
				}
				
			}
		
		}
		
		else if(tcp_entity_type == 1)//server
		{
			switch(_control_block.state)
			{
			case LISTEN:
			{
				//Expecting SYN request. When recieved, send SYN_ACK to ip and set the <ip,port> tuple in tcb.
				if(ptype != SYN) // assuming that SYN's seq is always 0
				{
					fprintf(stderr, "wrong packet, tcp server is waiting for SYN.\n");
					packet->kill();
					break;
				}
				nxt_ack = (nxt_ack + 1) % 2;
				_control_block.ipaddr = sipaddr;
				_control_block.tcpport = sport;
				WritablePacket *ack_pack = Packet::make(0,0,sizeof(tcp_header) + sizeof(ip_header),0);//leaving the space for ip_header
				memset(ack_pack,0,ack_pack->length);
				(tcp_header*)ack_pack->type = SYN+ACK;
				(tcp_header*)ack_pack->ack_num = htons(nxt_ack);
				(tcp_header*)ack_pack->source = htons(_my_port_number);
				(tcp_header*)ack_pack->destination = htons(_control_block.tcpport);
				(tcp_header*)ack_pack->source_ip = htonl(_my_ipaddr);
				(tcp_header*)ack_pack->dest_ip = htonl(sipaddr);
				set_resend_timer(_ack_timer, TimerInfo(ESTAB,nxt_seq,nxt_ack,ack_pack));
				output(1).push(ack_pack);
			}
			case SYN_WAIT:
			{
				//Waiting for ACK. When recieved, send ack+data to ip, change to ESTAB state.
				if(sipaddr != _control_block.ipaddr || sport != _control_block.tcpport)
				{
					fprintf(stderr, "Recieved packet from an unconnected socket while busy.\n");
					packet->kill();
					break;
				}
				if(ptype != ACK+DATA || seq != nxt_ack) 
				{
					fprintf(stderr, "wrong packet, tcp server is waiting for ACK+DATA.\n");
					packet->kill();
					break;
				}
				cancel_timer(_ack_timer, _ack_tif);
				nxt_ack = (nxt_ack + 1) % 2;
				WritablePacket *data_pack = Packet::make(0,0,packet.length() - sizeof(tcp_header),0)
				memcpy(data_pack, (char*)packet + sizeof(tcp_header), packet.length() - sizeof(tcp_header));
				output(0).push(data_pack);
				// sending ACK+DATA
				
				assert(pbuff.front());
				uint32_t tlen = pbuff.front()->length();
				WritablePacket *ack_pack = pbuff.front()->clone()->push(sizeof(tcp_header));// add the header on
				
				(tcp_header*)ack_pack->type = ACK+DATA;
				(tcp_header*)ack_pack->sequence = htons(_control_block.nxt_seq);
				(tcp_header*)ack_pack->ack_num = htons(nxt_ack);
				(tcp_header*)ack_pack->source = htons(_my_port_number);
				(tcp_header*)ack_pack->destination = htons(_control_block.tcpport);
				(tcp_header*)ack_pack->size = htonl(tlen);
				(tcp_header*)ack_pack->source_ip = htonl(_my_ipaddr);
				(tcp_header*)ack_pack->dest_ip = htonl(sipaddr);
				nxt_seq = (nxt_seq + 1) % 2;
				pbuff.pop_front();
				_control_block.state = ESTAB;
				set_resend_timer(_ack_timer, TimerInfo(ESTAB,nxt_seq,nxt_ack,ack_pack));
				output(1).push(ack_pack);
				break;			
			}
			
			case ESTAB:
			{
				//Expecting ack, set the seq.
				if(sipaddr != _control_block.ipaddr || sport != _control_block.tcpport)
				{
					fprintf(stderr, "Recieved packet from an unconnected socket while busy.\n");
					packet->kill();
					break;
				}
				
				if(ptype != ACK || ack != nxt_seq) 
				{
					fprintf(stderr, "wrong packet, tcp server is waiting for ACK+DATA.\n");
					packet->kill();
					break;
				}
				
				if(pbuff.empty())
				{
					WritablePacket *fin_pack = Packet::make(0,0,sizeof(tcp_header) + sizeof(ip_header),0);//leaving the space for ip_header
					memset(fin_pack,0,fin_pack->length);
					(tcp_header*)fin_pack->type = FIN;
					(tcp_header*)fin_pack->source = htons(_my_port_number);
					(tcp_header*)fin_pack->destination = htons(_control_block.tcpport);
					(tcp_header*)fin_pack->source_ip = htonl(_my_ipaddr);
					(tcp_header*)fin_pack->dest_ip = htonl(sipaddr);
					set_resend_timer(_fin_timer, TimerInfo(ESTAB,nxt_seq,nxt_ack,fin_pack));
					output(1).push(fin_pack);
					break;
				}
				cancel_timer(_ack_timer, _ack_tif);
				uint32_t tlen = pbuff.front()->length();
				WritablePacket *ack_pack = pbuff.front()->clone()->push(sizeof(tcp_header));// add the header on
				(tcp_header*)ack_pack->type = DATA;
				(tcp_header*)ack_pack->sequence = htons(_control_block.nxt_seq);
				(tcp_header*)ack_pack->ack_num = htons(nxt_ack);
				(tcp_header*)ack_pack->source = htons(_my_port_number);
				(tcp_header*)ack_pack->destination = htons(_control_block.tcpport);
				(tcp_header*)ack_pack->size = htonl(tlen);
				(tcp_header*)ack_pack->source_ip = htonl(_my_ipaddr);
				(tcp_header*)ack_pack->dest_ip = htonl(sipaddr);
				nxt_seq = (nxt_seq + 1) % 2;
				pbuff.pop_front();
				set_resend_timer(_ack_timer, TimerInfo(ESTAB,nxt_seq,nxt_ack,ack_pack));
				output(1).push(ack_pack);
				
			}
			
			case FIN_SENT:
			{
				//Waiting for ack of fin. Change to LISTEN when ack recieved or time-out.
				if(sipaddr != _control_block.ipaddr || sport != _control_block.tcpport)
				{
					fprintf(stderr, "Recieved packet from an unconnected socket while busy.\n");
					packet->kill();
					break;
				}
				
				if(ptype != ACK+FIN) 
				{
					fprintf(stderr, "wrong packet, tcp server is waiting for ACK+DATA.\n");
					packet->kill();
					break;
				}
				_control_block.nxt_ack = _control_block.nxt_seq = 0;
				state = LISTEN;
				break;
				
			}
		}
}
void BasicClient::run_timer(Timer *timer) {
    if(timer == _fin_timer)
	{
		_control_block.nxt_ack = _control_block.nxt_seq = 0;
		_control_block.state = CLOSED;
	}
	else if(timer == _ack_timer)
	{
		output(1).push(_ack_tif.pack);
	}
}
CLICK_ENDDECLS
#endif 
