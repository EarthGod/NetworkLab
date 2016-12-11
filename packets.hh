typedef enum {
	DATA = 1,
	ACK = 2,
	SYN = 4,
	FIN = 8,
	HELLO = 16,
	RESP = 32,
	BROAD = 64,
} packet_types;
// SYN + ACK represents SYN_ACK
struct MyTCPHeader{
     packet_types type; 
	 uint16_t sequence;
	 uint16_t ack_num;
	 uint16_t source;
	 uint16_t destination;
     uint32_t size;
	 uint32_t source_ip; // these are not real header data in tcp header
	 uint32_t dest_ip;   // they are used for passing argument to ip layer
};

struct MyIPHeader{
     packet_types type; 
     uint32_t source;
     uint32_t destination;
     uint32_t size;
     uint32_t ttl;
};