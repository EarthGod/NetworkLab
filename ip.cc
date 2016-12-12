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
#define TIMERPERIOD 2

CLICK_DECLS 

ip::ip(): timer_update(this), timer_hello(this){
	ptcnt = 0;
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
	timer_hello.schedule_now();
	timer_update.schedule_after_sec(2*TIMERPERIOD);
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
  mat2ipaddr[0] = this->ip_addr;
  ipaddr2mat[this->ip_addr] = 0;
  return 0;
}

void ip::run_timer(Timer *timer){
	if(timer == &this->timer_update){
		timer_update.reschedule_after_sec(4*TIMERPERIOD);
		int cnt = 0;
		for (int i = 0; i < POINTCOUNT; ++i)
			for (int j = 0; j < POINTCOUNT; ++j)
				if(this->adjacentMatrix[i][j] != INF && this->adjacentMatrix[i][j] != 0)
					cnt++;
	
	    //click_chatter("111-%x: BROADCASTING adjacentMatrix. PORTCOUNT: %d", this->ip_addr, PORTCOUNT);
	    for (int i = 1; i < PORTCOUNT; ++i){
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
	    			if(this->adjacentMatrix[i][j] != INF && i != j){
	    				memcpy(data, &mat2ipaddr[i], sizeof(uint32_t));
	    				memcpy(data+4, &mat2ipaddr[j], sizeof(uint32_t));
	    				memcpy(data+8, &this->adjacentMatrix[i][j], sizeof(int));
	    				data += 12;
	    			}
	    		}
	    	}
	    	//click_chatter("%x: 111 pushing %d", this->ip_addr, i);
	    	output(i).push(packet);
	    }
	}
	else if(timer == &this->timer_hello){
		timer_hello.reschedule_after_sec(4*TIMERPERIOD);
	    //click_chatter("222-%x: BROADCASTING HELLO. PORTCOUNT: %d", this->ip_addr, PORTCOUNT);
	    for(int i = 1; i < PORTCOUNT; ++i){
	    	WritablePacket *packet = Packet::make(NULL, sizeof(struct MyIPHeader));
	    	struct MyIPHeader *format = (struct MyIPHeader*) packet->data();
	    	format->type = HELLO;
			format->source = this->ip_addr;
			format->destination = 0xffffffff; // stands for broadcast
			format->ttl = 1; //time to live
	    	format->size = sizeof(struct MyIPHeader);
	    	output(i).push(packet);
	    }
	}
	else{
		assert(false);
	}
}

