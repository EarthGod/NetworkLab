#ifndef CLICK_TCPENTITY_HH 
#define CLICK_TCPENTITY_HH 
#include <click/element.hh>
#include <click/timer.hh>
#include <deque>
#include "packets.hh"
using std::deque;
CLICK_DECLS



typedef enum{
	CLOSED,
	LISTEN,
	SYN_SENT,
	SYN_WAIT,
	ESTAB,
	FIN_SENT
	FIN_WAIT,
} stat;

struct TCB{
	uint32_t ipaddr;
	uint16_t tcpport;
	int nxt_ack; //0 or 1
	int nxt_seq; //0 or 1
	stat state;
	deque<WritablePacket*> pbuff;
};

struct TimerInfo{
	stat tstat;
	uint16_t tseq;
	uint16_t tack;
	WritablePacket* pack;
	Timerinfo(){
		tstat=CLOSED;tseq=tack=0;
	}
	TimerInfo(stat _tstat, uint16_t _tseq, uint16_t _tack, WritablePacket* _pack){
		tstat = _tstat;tseq = _tseq;tack = _tack;pack = _pack;
	}
};

class tcp_entity : public Element {
    public:
        tcp_entity();
        ~tcp_entity();
        const char *class_name() const { return "tcp_entity";}
        const char *port_count() const { return "2-/2-";}
        const char *processing() const { return PUSH; }
		int configure(Vector<String> &conf, ErrorHandler *errh);
		
        
		void push(int port, Packet *packet);
        int initialize(ErrorHandler*);
		void set_resend_timer(Timer* _timer, TimerInfo tif);
		void run_timer(Timer *timer);
		void cancel_timer(Timer* _timer, TimerInfo tif);
    private: 
        Timer _fin_timer;
		Timer _ack_timer;
		TimerInfo _fin_tif;
		TimerInfo _ack_tif;
		TCB _control_block;
		int tcp_entity_type; //0 for client, 1 for server
		uint32_t _my_ipaddr;
		uint32_t _time_out;//msec
		uint16_t _my_port_number;
		
}; 

CLICK_ENDDECLS
#endif 
