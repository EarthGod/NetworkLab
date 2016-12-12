#ifndef CLICK_PASSIVECLIENT_HH
#define CLICK_PASSIVECLIENT_HH

#include <click/element.hh>

CLICK_DECLS

class PassiveClient : public Element {

public:
	const char *class_name() const { return "PassiveClient";}
	const char *port_count() const { return "1/0"; }
	const char *process() const { return PUSH; }
	
	int configure(Vector<String> &, ErrorHandler *);
	void push(int, Packet *);
	
private:
	String fileName;
};

CLICK_ENDDECLS

#endif
