#include <click/config.h>
#include <click/args.hh>
#include <click/error.hh>
#include "passiveclient.hh"
#include <fstream>
#include <iostream>
#include <string>

CLICK_DECLS

int PassiveClient::configure(Vector<String> &conf, ErrorHandler *errh)
{
	return Args(conf, this, errh).read("FILENAME", fileName).complete();
}

void PassiveClient::push(int port, Packet *packet)
{
	std::string name(fileName.data(), fileName.length());
	std::ofstream os(name.c_str(), std::ofstream::binary);
	os.write((char *)packet->data(), packet->length());
	os.close();
	std::cout << "PassiveClient: file saved" << std::endl;
	packet->kill();
}

CLICK_ENDDECLS
EXPORT_ELEMENT(PassiveClient)
