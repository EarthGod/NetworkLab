#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/packet.hh>
#include "packet.hh"
#include "exercise3receiver.hh"

CLICK_DECLS 
PacketReceiver::PacketReceiver(){}
PacketReceiver::~PacketReceiver(){}

void PacketReceiver::push(int, Packet *p) {
    struct PacketHeader *format = (struct PacketHeader*) p->data();
    if (format->type == 1) {
		output(1).push(p);
    }
    else {
		output(0).push(p);
    }    
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(PacketReceiver)
