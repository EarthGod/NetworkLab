#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "packets.hh"
#include "simipgen.hh"

CLICK_DECLS 

simipgen::simipgen() : _timer(this) {
}

simipgen::~simipgen(){
}

int simipgen::configure(Vector<String> &conf, ErrorHandler *errh)
{
	if (cp_va_kparse(conf, this, errh,
                  "SRC_IP", cpkP+cpkM, cpIPAddress, &_src_ip,
				  "DST_IP", cpkP+cpkM, cpIPAddress, &_dst_ip,
                  cpEnd) < 0) {
    return -1;
	}
	return 0;

}

int simipgen::initialize(ErrorHandler *errh){
    _timer.initialize(this);
    _timer.schedule_now();
    return 0;
}

void simipgen::run_timer(Timer *timer) {
    assert(timer == &_timer);
    MyTCPHeader* header = new MyTCPHeader();
    header->type = DATA;
    header->source_ip = _src_ip;
    header->dest_ip = _dst_ip;
    header->size = sizeof(struct MyTCPHeader) + 5;
    WritablePacket *packet = Packet::make(header,sizeof(struct MyTCPHeader)+5);
    char* loc2write=(char*)(packet->data()+sizeof(struct MyTCPHeader));
    memcpy(loc2write, "hello", 5);
    _timer.schedule_after_sec(1);
    output(0).push(packet);
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(simipgen)
