#ifndef CLICK_BASICCLIENT_HH 
#define CLICK_BASICCLIENT_HH 
#include <click/element.hh>
#include <click/timer.hh>


using std::vector;
CLICK_DECLS


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
		
    private: 
        Timer _timer;
		TCB _control_block;
		int tcp_entity_type; //0 for client, 1 for server
		int ipaddr;
		uint32_t _time_out;
		uint16_t _my_port_number;
		
}; 

CLICK_ENDDECLS
#endif 
