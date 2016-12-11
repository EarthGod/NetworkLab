#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include <map>
#include "ip.hh"
using std::map;

//NOT GOOD; SHOULD KNOW HOW MANY PORTS ARE DEFINED IN xx.click
#define PORTCOUNT 10
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
	timer_update.schedule_after_sec(4);
    return 0;
}

int ip::configure(Vector<String> &conf, ErrorHandler *errh) {
  if (cp_va_kparse(conf, this, errh,
                  "IP_ADDR", cpkP+cpkM, cpUnsigned, &ip_addr,
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
				if(this->adjacentMatrix[i][j] != INF)
					cnt++;
		WritablePacket *packet = Packet::make(0,0,sizeof(struct MyIPHeader)+12*cnt, 0);
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
	    for (int i = 1; i < PORTCOUNT; ++i)
	    	output(i).push(packet);
	    timer_update.schedule_after_sec(TIMERPERIOD);
	}
	else if(timer == &this->timer_hello){
		WritablePacket *packet = Packet::make(0,0,sizeof(struct MyIPHeader), 0);
	    struct MyIPHeader *format = (struct MyIPHeader*) packet->data();
	    format->type = HELLO;
		format->source = this->ip_addr;
		format->destination = 0xffffffff; // stands for broadcast
		format->ttl = 1; //time to live
	    format->size = sizeof(struct MyIPHeader);
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
		struct tcp_Header *tcpheader = (struct tcp_Header*)packet->data();
		if (this->ip_port_table.find(tcpheader->dest_ip) == this->ip_port_table.end()){
			click_chatter("NOT FOUND IN ROUTER TABLE: %d; PORT: %d;", tcpheader->dest_ip, port);
			packet->kill();
			return;
		}
		click_chatter("Received packet from %u on port %d", tcpheader->source, port);
		//wrap
		WritablePacket* newpacket = Packet::make(0,0,sizeof(struct MyIPHeader)+(tcpheader->size), 0);
	    struct MyIPHeader* format = (struct MyIPHeader*) newpacket->data();
	    format->type = DATA; //DATA
		format->source = tcpheader->source_ip;
		format->destination = tcpheader->dest_ip;
		format->ttl = this->dist[ip_port_table[tcpheader->dest_ip]];
		format->size = tcpheader->size + (sizeof(struct MyIPHeader));
		char *data = (char*)(newpacket->data()+sizeof(struct MyIPHeader));
		memcpy(data, tcpheader, tcpheader->size);
		//send
		output(findport(ip_port_table[tcpheader->dest_ip])).push(newpacket);
	}
	else{
		struct MyIPHeader *ipheader = (struct MyIPHeader*)packet->data();
		if (ipheader->ttl <= 0){
			packet->kill();
			return;
		}
		click_chatter("Received packet from %u on port %d, type: %d", ipheader->source, port, ipheader->type);
		if (ipheader->type == HELLO){
			//HELLO: return the topology this router know
			WritablePacket* newpacket = Packet::make(0,0,sizeof(struct MyIPHeader), 0);
			struct MyIPHeader* format = (struct MyIPHeader*) newpacket->data();
	   		format->type = RESP; //RESP
			format->source = this->ip_addr;
			format->destination = ipheader->source;
			format->ttl = 1;
			format->size = sizeof(struct MyIPHeader);
			output(port).push(newpacket);
			return;
		}
		else if (ipheader->type == RESP){
			//RESP: update the adjacent matrix
			ip_port_table[ipheader->source] = port;
			port_ip_table[port] = ipheader->source;
			adjacentMatrix[0][port] = 1;
			adjacentMatrix[port][0] = 1;
			packet->kill();
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
			for (int i = 1; i < POINTCOUNT; ++i){
				if (i != port){
					output(i).push(packet);
				}
			}
			return;
		}
		if (ipheader->destination == this->ip_addr){
			//push to TCP through port 0;
			WritablePacket* newpacket = Packet::make(0,0,ipheader->size - sizeof(MyIPHeader), 0);
			memcpy(newpacket, packet->data()+sizeof(MyIPHeader), ipheader->size - sizeof(MyIPHeader));
			output(0).push(newpacket);
			return;
		}
		if(ip_port_table.find(ipheader->destination) != ip_port_table.end()){
			ipheader->ttl--;
			output(findport(ip_port_table[ipheader->destination])).push(packet);
			return;
		}
		else{
			//not found in routing table
			//discard
			packet->kill();
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
