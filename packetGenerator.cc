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
	seq = 1;
    return 0;
}

void PacketGenerator::run_timer(Timer *timer) {
    assert(timer == &_timer);
	seq++;
	char dat[7];
	dat[0] = 'h';dat[1]='e';dat[2]='l';dat[3]='l';dat[4]='o';
	dat[5] = seq + '0'; 
	dat[6] = '\0';
    WritablePacket *packet = Packet::make(NULL,sizeof(MyClientHeader)+7);
	memcpy(packet->data()+sizeof(MyClientHeader), dat, 7);
	MyClientHeader* th = (MyClientHeader*)packet->data();
	th->dst_port = _dst_port;
	th->dst_ip = _dst_ip;
    output(0).push(packet);
    _timer.reschedule_after_sec(2);
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(PacketGenerator)
