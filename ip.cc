#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include <map>
#include "ip.hh"
using std::map;

//NOT GOOD; SHOULD KNOW HOW MANY PORTS ARE DEFINED IN xx.click
#define PORTCOUNT (this->nports(false))
#define POINTCOUNT 50
#define INF 1000000
#define TIMERPERIOD 3

CLICK_DECLS 

ip::ip(): timer_update(this), timer_hello(this){
	for (int i = 0; i < POINTCOUNT; ++i){
		this->path[i] = -1;
		for (int j = 0; j < POINTCOUNT; ++j){
			if (i == j) this->adjacentMatrix[i][j] = 0;
			else this->adjacentMatrix[i][j] = INF;
		}
	}
}

ip::~ip(){
}

int ip::initialize(ErrorHandler *errh){
	timer_update.initialize(this);
	timer_hello.initialize(this);
	timer_update.schedule_after_sec(10);
	timer_hello.schedule_after_sec(2);
    return 0;
}

int ip::configure(Vector<String> &conf, ErrorHandler *errh) {
  if (cp_va_kparse(conf, this, errh,
                  "IP_ADDR", cpkP+cpkM, cpIPAddress, &ip_addr,
                  cpEnd) < 0) {
    return -1;
  }
  port_ip_table[0] = this->ip_addr;
  ip_port_table[this->ip_addr] = 0;
  return 0;
}

void ip::run_timer(Timer *timer){
	if(timer == &this->timer_update){
		int cnt = 0;
		for (int i = 0; i < POINTCOUNT; ++i)
			for (int j = 0; j < POINTCOUNT; ++j)
				if(this->adjacentMatrix[i][j] != INF && this->adjacentMatrix[i][j] != 0)
					cnt++;
		WritablePacket *packet = Packet::make(NULL,sizeof(struct MyIPHeader)+12*cnt);
	    struct MyIPHeader *format = (struct MyIPHeader*) packet->data();
	    format->type = BROAD;
		format->source = this->ip_addr;
		format->destination = 0xffffffff; // stands for broadcast
		format->ttl = 5; //time to live
	    format->size = sizeof(struct MyIPHeader)+12*cnt;
	    char* data = (char*)(packet->data()+sizeof(struct MyIPHeader));
	    for(int i = 0; i < POINTCOUNT; ++i){
	    	for (int j = 0; j < POINTCOUNT; ++j){
	    		if(this->adjacentMatrix[i][j] != INF){
	    			memcpy(data, &port_ip_table[i], sizeof(uint32_t));
	    			memcpy(data+4, &port_ip_table[j], sizeof(uint32_t));
	    			memcpy(data+8, &this->adjacentMatrix[i][j], sizeof(int));
	    			data += 12;
	    		}
	    	}
	    }
	    click_chatter("111-%x: BROADCASTING adjacentMatrix. PORTCOUNT: %d", this->ip_addr, PORTCOUNT);
	    for (int i = 1; i < PORTCOUNT; ++i)
	    	output(i).push(packet);
	    timer_update.schedule_after_sec(TIMERPERIOD);
	}
	else if(timer == &this->timer_hello){
		WritablePacket *packet = Packet::make(NULL, sizeof(struct MyIPHeader));
	    struct MyIPHeader *format = (struct MyIPHeader*) packet->data();
	    format->type = HELLO;
		format->source = this->ip_addr;
		format->destination = 0xffffffff; // stands for broadcast
		format->ttl = 1; //time to live
	    format->size = sizeof(struct MyIPHeader);
	    click_chatter("222-%x: BROADCASTING HELLO. PORTCOUNT: %d", this->ip_addr, PORTCOUNT);
	    for(int i = 1; i < PORTCOUNT; ++i)
	    	output(i).push(packet);
	    timer_update.schedule_after_sec(TIMERPERIOD);
	}
	else{
		assert(false);
	}
}