void ip::push(int port, Packet *packet) {
	assert(packet);
	//Assume that IP get packet from TCP through data 0;
	if(port == 0){
		struct MyTCPHeader *tcpheader = (struct MyTCPHeader*)(packet->data());
		if (ipaddr2mat.find(tcpheader->dest_ip) == ipaddr2mat.end()){
			//click_chatter("%x: NOT FOUND IN ROUTER TABLE: %x; PORT: %d;", this->ip_addr, tcpheader->dest_ip, port);
			packet->kill();
			return;
		}
		//click_chatter("333-%x: Received packet from %x on port %d, type: %d. packet dest: %x.", this->ip_addr, tcpheader->source_ip, port,tcpheader->type,tcpheader->dest_ip);
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
		//click_chatter("444-%x: sending to other ip element! Port: %d", this->ip_addr, findport(ip_port_table[tcpheader->dest_ip]));
		int port2out = findport(ipaddr2mat[tcpheader->dest_ip]);
		if (port2out != -1)
			output(port2out).push(newpacket);
	}
	else{
		struct MyIPHeader *ipheader = (struct MyIPHeader*)packet->data();
		//click_chatter("555-%x: Received packet from %x on port %d, type: %d. packet dest: %x.", this->ip_addr, ipheader->source, port, ipheader->type, ipheader->destination);
		if (ipheader->type == HELLO){
			//HELLO: return the topology this router know
			WritablePacket* newpacket = Packet::make(NULL,sizeof(struct MyIPHeader));
			struct MyIPHeader* format = (struct MyIPHeader*) newpacket->data();
	   		format->type = RESP; //RESP
			format->source = this->ip_addr;
			format->destination = ipheader->source;
			format->ttl = 1;
			format->size = sizeof(struct MyIPHeader);
			//click_chatter("666-%x: sending to other ip element! Port: %d", this->ip_addr, port);
			output(port).push(newpacket);
			return;
		}
		else if (ipheader->type == RESP){
			//RESP: update the adjacent matrix
			if(ipaddr2mat.find(ipheader->source) != ipaddr2mat.end()){
				packet->kill();
				return;
			}
			ip_port_table[ipheader->source] = port;
			port_ip_table[port] = ipheader->source;
			ptcnt++;
			mat2ipaddr[ptcnt] = ipheader->source;
			ipaddr2mat[ipheader->source] = ptcnt;
			adjacentMatrix[0][ptcnt] = 1;
			adjacentMatrix[ptcnt][0] = 1;
			/*
			click_chatter("777-%x: updated adjacentMatrix! printing:", this->ip_addr);
			for (int i = 0; i <= ptcnt; ++i){
				for (int j = 0; j <= ptcnt; ++j){
					if (adjacentMatrix[i][j] != INF){
						click_chatter("%x, %x: %d", mat2ipaddr[i], mat2ipaddr[j], adjacentMatrix[i][j]);
					}
				}
			}
			*/
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
				//click_chatter("adding: %x, %x: %d", addr1, addr2, dist);
				if(ipaddr2mat.find(addr1) == ipaddr2mat.end()){
					//click_chatter("NOT found addr1: %x!", addr1);
					ptcnt++;
					ipaddr2mat[addr1] = ptcnt;
					mat2ipaddr[ptcnt] = addr1;
				}
				if(ipaddr2mat.find(addr2) == ipaddr2mat.end()){
					//click_chatter("NOT found addr2: %x!", addr2);
					ptcnt++;
					ipaddr2mat[addr2] = ptcnt;
					mat2ipaddr[ptcnt] = addr2;
				}
				if (dist < this->adjacentMatrix[ipaddr2mat[addr1]][ipaddr2mat[addr2]]){
					this->adjacentMatrix[ipaddr2mat[addr1]][ipaddr2mat[addr2]] = dist;
					this->adjacentMatrix[ipaddr2mat[addr2]][ipaddr2mat[addr1]] = dist;
				}
			}
			/*
			click_chatter("%x: parse BROAD finished!! now printing matrix: ", this->ip_addr);
			for (int i = 0; i <= ptcnt; ++i){
				for (int j = 0; j <= ptcnt; ++j){
					if (adjacentMatrix[i][j] != INF){
						click_chatter("%x, %x: %d", mat2ipaddr[i], mat2ipaddr[j], adjacentMatrix[i][j]);
					}
				}
			}
			*/
			dijkstra();
			packet->kill();
			return;
		}
		assert(ipheader->type == DATA);
		//click_chatter("000-%x: DESTIP: %x", this->ip_addr, ipheader->destination);
		if (ipheader->destination == this->ip_addr){
			//push to TCP through port 0;
			//click_chatter("999-%x: Forwarding to TCP", this->ip_addr);
			WritablePacket* newpacket = Packet::make(packet->data()+sizeof(MyIPHeader), ipheader->size - sizeof(MyIPHeader));
			output(0).push(newpacket);
			return;
		}
		if(ipaddr2mat.find(ipheader->destination) != ipaddr2mat.end()){
			//click_chatter("000-%x: Forwarding to other IP. Port: %d", this->ip_addr,findport(ipaddr2mat[ipheader->destination]));
			ipheader->ttl--;
			output(findport(ipaddr2mat[ipheader->destination])).push(packet);
			return;
		}
		else{
			//not found in routing table
			//discard
			
			//flood?
			/*
			for (int i = 1; i < PORTCOUNT; ++i){
				output(i).push(packet);
			}
			*/
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
	/*
	click_chatter("%x: DIJKSTRA FINISHED! PRINTING PATH&DIST!", this->ip_addr);
	click_chatter("path:");
	for (int i = 0; i <= ptcnt; ++i)
		click_chatter("ipaddr-%x, numbered-%d!!! dist: %d, path: %d", mat2ipaddr[i], i, dist[i], path[i]);
	*/
}

int ip::findport(int ptnum){
	if(this->path[ptnum] == ipaddr2mat[ip_addr])
		return ip_port_table[mat2ipaddr[ptnum]];
	return findport(this->path[ptnum]);
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(ip)
