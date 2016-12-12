#ifndef CLICK_PACKETGENERATOR_HH 
#define CLICK_PACKETGENERATOR_HH 
#include <click/element.hh>
#include <click/timer.hh>

CLICK_DECLS

class PacketGenerator : public Element {
    public:
        PacketGenerator();
        ~PacketGenerator();
        const char *class_name() const { return "PacketGenerator";}
        const char *port_count() const { return "0/1";}
        const char *processing() const { return PUSH; }
		
        void run_timer(Timer*);
        int initialize(ErrorHandler*);
		int configure(Vector<String> &conf, ErrorHandler *errh);
    private: 
        Timer _timer;
		uint16_t seq;
		uint16_t _dst_port; 
		uint32_t _dst_ip;
}; 

CLICK_ENDDECLS
#endif 
