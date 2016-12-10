#ifndef CLICK_IP_HH 
#define CLICK_IP_HH 
#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashtable.hh>
#include <map>
#include "packets.hh"

CLICK_DECLS

const int maxn = 100;
class ip : public Element {
    public:
        ip();
        ~ip();
        const char *class_name() const { return "ip";}
        const char *port_count() const { return "1-/1-";}
        const char *processing() const { return PUSH; }
		
		void push(int port, Packet *packet);
        int initialize(ErrorHandler*);
		
	private:
        uint32_t ip_addr;
		map<uint32_t, int> ip_port_table;
        map<int, uint32_t> port_ip_table;
        int adjacentMatrix[maxn][maxn];
        int dist[maxn];
        int path[maxn];
        Timer timer_update;
}; 

CLICK_ENDDECLS
#endif 
