#include <click/config.h>
#include <click/args.hh>
#include <click/error.hh>
#include "activeclient.hh"
#include <fstream>
#include <iostream>
#include <string>

CLICK_DECLS

int ActiveClient::configure(Vector<String> &conf, ErrorHandler *errh)
{
	return Args(conf, this, errh).read("FILENAME", fileName).complete();
}

void ActiveClient::push(int port, Packet *packet)
{
	std::string name(fileName.data(), fileName.length());
	std::ifstream is(name.c_str(), std::ifstream::binary);
	std::cout << "ActiveClient: reading " << name << std::endl;
	is.seekg(0, is.end);
	uint32_t length = is.tellg();
	is.seekg(0, is.beg);
	WritablePacket *outPacket = Packet::make(length);
	is.read((char *)outPacket->data(), length);
	output(0).push(outPacket);
	packet->kill();
}

CLICK_ENDDECLS
EXPORT_ELEMENT(ActiveClient)
