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
	nxt_ack = 0;
	nxt_seq = 0;
	tcpport = 0;
	state = CLOSED;
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
			//fprintf(stderr, "Wrong input, tcp client could not send data actively.\n");
			//fprintf(stderr, "Type is %d My port num is %d, MY ipaddr is %x, Time_out is %d\n",tcpEntity_type, _my_port_number, _my_ipaddr, _time_out);
			packet->kill();
			// should not recieve anything from client
		}
		
		else if(tcpEntity_type == 1)//tcp as server role
		{
			
			uint16_t dstport = ((MyClientHeader* )packet->data())->dst_port;
			uint32_t dstip = ((MyClientHeader* )packet->data())->dst_ip;
			//fprintf(stderr, "TCP server recieved packet from client, dstport:%d, dstip:%x\n",dstport,dstip);
			WritablePacket* wp = Packet::make(NULL,packet->length()-sizeof(MyClientHeader));
			memcpy(wp->data(),packet->data()+sizeof(MyClientHeader),packet->length());
			pbuff.push_back(wp);
			
			//send SYN
			//fprintf(stderr, "sending SYN to %d %x\n", dstport, dstip);
			ipaddr = dstip;
			tcpport = dstport;
			
			WritablePacket* syn_pack = Packet::make(NULL,sizeof(MyTCPHeader));
			((MyTCPHeader*)syn_pack->data())->type = SYN;
			((MyTCPHeader*)syn_pack->data())->ack_num = nxt_ack;
			((MyTCPHeader*)syn_pack->data())->sequence = nxt_seq;
			((MyTCPHeader*)syn_pack->data())->source = _my_port_number;
			((MyTCPHeader*)syn_pack->data())->destination = dstport;
			((MyTCPHeader*)syn_pack->data())->source_ip = _my_ipaddr;
			((MyTCPHeader*)syn_pack->data())->dest_ip = dstip;
			set_resend_timer(&_ack_timer, TimerInfo(ESTAB,nxt_seq,nxt_ack,syn_pack));
			state = SYN_SENT;
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
			//fprintf(stderr, "TCP client recieved packet from ip, type:%d, seq:%d, ack:%d, sport:%d, dport:%d, sipaddr:%x\n",ptype, seq, ack, sport, dport, sipaddr);
			//fprintf(stderr, "\nTCP client current state is %d\n\n", state);
			switch(state)
			{
				
			case CLOSED:
			{
				//Expecting SYN request. When recieved, send SYN_ACK to ip and set the <ip,port> tuple in tcb.
				if(ptype != SYN) // assuming that SYN's seq is always 0
				{
					//fprintf(stderr, "tcp : wrong packet, tcp client is waiting for SYN.\n");
					packet->kill();
					break;
				}
				
				//fprintf(stderr, "recieved SYN from %d %x\n", sport, sipaddr);
				packet->kill();
				ipaddr = sipaddr;
				tcpport = sport;
				WritablePacket *ack_pack = Packet::make(NULL, sizeof(MyTCPHeader));
				memset(ack_pack->data(),0,ack_pack->length());
				((MyTCPHeader*)ack_pack->data())->type = SYN+ACK;
				((MyTCPHeader*)ack_pack->data())->ack_num = (nxt_ack);
				((MyTCPHeader*)ack_pack->data())->source = (_my_port_number);
				((MyTCPHeader*)ack_pack->data())->destination = (tcpport);
				((MyTCPHeader*)ack_pack->data())->source_ip = (_my_ipaddr);
				((MyTCPHeader*)ack_pack->data())->dest_ip = (sipaddr);
				set_resend_timer(&_ack_timer, TimerInfo(ESTAB,nxt_seq,nxt_ack,ack_pack));
				state = SYN_WAIT;
				output(0).push(ack_pack);
				break;
			}
			
			case SYN_WAIT:
			{
				//Waiting for ACK. When recieved, send ack to ip, change to ESTAB state.
				if(sipaddr != ipaddr || sport != tcpport)
				{
					//fprintf(stderr, "tcp : Recieved packet from an unconnected socket while busy.\n");
					packet->kill();
					break;
				}
				if(ptype != ACK) 
				{
					//fprintf(stderr, "tcp : Wrong packet, tcp client is waiting for ACK.\n");
					//fprintf(stderr, "Curennt packet from ip is, type:%d, seq:%d, ack:%d, sport:%d, dport:%d, sipaddr:%x\n", ptype, seq, ack, sport, dport, sipaddr);
					packet->kill();
					break;
				}
				
				//fprintf(stderr, "recieved ACK from %d %x\n", sport, sipaddr);
				
				// change state to ESTAB
				state = ESTAB;
				WritablePacket *ack_pack = Packet::make(NULL,sizeof(MyTCPHeader));
				memset(ack_pack->data(),0,ack_pack->length());
				((MyTCPHeader*)ack_pack->data())->type = ACK;
				((MyTCPHeader*)ack_pack->data())->ack_num = nxt_ack;
				((MyTCPHeader*)ack_pack->data())->source = _my_port_number;
				((MyTCPHeader*)ack_pack->data())->destination = tcpport;
				((MyTCPHeader*)ack_pack->data())->source_ip = _my_ipaddr;
				((MyTCPHeader*)ack_pack->data())->dest_ip = (sipaddr);
				set_resend_timer(&_ack_timer, TimerInfo(ESTAB,nxt_seq,nxt_ack,ack_pack));
				output(0).push(ack_pack);
				break;	
				
				
				
			}
			case ESTAB:
			{
				//fprintf(stderr, "TCP client: connected to %d %x\n", sport, sipaddr);
				//check the seq, send ack to ip and pass data to client if correct.
				//if recieve fin, send ack+fin to ip, change to FIN_WAIT state
				if(ptype != FIN && ptype != DATA)
				{
					//fprintf(stderr, "TCP server : wrong packet, tcp client is waiting for DATA or FIN.\n");
					//fprintf(stderr, "---recieved packet: type:%d, seq:%d, ack:%d, sport:%d, dport:%d, sipaddr:%x\n", ptype, seq, ack, sport, dport, sipaddr);
					packet->kill();
					break;
				}
				
				if(seq != nxt_ack)
				{
					//fprintf(stderr, "TCP server : wrong seqnum %d, should be %d\n", seq, nxt_ack);
					packet->kill();
					break;
				}
				
				if(ptype == FIN)
				{
					//fprintf(stderr, "tcp : recieved FIN from %d %x\n", sport, sipaddr);
					nxt_ack = (nxt_ack + 1) % 2;
					WritablePacket *ack_pack = Packet::make(NULL,sizeof(MyTCPHeader));
					memset(ack_pack->data(),0,ack_pack->length());
					((MyTCPHeader*)ack_pack->data())->type = FIN+ACK;
					((MyTCPHeader*)ack_pack->data())->ack_num = nxt_ack;
					((MyTCPHeader*)ack_pack->data())->sequence = nxt_seq;
					((MyTCPHeader*)ack_pack->data())->source = _my_port_number;
					((MyTCPHeader*)ack_pack->data())->destination = tcpport;
					((MyTCPHeader*)ack_pack->data())->source_ip = _my_ipaddr;
					((MyTCPHeader*)ack_pack->data())->dest_ip = sipaddr;
					set_resend_timer(&_ack_timer, _ack_tif=TimerInfo(ESTAB,nxt_seq,nxt_ack,ack_pack));
					set_resend_timer(&_fin_timer, _fin_tif=TimerInfo(ESTAB,nxt_seq,nxt_ack,ack_pack));
					nxt_seq = (nxt_seq + 1) % 2;
					state = FIN_WAIT;
					output(0).push(ack_pack);
					break;
				}
				
				
				else if(ptype == DATA)
				{
					//fprintf(stderr, "TCP client : recieved DATA from %d %x\n", sport, sipaddr);
					//send to client
					cancel_timer(&_ack_timer, _ack_tif);
					nxt_ack = (nxt_ack + 1) % 2;
					WritablePacket *data_pack = Packet::make(NULL,packet->length() - sizeof(MyTCPHeader));
					memcpy(data_pack->data(), (char*)(packet->data()) + sizeof(MyTCPHeader), packet->length() - sizeof(MyTCPHeader));
					output(1).push(data_pack);
					//send ack
					WritablePacket *ack_pack = Packet::make(NULL,sizeof(MyTCPHeader));
					memset(ack_pack->data(),0,ack_pack->length());
					((MyTCPHeader*)ack_pack->data())->type = ACK;
					((MyTCPHeader*)ack_pack->data())->ack_num = nxt_ack;
					((MyTCPHeader*)ack_pack->data())->source = _my_port_number;
					((MyTCPHeader*)ack_pack->data())->destination = tcpport;
					((MyTCPHeader*)ack_pack->data())->source_ip = _my_ipaddr;
					((MyTCPHeader*)ack_pack->data())->dest_ip = (sipaddr);
					set_resend_timer(&_ack_timer, TimerInfo(ESTAB,nxt_seq,nxt_ack,ack_pack));
					output(0).push(ack_pack);
					break;
					
				}
			}
				
			
			
			case FIN_WAIT:
			{
				//waiting for ack, if recieved ack or time-out, change to CLOSED state
				if(ptype != ACK || ack != nxt_seq)
				{
					//fprintf(stderr, "TCP Client : Wrong packet, tcp client is waiting for ACK of FIN.\n");
					packet->kill();
					break;
				}
				else
				{
					//fprintf(stderr, "TCP Cclient : recieved ACK for FIN from %d %x\n", sport, sipaddr);
					cancel_timer(&_fin_timer, _fin_tif);
					cancel_timer(&_ack_timer, _ack_tif);
					nxt_ack = nxt_seq = 0;
					ipaddr=tcpport=0;
					state = CLOSED;
					break;
				}
				
			}
			}
		}
		
		else if(tcpEntity_type == 1)//server
		{
			//fprintf(stderr, "TCP server recieved packet from ip, type:%d, seq:%d, ack:%d, sport:%d, dport:%d, sipaddr:%x\n", ptype, seq, ack, sport, dport, sipaddr);
			//fprintf(stderr, "TCP server current state is %d\n", state);
			switch(state)
			{
			case CLOSED:
			{
				//should not recieve anything from ip when closed
				//fprintf(stderr, "TCo server :  Wrong packet, tcp server is now closed.\n");
				packet->kill();
				break;
			}
			case SYN_SENT:
			{
				//expecting SYN_ACK
				if(ptype != SYN+ACK || sipaddr != ipaddr)
				{
					//fprintf(stderr, "TCP server : Wrong packet, tcp server is waiting for SYNACK.\n");
					packet->kill();
					break;
				}
				//send ACK
				//fprintf(stderr, "recieved SYN+ACK from %d %x\n", sport, sipaddr);
				cancel_timer(&_ack_timer, _ack_tif);
				/*
				//fprintf(stderr, "sending data!\n");
				uint32_t tlen = pbuff.front()->length();
				
				//fprintf(stderr, "sending data: %s length: %d\n", pbuff.front()->data(), tlen);
				WritablePacket *ack_pack = Packet::make(NULL,sizeof(MyTCPHeader)+tlen);
				((MyTCPHeader*)ack_pack->data())->type = ACK+DATA;
				((MyTCPHeader*)ack_pack->data())->sequence = nxt_seq;
				((MyTCPHeader*)ack_pack->data())->ack_num = nxt_ack;
				((MyTCPHeader*)ack_pack->data())->source = _my_port_number;
				((MyTCPHeader*)ack_pack->data())->destination = tcpport;
				((MyTCPHeader*)ack_pack->data())->size = tlen;
				((MyTCPHeader*)ack_pack->data())->source_ip = _my_ipaddr;
				((MyTCPHeader*)ack_pack->data())->dest_ip = sipaddr;
				memcpy(ack_pack->data()+sizeof(MyTCPHeader), pbuff.front()->data(), tlen);
				nxt_seq = (nxt_seq + 1) % 2;
				pbuff.pop_front();
				set_resend_timer(&_ack_timer, TimerInfo(ESTAB,nxt_seq,nxt_ack,ack_pack));
				state = ESTAB;
				output(0).push(ack_pack);
				*/
				WritablePacket *ack_pack = Packet::make(NULL,sizeof(MyTCPHeader));
				((MyTCPHeader*)ack_pack->data())->type = ACK;
				((MyTCPHeader*)ack_pack->data())->sequence = nxt_seq;
				((MyTCPHeader*)ack_pack->data())->ack_num = nxt_ack;
				((MyTCPHeader*)ack_pack->data())->source = _my_port_number;
				((MyTCPHeader*)ack_pack->data())->destination = tcpport;
				((MyTCPHeader*)ack_pack->data())->size = 0;
				((MyTCPHeader*)ack_pack->data())->source_ip = _my_ipaddr;
				((MyTCPHeader*)ack_pack->data())->dest_ip = sipaddr;
				state=ESTAB;
				output(0).push(ack_pack);
				break;
			}
			
			
			case ESTAB:
			{
				//fprintf(stderr, "TCP server: connected to %d %x\n", sport, sipaddr);
				//Expecting ack, set the seq.
				if(sipaddr != ipaddr || sport != tcpport)
				{
					//fprintf(stderr, "TCP server : Recieved packet from an unconnected socket while busy.\n");
					packet->kill();
					break;
				}
				
				if(ptype != ACK || ack != nxt_seq) 
				{
					//fprintf(stderr, "TCP server : wrong packet, tcp server is waiting for ACK.\n");
					packet->kill();
					break;
				}
				
				
				if(pbuff.empty())
				{
					WritablePacket *fin_pack = Packet::make(NULL,sizeof(MyTCPHeader));//leaving the space for MyIPHeader
					memset(fin_pack->data(),0,fin_pack->length());
					((MyTCPHeader*)fin_pack->data())->type = FIN;
					((MyTCPHeader*)fin_pack->data())->sequence = nxt_seq;
					((MyTCPHeader*)fin_pack->data())->source = _my_port_number;
					((MyTCPHeader*)fin_pack->data())->destination = tcpport;
					((MyTCPHeader*)fin_pack->data())->source_ip = _my_ipaddr;
					((MyTCPHeader*)fin_pack->data())->dest_ip = sipaddr;
					set_resend_timer(&_ack_timer, _ack_tif=TimerInfo(ESTAB,nxt_seq,nxt_ack,fin_pack));
					state = FIN_SENT;
					output(0).push(fin_pack);
					break;
				}
				
				cancel_timer(&_ack_timer, _ack_tif);
				uint32_t tlen = pbuff.front()->length();
				WritablePacket *ack_pack = Packet::make(NULL,sizeof(MyTCPHeader)+tlen);// add the header on
				((MyTCPHeader*)ack_pack->data())->type = DATA;
				((MyTCPHeader*)ack_pack->data())->sequence = nxt_seq;
				((MyTCPHeader*)ack_pack->data())->ack_num = nxt_ack;
				((MyTCPHeader*)ack_pack->data())->source = _my_port_number;
				((MyTCPHeader*)ack_pack->data())->destination = tcpport;
				((MyTCPHeader*)ack_pack->data())->size = tlen;
				((MyTCPHeader*)ack_pack->data())->source_ip = _my_ipaddr;
				((MyTCPHeader*)ack_pack->data())->dest_ip = sipaddr;
				memcpy(ack_pack->data()+sizeof(MyTCPHeader), pbuff.front()->data(), tlen);
				nxt_seq = (nxt_seq + 1) % 2;
				pbuff.pop_front();
				set_resend_timer(&_ack_timer, _ack_tif=TimerInfo(ESTAB,nxt_seq,nxt_ack,ack_pack));
				output(0).push(ack_pack);
				break;
			}
			
			case FIN_SENT:
			{
				//Waiting for ack of fin. Change to LISTEN when ack recieved or time-out.
				if(sipaddr != ipaddr || sport != tcpport)
				{
					//fprintf(stderr, "TCP server : Recieved packet from an unconnected socket while busy.\n Server has connected to %d, %x", tcpport, ipaddr);
					
					//fprintf(stderr, "---recieved packet: type:%d, seq:%d, ack:%d, sport:%d, dport:%d, sipaddr:%x\n", ptype, seq, ack, sport, dport, sipaddr);
					packet->kill();
					break;
				}
				
				if(ptype != ACK+FIN) 
				{
					//fprintf(stderr, "TCP server : Wrong packet, tcp server is waiting for ACK+FIN.\n");
					//fprintf(stderr, "---recieved packet: type:%d, seq:%d, ack:%d, sport:%d, dport:%d, sipaddr:%x\n", ptype, seq, ack, sport, dport, sipaddr);
					packet->kill();
					break;
				}
				//fprintf(stderr, "TCP server : recieved ack for fin from %d %x\n", sport, sipaddr);
				cancel_timer(&_ack_timer, _ack_tif);
				(nxt_ack = nxt_ack + 1) % 2;
				WritablePacket *ack_pack = Packet::make(NULL,sizeof(MyTCPHeader));
				((MyTCPHeader*)ack_pack->data())->type = ACK;
				((MyTCPHeader*)ack_pack->data())->sequence = nxt_seq;
				((MyTCPHeader*)ack_pack->data())->ack_num = nxt_ack;
				((MyTCPHeader*)ack_pack->data())->source = _my_port_number;
				((MyTCPHeader*)ack_pack->data())->destination = tcpport;
				((MyTCPHeader*)ack_pack->data())->size = 0;
				((MyTCPHeader*)ack_pack->data())->source_ip = _my_ipaddr;
				((MyTCPHeader*)ack_pack->data())->dest_ip = sipaddr;
				nxt_ack = nxt_seq = 0;
				ipaddr=tcpport=0;
				state = CLOSED;
				output(0).push(ack_pack);
				break;
				
			}
			}
		}
	}
}

void tcpEntity::run_timer(Timer *timer) {
	if(timer == &_debug_timer)
	{
		//fprintf(stderr, "current stat: %d", state);
		_debug_timer.schedule_after_msec(2000);
	}
    if(timer == &_fin_timer)
	{
		//fprintf(stderr, "finTimer_fired!\n");
		_ack_timer.unschedule();
		nxt_ack = nxt_seq = 0;
		state = CLOSED;
	}
	else if(timer == &_ack_timer)
	{
		//fprintf(stderr, "ackTimer fired!\n");
		output(0).push(_ack_tif.pack);
		set_resend_timer(&_ack_timer, _ack_tif);
	}
}
CLICK_ENDDECLS
EXPORT_ELEMENT(tcpEntity)
