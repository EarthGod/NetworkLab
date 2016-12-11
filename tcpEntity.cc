#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/packet.hh>
#include "tcpEntity.hh"

//using std::vector;
CLICK_DECLS

tcpEntity::tcpEntity() : _fin_timer(this), _ack_timer(this){	
}

tcpEntity::~tcpEntity(){	
}

int tcpEntity :: configure(Vector<String> &conf, ErrorHandler *errh)
{
	if (cp_va_kparse(conf, this, errh,
                  "MY_PORT_NUM", cpkP+cpkM, cpUnsigned, &_my_port_number,
				  "TYPE", cpkP+cpkM, cpUnsigned, &tcpEntity_type,
                  "TIME_OUT", cpkP+cpkM, cpUnsigned, &_time_out,
                  "MY_IP_ADDR", cpkP+cpkM, cpUnsigned, &_my_ipaddr,
                  cpEnd) < 0) {
    return -1;
	}
	return 0;

}

/*

*/

int tcpEntity::initialize(ErrorHandler*)
{
	_fin_timer.initialize(this);
	_ack_timer.initialize(this);
	_ack_tif = TimerInfo();
	_fin_tif = TimerInfo();
	_control_block.nxt_ack = 0;
	_control_block.nxt_seq = 0;
	_control_block.tcp_port = 0;
	if(tcpEntity_type == 0)//client
		_control_block.state = CLOSED;
	if(tcpEntity_type == 1)//server
		_control_block.state = LISTEN;
}


void tcpEntity::set_resend_timer(Timer* _timer, TimerInfo tif)
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


void tcpEntity::cancel_timer(Timer* _timer, TimerInfo tif)
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

