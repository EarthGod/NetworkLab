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

int PacketGenerator::initialize(ErrorHandler *errh){
    _timer.initialize(this);
    _timer.schedule_now();
    return 0;
}

void PacketGenerator::run_timer(Timer *timer) {
    assert(timer == &_timer);
    WritablePacket *packet = Packet::make(0,0,5,0);
	memcpy(packet, "hello", 5);
    output(0).push(packet);
    _timer.reschedule_after_sec(2);
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(PacketGenerator)