void ip::push(int port, Packet *packet) {
	assert(packet);
	//Assume that IP get packet from TCP through data 0;
	if(port == 0){
		struct MyTCPHeader *tcpheader = (struct MyTCPHeader*)packet->data();
		if (this->ip_port_table.find(tcpheader->dest_ip) == this->ip_port_table.end()){
			click_chatter("NOT FOUND IN ROUTER TABLE: %x; PORT: %d;", tcpheader->dest_ip, port);
			return;
		}
		click_chatter("333-%x: Received packet from %x on port %d, type: %d. packet dest: %x.", this->ip_addr, tcpheader->source_ip, port,tcpheader->type,tcpheader->dest_ip);
		//wrap
		WritablePacket* newpacket = Packet::make(NULL,sizeof(struct MyIPHeader)+(tcpheader->size));
	    struct MyIPHeader* format = (struct MyIPHeader*) newpacket->data();
	    format->type = DATA; //DATA
		format->source = tcpheader->source_ip;
		format->destination = tcpheader->dest_ip;
		format->ttl = this->dist[ip_port_table[tcpheader->dest_ip]];
		format->size = tcpheader->size + (sizeof(struct MyIPHeader));
		char *data = (char*)(newpacket->data()+sizeof(struct MyIPHeader));
		memcpy(data, tcpheader, tcpheader->size);
		//send
		click_chatter("444-%x: sending to other ip element! Port: %d", this->ip_addr, findport(ip_port_table[tcpheader->dest_ip]));
		output(findport(ip_port_table[tcpheader->dest_ip])).push(newpacket);
	}
	else{
		struct MyIPHeader *ipheader = (struct MyIPHeader*)packet->data();
		if (ipheader->ttl <= 0){
			click_chatter("Wrong TTL!!");
			return;
		}
		click_chatter("555-%x: Received packet from %x on port %d, type: %d. packet dest: %x.", this->ip_addr, ipheader->source, port, ipheader->type, ipheader->destination);
		if (ipheader->type == HELLO){
			//HELLO: return the topology this router know
			WritablePacket* newpacket = Packet::make(NULL,sizeof(struct MyIPHeader));
			struct MyIPHeader* format = (struct MyIPHeader*) newpacket->data();
	   		format->type = RESP; //RESP
			format->source = this->ip_addr;
			format->destination = ipheader->source;
			format->ttl = 1;
			format->size = sizeof(struct MyIPHeader);
			click_chatter("666-%x: sending to other ip element! Port: %d", this->ip_addr, port);
			output(port).push(newpacket);
			return;
		}
		else if (ipheader->type == RESP){
			//RESP: update the adjacent matrix
			ip_port_table[ipheader->source] = port;
			port_ip_table[port] = ipheader->source;
			adjacentMatrix[0][port] = 1;
			adjacentMatrix[port][0] = 1;
			click_chatter("777-%x: updated adjacentMatrix!", this->ip_addr);
			return;
		}
		else if (ipheader->type == BROAD){
			//BROAD: update the adjacent matrix
			int numofpair = (ipheader->size - sizeof(struct MyIPHeader))/12;
			char* data = (char*)(packet->data()+sizeof(struct MyIPHeader));
			for (int i = 0; i < numofpair; ++i){
				uint32_t addr1 = *((uint32_t*)(data + 12*i));
				uint32_t addr2 = *((uint32_t*)(data + 12*i + 4));
				uint32_t dist = *((uint32_t*)(data + 12*i + 8));
				if(ip_port_table.find(addr1) != ip_port_table.end() && ip_port_table.find(addr2) != ip_port_table.end()){
					if (dist < this->adjacentMatrix[ip_port_table[addr1]][ip_port_table[addr2]]){
						this->adjacentMatrix[ip_port_table[addr1]][ip_port_table[addr2]] = dist;
						this->adjacentMatrix[ip_port_table[addr2]][ip_port_table[addr1]] = dist;
					}
				}
			}
			dijkstra();
			ipheader->ttl--;
			click_chatter("888-%x: Forwarding BROADCASTING packet. PORTCOUNT: %d", this->ip_addr, PORTCOUNT);
			for (int i = 1; i < POINTCOUNT; ++i){
				if (i != port){
					output(i).push(packet);
				}
			}
			return;
		}
		if (ipheader->destination == this->ip_addr){
			//push to TCP through port 0;
			click_chatter("999-%x: Forwarding to TCP", this->ip_addr);
			WritablePacket* newpacket = Packet::make(NULL, ipheader->size - sizeof(MyIPHeader));
			memcpy(newpacket, packet->data()+sizeof(MyIPHeader), ipheader->size - sizeof(MyIPHeader));
			output(0).push(newpacket);
			return;
		}
		if(ip_port_table.find(ipheader->destination) != ip_port_table.end()){
			click_chatter("000-%x: Forwarding to other IP", this->ip_addr);
			ipheader->ttl--;
			output(findport(ip_port_table[ipheader->destination])).push(packet);
			return;
		}
		else{
			//not found in routing table
			//discard
		}
	}
}

void ip::dijkstra(){
	int mark[POINTCOUNT];
	for (int i = 0; i < POINTCOUNT; ++i){
		this->dist[i] = this->adjacentMatrix[0][i];
		mark[i] = 0;
		if(this->dist[i] != INF)
			this->path[i] = 0;
	}
	for (int j = 0; j < POINTCOUNT; ++j){
		int min = INF, minpos;
		for (int i = 0; i < POINTCOUNT; ++i)
			if (!mark[i] && this->dist[i] < min){
				min = this->dist[i];
				minpos = i;
			}
		if (min == INF) break;
		mark[minpos] = 1;
		for (int i = 0; i < POINTCOUNT; ++i)
			if (!mark[i] && this->dist[i] > this->dist[minpos] + this->adjacentMatrix[minpos][i]){
				this->dist[i] = this->dist[minpos] + this->adjacentMatrix[minpos][i];
				this->path[i] = minpos;
			}
	}
}

int ip::findport(int portnum){
	if(this->path[portnum] == 0)
		return portnum;
	return findport(this->path[portnum]);
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(ip)