void tcpEntity::push (int port, Packet *packet)
{
	
	if(port == 1) // from client
	{
		if(tcpEntity_type == 0)//tcp as cilent role
		{
			fprintf(stderr, "Wrong input, tcp client could not send data actively.\n");
			packet->kill();
			// should not recieve anything from client
		}
		
		else if(tcpEntity_type == 1)//tcp as server role
		{
			pbuff.push_back(packet->clone()); //put data in queue
		}
	}
	
	if(port == 0) // from ip
	{
		tcp_types ptype = (MyTCPHeader*)packet->type;
		int seq = ((MyTCPHeader*)packet->sequence);
		int ack = ((MyTCPHeader*)packet->ack_num);
		int sport = ((MyTCPHeader*)packet->source);
		int dport = ((MyTCPHeader*)packet->destination);
		int sipaddr = ((MyTCPHeader*)packet->source_ip);
		if(tcpEntity_type == 0)//client
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
				if(ptype != SYN+ACK || (sipaddr) != _control_block.ipaddr)
				{
					fprintf(stderr, "wrong packet, tcp client is waiting for SYNACK.\n");
					packet->kill();
					break;
				}
				//send ACK
				nxt_ack = (nxt_ack + 1) % 2;
				WritablePacket *ack_pack = Packet::make(0,0,sizeof(MyTCPHeader) + sizeof(ip_header),0);//leaving the space for ip_header
				memset(ack_pack,0,ack_pack->length);
				(MyTCPHeader*)ack_pack->type = ACK;
				(MyTCPHeader*)ack_pack->ack_num = (nxt_ack);
				(MyTCPHeader*)ack_pack->source = (_my_port_number);
				(MyTCPHeader*)ack_pack->destination = (_control_block.tcpport);
				(MyTCPHeader*)ack_pack->source_ip = (_my_ipaddr);
				(MyTCPHeader*)ack_pack->dest_ip = (sipaddr);
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
					WritablePacket *ack_pack = Packet::make(0,0,sizeof(MyTCPHeader) + sizeof(ip_header),0);//leaving the space for ip_header
					memset(ack_pack,0,ack_pack->length);
					(MyTCPHeader*)ack_pack->type = FIN+ACK;
					(MyTCPHeader*)ack_pack->ack_num = (nxt_ack);
					(MyTCPHeader*)ack_pack->source = (_my_port_number);
					(MyTCPHeader*)ack_pack->destination = (_control_block.tcpport);
					(MyTCPHeader*)ack_pack->source_ip = (_my_ipaddr);
					(MyTCPHeader*)ack_pack->dest_ip = (sipaddr);
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
					WritablePacket *data_pack = Packet::make(0,0,packet.length() - sizeof(MyTCPHeader),0)
					memcpy(data_pack, (char*)packet + sizeof(MyTCPHeader), packet.length() - sizeof(MyTCPHeader));
					output(0).push(data_pack);
					WritablePacket *ack_pack = Packet::make(0,0,sizeof(MyTCPHeader) + sizeof(ip_header),0);//leaving the space for ip_header
					memset(ack_pack,0,ack_pack->length);
					(MyTCPHeader*)ack_pack->type = ACK;
					(MyTCPHeader*)ack_pack->ack_num = (nxt_ack);
					(MyTCPHeader*)ack_pack->source = (_my_port_number);
					(MyTCPHeader*)ack_pack->destination = (_control_block.tcpport);
					(MyTCPHeader*)ack_pack->source_ip = (_my_ipaddr);
					(MyTCPHeader*)ack_pack->dest_ip = (sipaddr);
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
		
		else if(tcpEntity_type == 1)//server
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
				WritablePacket *ack_pack = Packet::make(0,0,sizeof(MyTCPHeader) + sizeof(ip_header),0);//leaving the space for ip_header
				memset(ack_pack,0,ack_pack->length);
				(MyTCPHeader*)ack_pack->type = SYN+ACK;
				(MyTCPHeader*)ack_pack->ack_num = (nxt_ack);
				(MyTCPHeader*)ack_pack->source = (_my_port_number);
				(MyTCPHeader*)ack_pack->destination = (_control_block.tcpport);
				(MyTCPHeader*)ack_pack->source_ip = (_my_ipaddr);
				(MyTCPHeader*)ack_pack->dest_ip = (sipaddr);
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
				WritablePacket *data_pack = Packet::make(0,0,packet.length() - sizeof(MyTCPHeader),0)
				memcpy(data_pack, (char*)packet + sizeof(MyTCPHeader), packet.length() - sizeof(MyTCPHeader));
				output(0).push(data_pack);
				// sending ACK+DATA
				
				assert(pbuff.front());
				uint32_t tlen = pbuff.front()->length();
				WritablePacket *ack_pack = pbuff.front()->clone()->push(sizeof(MyTCPHeader));// add the header on
				
				(MyTCPHeader*)ack_pack->type = ACK+DATA;
				(MyTCPHeader*)ack_pack->sequence = (_control_block.nxt_seq);
				(MyTCPHeader*)ack_pack->ack_num = (nxt_ack);
				(MyTCPHeader*)ack_pack->source = (_my_port_number);
				(MyTCPHeader*)ack_pack->destination = (_control_block.tcpport);
				(MyTCPHeader*)ack_pack->size = (tlen);
				(MyTCPHeader*)ack_pack->source_ip = (_my_ipaddr);
				(MyTCPHeader*)ack_pack->dest_ip = (sipaddr);
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
					WritablePacket *fin_pack = Packet::make(0,0,sizeof(MyTCPHeader) + sizeof(ip_header),0);//leaving the space for ip_header
					memset(fin_pack,0,fin_pack->length);
					(MyTCPHeader*)fin_pack->type = FIN;
					(MyTCPHeader*)fin_pack->source = (_my_port_number);
					(MyTCPHeader*)fin_pack->destination = (_control_block.tcpport);
					(MyTCPHeader*)fin_pack->source_ip = (_my_ipaddr);
					(MyTCPHeader*)fin_pack->dest_ip = (sipaddr);
					set_resend_timer(_fin_timer, TimerInfo(ESTAB,nxt_seq,nxt_ack,fin_pack));
					output(1).push(fin_pack);
					break;
				}
				cancel_timer(_ack_timer, _ack_tif);
				uint32_t tlen = pbuff.front()->length();
				WritablePacket *ack_pack = pbuff.front()->clone()->push(sizeof(MyTCPHeader));// add the header on
				(MyTCPHeader*)ack_pack->type = DATA;
				(MyTCPHeader*)ack_pack->sequence = (_control_block.nxt_seq);
				(MyTCPHeader*)ack_pack->ack_num = (nxt_ack);
				(MyTCPHeader*)ack_pack->source = (_my_port_number);
				(MyTCPHeader*)ack_pack->destination = (_control_block.tcpport);
				(MyTCPHeader*)ack_pack->size = (tlen);
				(MyTCPHeader*)ack_pack->source_ip = (_my_ipaddr);
				(MyTCPHeader*)ack_pack->dest_ip = (sipaddr);
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
void tcpEntity::run_timer(Timer *timer) {
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
EXPORT_ELEMENT(tcpEntity)
