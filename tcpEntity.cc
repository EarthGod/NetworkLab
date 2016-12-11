#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/packet.hh>
#include "tcpEntity.hh"

//using std::vector;
CLICK_DECLS

tcpEntity::tcpEntity() : _fin_timer(this), _ack_timer(this), _debug_timer(this){	
}

tcpEntity::~tcpEntity(){	
}

int tcpEntity :: configure(Vector<String> &conf, ErrorHandler *errh)
{
	if (cp_va_kparse(conf, this, errh,
                  "MY_PORT_NUM", cpkP+cpkM, cpTCPPort, &_my_port_number,
				  "TYPE", cpkP+cpkM, cpUnsigned, &tcpEntity_type,
                  "TIME_OUT", cpkP+cpkM, cpUnsigned, &_time_out,
                  "MY_IP_ADDR", cpkP+cpkM, cpIPAddress, &_my_ipaddr,
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
	_control_block.nxt_ack = 0;
	_control_block.nxt_seq = 0;
	_control_block.tcpport = 0;
	_control_block.state = CLOSED;
	return 0;
}


void tcpEntity::set_resend_timer(Timer* _timer, TimerInfo tif)
{
	if(_timer == &_ack_timer)
	{
		_ack_tif = tif;
		_ack_timer.schedule_after_msec(_time_out);
	}
	else if(_timer == &_fin_timer)
	{
		_fin_tif = tif;
		_fin_timer.schedule_after_msec(2 * _time_out);
	}
}


void tcpEntity::cancel_timer(Timer* _timer, TimerInfo tif)
{
	if(_timer == &_ack_timer)
	{
		if(_ack_tif.tstat == tif.tstat && _ack_tif.tseq == tif.tseq && _ack_tif.tack == tif.tack)
				_ack_timer.unschedule();
		return;
	}
	else if(_timer == &_fin_timer)
	{
		if(_fin_tif.tstat == tif.tstat && _fin_tif.tseq == tif.tseq && _fin_tif.tack == tif.tack)
				_fin_timer.unschedule();
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
			fprintf(stderr, "Type is %d My port num is %d, MY ipaddr is %x, Time_out is %d\n",tcpEntity_type, _my_port_number, _my_ipaddr, _time_out);
			packet->kill();
			// should not recieve anything from client
		}
		
		else if(tcpEntity_type == 1)//tcp as server role
		{
			
			uint16_t dstport = ((MyClientHeader* )packet->data())->dst_port;
			uint32_t dstip = ((MyClientHeader* )packet->data())->dst_ip;
			fprintf(stderr, "TCP server recieved packet from client, dstport:%d, dstip:%x\n",dstport,dstip);
			WritablePacket* wp = Packet::make(NULL,packet->length()-sizeof(MyClientHeader));
			memcpy(wp->data(),packet->data()+sizeof(MyClientHeader),packet->length());
			_control_block.pbuff.push_back(wp);
			
			//send SYN
			fprintf(stderr, "sending SYN to %d %x\n", dstport, dstip);
			_control_block.ipaddr = dstip;
			_control_block.tcpport = dstport;
			
			WritablePacket* syn_pack = Packet::make(NULL,sizeof(MyTCPHeader));
			((MyTCPHeader*)syn_pack->data())->type = SYN;
			((MyTCPHeader*)syn_pack->data())->ack_num = _control_block.nxt_ack;
			((MyTCPHeader*)syn_pack->data())->source = _my_port_number;
			((MyTCPHeader*)syn_pack->data())->destination = dstport;
			((MyTCPHeader*)syn_pack->data())->source_ip = _my_ipaddr;
			((MyTCPHeader*)syn_pack->data())->dest_ip = dstip;
			set_resend_timer(&_ack_timer, TimerInfo(ESTAB,_control_block.nxt_seq,_control_block.nxt_ack,syn_pack));
			_control_block.state = SYN_SENT;
			output(0).push(syn_pack);
			
		}
	}
	
	if(port == 0) // from ip
	{
		uint16_t ptype = ((MyTCPHeader*)packet->data())->type;
		uint16_t seq = ((MyTCPHeader*)packet->data())->sequence;
		uint16_t ack = ((MyTCPHeader*)packet->data())->ack_num;
		uint16_t sport = ((MyTCPHeader*)packet->data())->source;
		uint16_t dport = ((MyTCPHeader*)packet->data())->destination;
		uint32_t sipaddr = ((MyTCPHeader*)packet->data())->source_ip;
		
		
		
		if(tcpEntity_type == 0)//client
		{
			fprintf(stderr, "TCP client recieved packet from ip, type:%d, seq:%d, ack:%d, sport:%d, dport:%d, sipaddr:%x\n",
		      ptype, seq, ack, sport, dport, sipaddr);
			switch(_control_block.state)
			{
				
			case CLOSED:
			{
				//Expecting SYN request. When recieved, send SYN_ACK to ip and set the <ip,port> tuple in tcb.
				if(ptype != SYN) // assuming that SYN's seq is always 0
				{
					fprintf(stderr, "tcp : wrong packet, tcp client is waiting for SYN.\n");
					packet->kill();
					break;
				}
				
				fprintf(stderr, "recieved SYN from %d %x\n", sport, sipaddr);
				
				_control_block.ipaddr = sipaddr;
				_control_block.tcpport = sport;
				WritablePacket *ack_pack = Packet::make(NULL, sizeof(MyTCPHeader));
				memset(ack_pack->data(),0,ack_pack->length());
				((MyTCPHeader*)ack_pack->data())->type = SYN+ACK;
				((MyTCPHeader*)ack_pack->data())->ack_num = (_control_block.nxt_ack);
				((MyTCPHeader*)ack_pack->data())->source = (_my_port_number);
				((MyTCPHeader*)ack_pack->data())->destination = (_control_block.tcpport);
				((MyTCPHeader*)ack_pack->data())->source_ip = (_my_ipaddr);
				((MyTCPHeader*)ack_pack->data())->dest_ip = (sipaddr);
				set_resend_timer(&_ack_timer, TimerInfo(ESTAB,_control_block.nxt_seq,_control_block.nxt_ack,ack_pack));
				_control_block.state = SYN_WAIT;
				output(0).push(ack_pack);
				
				
				
			}
			
			case SYN_WAIT:
			{
				//Waiting for ACK+DATA. When recieved, send ack to ip, change to ESTAB state.
				if(sipaddr != _control_block.ipaddr || sport != _control_block.tcpport)
				{
					fprintf(stderr, "tcp : Recieved packet from an unconnected socket while busy.\n");
					packet->kill();
					break;
				}
				if(ptype != ACK+DATA) 
				{
					fprintf(stderr, "tcp : Wrong packet, tcp client is waiting for ACK+DATA.\n");
					fprintf(stderr, "Curennt packet from ip is, type:%d, seq:%d, ack:%d, sport:%d, dport:%d, sipaddr:%x\n",
					ptype, seq, ack, sport, dport, sipaddr);
					packet->kill();
					break;
				}
				
				fprintf(stderr, "recieved ACK+DATA from %d %x\n", sport, sipaddr);
				
				cancel_timer(&_ack_timer, _ack_tif);
				_control_block.nxt_ack = (_control_block.nxt_ack + 1) % 2;
				WritablePacket *data_pack = Packet::make(NULL, packet->length() - sizeof(MyTCPHeader));
				memcpy(data_pack, packet->data() + sizeof(MyTCPHeader), packet->length() - sizeof(MyTCPHeader));
				output(1).push(data_pack);
				
				// sending ACK
				WritablePacket *ack_pack = Packet::make(NULL,sizeof(MyTCPHeader));
				((MyTCPHeader*)ack_pack->data())->type = ACK;
				((MyTCPHeader*)ack_pack->data())->sequence = (_control_block.nxt_seq);
				((MyTCPHeader*)ack_pack->data())->ack_num = (_control_block.nxt_ack);
				((MyTCPHeader*)ack_pack->data())->source = (_my_port_number);
				((MyTCPHeader*)ack_pack->data())->destination = (_control_block.tcpport);
				((MyTCPHeader*)ack_pack->data())->source_ip = (_my_ipaddr);
				((MyTCPHeader*)ack_pack->data())->dest_ip = (sipaddr);
				_control_block.pbuff.pop_front();
				_control_block.state = ESTAB;
				set_resend_timer(&_ack_timer, TimerInfo(ESTAB,_control_block.nxt_seq,_control_block.nxt_ack,ack_pack));
				output(0).push(ack_pack);
				break;			
			}
			case ESTAB:
			{
				fprintf(stderr, "link established to %d %x\n", sport, sipaddr);
				//check the seq, send ack to ip and pass data to client if correct.
				//if recieve fin, send ack+fin to ip, change to FIN_WAIT state
				if(ptype != FIN && ptype != DATA)
				{
					fprintf(stderr, "tcp : wrong packet, tcp client is waiting for DATA or FIN.\n");
					packet->kill();
					break;
				}
				
				if(seq != _control_block.nxt_ack)
				{
					packet->kill();
					break;
				}
				
				if(ptype == FIN)
				{
					_control_block.nxt_ack = (_control_block.nxt_ack + 1) % 2;
					WritablePacket *ack_pack = Packet::make(NULL,sizeof(MyTCPHeader));
					memset(ack_pack->data(),0,ack_pack->length());
					((MyTCPHeader*)ack_pack->data())->type = FIN+ACK;
					((MyTCPHeader*)ack_pack->data())->ack_num = (_control_block.nxt_ack);
					((MyTCPHeader*)ack_pack->data())->source = (_my_port_number);
					((MyTCPHeader*)ack_pack->data())->destination = (_control_block.tcpport);
					((MyTCPHeader*)ack_pack->data())->source_ip = (_my_ipaddr);
					((MyTCPHeader*)ack_pack->data())->dest_ip = (sipaddr);
					set_resend_timer(&_ack_timer, TimerInfo(ESTAB,_control_block.nxt_seq,_control_block.nxt_ack,ack_pack));
					set_resend_timer(&_fin_timer, TimerInfo(ESTAB,_control_block.nxt_seq,_control_block.nxt_ack,ack_pack));
					_control_block.state = FIN_WAIT;
					output(0).push(ack_pack);
					break;
				}
				
				
				else if(ptype == DATA)
				{
					fprintf(stderr, "tcp : recieved DATA from %d %x\n", sport, sipaddr);
					//send to client
					cancel_timer(&_ack_timer, _ack_tif);
					_control_block.nxt_ack = (_control_block.nxt_ack + 1) % 2;
					WritablePacket *data_pack = Packet::make(NULL,packet->length() - sizeof(MyTCPHeader));
					memcpy(data_pack->data(), (char*)(packet->data()) + sizeof(MyTCPHeader), packet->length() - sizeof(MyTCPHeader));
					output(1).push(data_pack);
					//send ack
					WritablePacket *ack_pack = Packet::make(NULL,sizeof(MyTCPHeader));
					memset(ack_pack->data(),0,ack_pack->length());
					((MyTCPHeader*)ack_pack->data())->type = ACK;
					((MyTCPHeader*)ack_pack->data())->ack_num = _control_block.nxt_ack;
					((MyTCPHeader*)ack_pack->data())->source = _my_port_number;
					((MyTCPHeader*)ack_pack->data())->destination = _control_block.tcpport;
					((MyTCPHeader*)ack_pack->data())->source_ip = _my_ipaddr;
					((MyTCPHeader*)ack_pack->data())->dest_ip = (sipaddr);
					set_resend_timer(&_ack_timer, TimerInfo(ESTAB,_control_block.nxt_seq,_control_block.nxt_ack,ack_pack));
					output(0).push(ack_pack);
					break;
					
				}
			}
				
			
			
			case FIN_WAIT:
			{
				//waiting for ack, if recieved ack or time-out, change to CLOSED state
				if(ptype != ACK || ack != _control_block.nxt_seq)
				{
					fprintf(stderr, "tcp : Wrong packet, tcp client is waiting for ACK of FIN.\n");
					packet->kill();
					break;
				}
				else
				{
					fprintf(stderr, "tcp : recieved ACK for FIN from %d %x\n", sport, sipaddr);
					cancel_timer(&_fin_timer, _fin_tif);
					_control_block.nxt_ack = _control_block.nxt_seq = 0;
					_control_block.state = CLOSED;
					break;
				}
				
			}
			}
		}
		
		else if(tcpEntity_type == 1)//server
		{
			fprintf(stderr, "TCP server recieved packet from ip, type:%d, seq:%d, ack:%d, sport:%d, dport:%d, sipaddr:%x\n",
		      ptype, seq, ack, sport, dport, sipaddr);
			fprintf(stderr, "TCP server current state is %d\n", _control_block.state);
			switch(_control_block.state)
			{
			case CLOSED:
			{
				//should not recieve anything from ip when closed
				fprintf(stderr, "tcp: Wrong packet, tcp server is now closed.\n");
				packet->kill();
				break;
			}
			case SYN_SENT:
			{
				//expecting SYN_ACK
				if(ptype != SYN+ACK || sipaddr != _control_block.ipaddr)
				{
					fprintf(stderr, "tco:Wrong packet, tcp server is waiting for SYNACK.\n");
					packet->kill();
					break;
				}
				//send ACK+DATA
				fprintf(stderr, "recieved SYN+ACK from %d %x\n", sport, sipaddr);
				cancel_timer(&_ack_timer, _ack_tif);
				fprintf(stderr, "sending data!\n");
				uint32_t tlen = _control_block.pbuff.front()->length();
				
				fprintf(stderr, "sending data: %s length: %d\n", _control_block.pbuff.front()->data(), tlen);
				WritablePacket *ack_pack = Packet::make(NULL,sizeof(MyTCPHeader)+tlen);
				((MyTCPHeader*)ack_pack->data())->type = ACK+DATA;
				((MyTCPHeader*)ack_pack->data())->sequence = _control_block.nxt_seq;
				((MyTCPHeader*)ack_pack->data())->ack_num = _control_block.nxt_ack;
				((MyTCPHeader*)ack_pack->data())->source = _my_port_number;
				((MyTCPHeader*)ack_pack->data())->destination = _control_block.tcpport;
				((MyTCPHeader*)ack_pack->data())->size = tlen;
				((MyTCPHeader*)ack_pack->data())->source_ip = _my_ipaddr;
				((MyTCPHeader*)ack_pack->data())->dest_ip = sipaddr;
				memcpy(ack_pack->data()+sizeof(MyTCPHeader), _control_block.pbuff.front()->data(), tlen);
				_control_block.nxt_seq = (_control_block.nxt_seq + 1) % 2;
				_control_block.pbuff.pop_front();
				set_resend_timer(&_ack_timer, TimerInfo(ESTAB,_control_block.nxt_seq,_control_block.nxt_ack,ack_pack));
				_control_block.state = ESTAB;
				output(0).push(ack_pack);
				
				break;
			}
			
			
			case ESTAB:
			{
				fprintf(stderr, "link established to %d %x\n", sport, sipaddr);
				//Expecting ack, set the seq.
				if(sipaddr != _control_block.ipaddr || sport != _control_block.tcpport)
				{
					fprintf(stderr, "tcp : Recieved packet from an unconnected socket while busy.\n");
					packet->kill();
					break;
				}
				
				if(ptype != ACK || ack != _control_block.nxt_seq) 
				{
					fprintf(stderr, "tcp : wrong packet, tcp server is waiting for ACK.\n");
					packet->kill();
					break;
				}
				
				fprintf(stderr, "recieved ack  from %d %x ,Data is being transfered\n", sport, sipaddr);
				
				if(_control_block.pbuff.empty())
				{
					WritablePacket *fin_pack = Packet::make(NULL,sizeof(MyTCPHeader));//leaving the space for MyIPHeader
					memset(fin_pack->data(),0,fin_pack->length());
					((MyTCPHeader*)fin_pack->data())->type = FIN;
					((MyTCPHeader*)fin_pack->data())->source = (_my_port_number);
					((MyTCPHeader*)fin_pack->data())->destination = (_control_block.tcpport);
					((MyTCPHeader*)fin_pack->data())->source_ip = (_my_ipaddr);
					((MyTCPHeader*)fin_pack->data())->dest_ip = (sipaddr);
					set_resend_timer(&_fin_timer, TimerInfo(ESTAB,_control_block.nxt_seq,_control_block.nxt_ack,fin_pack));
					_control_block.state = FIN_SENT;
					output(0).push(fin_pack);
					break;
				}
				
				cancel_timer(&_ack_timer, _ack_tif);
				uint32_t tlen = _control_block.pbuff.front()->length();
				WritablePacket *ack_pack = Packet::make(NULL,sizeof(MyTCPHeader)+tlen);// add the header on
				((MyTCPHeader*)ack_pack->data())->type = DATA;
				((MyTCPHeader*)ack_pack->data())->sequence = _control_block.nxt_seq;
				((MyTCPHeader*)ack_pack->data())->ack_num = _control_block.nxt_ack;
				((MyTCPHeader*)ack_pack->data())->source = _my_port_number;
				((MyTCPHeader*)ack_pack->data())->destination = _control_block.tcpport;
				((MyTCPHeader*)ack_pack->data())->size = tlen;
				((MyTCPHeader*)ack_pack->data())->source_ip = _my_ipaddr;
				((MyTCPHeader*)ack_pack->data())->dest_ip = sipaddr;
				memcpy(ack_pack->data()+sizeof(MyTCPHeader), _control_block.pbuff.front()->data(), tlen);
				_control_block.nxt_seq = (_control_block.nxt_seq + 1) % 2;
				_control_block.pbuff.pop_front();
				set_resend_timer(&_ack_timer, TimerInfo(ESTAB,_control_block.nxt_seq,_control_block.nxt_ack,ack_pack));
				output(0).push(ack_pack);
				
			}
			
			case FIN_SENT:
			{
				//Waiting for ack of fin. Change to LISTEN when ack recieved or time-out.
				if(sipaddr != _control_block.ipaddr || sport != _control_block.tcpport)
				{
					fprintf(stderr, "tcp : Recieved packet from an unconnected socket while busy.\n");
					packet->kill();
					break;
				}
				
				if(ptype != ACK+FIN) 
				{
					fprintf(stderr, "tcp : Wrong packet, tcp server is waiting for ACK+FIN.\n");
					packet->kill();
					break;
				}
				fprintf(stderr, "recieved ack for fin from %d %x\n", sport, sipaddr);
				cancel_timer(&_fin_timer, _fin_tif);
				_control_block.nxt_ack = _control_block.nxt_seq = 0;
				_control_block.state = CLOSED;
				break;
				
			}
			}
		}
	}
}

void tcpEntity::run_timer(Timer *timer) {
	if(timer == &_debug_timer)
	{
		fprintf(stderr, "current stat: %d", _control_block.state);
		_debug_timer.schedule_after_msec(2000);
	}
    if(timer == &_fin_timer)
	{
		_control_block.nxt_ack = _control_block.nxt_seq = 0;
		_control_block.state = CLOSED;
	}
	else if(timer == &_ack_timer)
	{
		output(0).push(_ack_tif.pack);
	}
}
CLICK_ENDDECLS
EXPORT_ELEMENT(tcpEntity)
