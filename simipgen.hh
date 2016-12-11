#ifndef CLICK_SIMIPGEN_HH 
#define CLICK_SIMIPGEN_HH 
#include <click/element.hh>
#include <click/timer.hh>

CLICK_DECLS

class simipgen : public Element {
    public:
        simipgen();
        ~simipgen();
        const char *class_name() const { return "simipgen";}
        const char *port_count() const { return "0/1";}
        const char *processing() const { return PUSH; }
		
        void run_timer(Timer*);
        int initialize(ErrorHandler*);
		int configure(Vector<String> &conf, ErrorHandler *errh);
    private: 
        Timer _timer;
        uint32_t _src_ip;
		uint32_t _dst_ip;
}; 

CLICK_ENDDECLS
#endif 
