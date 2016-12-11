#ifndef PACKETS_HH
#define PACKETS_HH

#define DATA 1
#define ACK 2
#define SYN 4
#define FIN 8
#define HELLO 16
#define RESP 32
#define BROAD 64

struct MyClientHeader{
	uint16_t dst_port;
	uint32_t dst_ip;
};

struct MyTCPHeader{
     uint16_t type; 
	 uint16_t sequence;
	 uint16_t ack_num;
	 uint16_t source;
	 uint16_t destination;
     uint32_t size;
	 uint32_t source_ip; // these are not real header data in tcp header
	 uint32_t dest_ip;   // they are used for passing argument to ip layer
};

struct MyIPHeader{
     uint16_t type;
     uint16_t ttl;
     uint32_t source;
     uint32_t destination;
     uint32_t size;
};

#endif