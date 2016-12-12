#ifndef CLICK_ACTIVECLIENT_HH
#define CLICK_ACTIVECLIENT_HH

#include <click/element.hh>

CLICK_DECLS

class ActiveClient : public Element {

public:
	const char *class_name() const { return "ActiveClient";}
	const char *port_count() const { return "1/1"; }
	const char *process() const { return PUSH; }
	
	int configure(Vector<String> &, ErrorHandler *);
	void push(int, Packet *);
	
private:
	String fileName;
};

CLICK_ENDDECLS

#endif
