#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "exercise3source.hh" 
#include "packet.hh"

CLICK_DECLS 

PacketGenerator::PacketGenerator() : _timer(this) {
	seq = 0;
}

PacketGenerator::~PacketGenerator(){
	
}

int PacketGenerator::initialize(ErrorHandler *errh){
    _timer.initialize(this);
    _timer.schedule_now();
    return 0;
}

void PacketGenerator::run_timer(Timer *timer) {
	seq++;
    assert(timer == &_timer);
    WritablePacket *packet = Packet::make(0,0,sizeof(struct PacketHeader)+5, 0);
    memset(packet->data(),0,packet->length());
    struct PacketHeader *format = (struct PacketHeader*) packet->data();
    format->type = seq%2;
    format->size = sizeof(struct PacketHeader)+5;
	char *data = (char*)(packet->data()+sizeof(struct PacketHeader));
	memcpy(data, "hello", 5);
    output(0).push(packet);
    _timer.reschedule_after_sec(3);
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(PacketGenerator)
