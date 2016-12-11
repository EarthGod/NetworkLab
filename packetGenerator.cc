#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "packets.hh"
#include "packetGenerator.hh"

CLICK_DECLS 

PacketGenerator::PacketGenerator() : _timer(this) {
}

PacketGenerator::~PacketGenerator(){
}

int PacketGenerator::configure(Vector<String> &conf, ErrorHandler *errh)
{
	if (cp_va_kparse(conf, this, errh,
                  "DST_PORT", cpkP+cpkM, cpTCPPort, &_dst_port,
				  "DST_IP", cpkP+cpkM, cpIPAddress, &_dst_ip,
                  
                  cpEnd) < 0) {
    return -1;
	}
	return 0;

}

int PacketGenerator::initialize(ErrorHandler *errh){
    _timer.initialize(this);
    _timer.schedule_now();
    return 0;
}

void PacketGenerator::run_timer(Timer *timer) {
    assert(timer == &_timer);
    WritablePacket *packet = Packet::make(0,0,5+sizeof(MyClientHeader),0);
	((MyClientHeader*)packet)->dst_port = _dst_port;
	((MyClientHeader*)packet)->dst_ip = _dst_ip;
	memcpy(packet+sizeof(MyClientHeader), "hello", 5);
    output(0).push(packet);
    _timer.reschedule_after_sec(2);
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(PacketGenerator)
