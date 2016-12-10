typedef enum {
	HELLO = 0,
	RESP = 1,
	BROAD = 2,
	DATA = 4,
} packet_types;
// ASK/ANSWER to get the topology of the network
struct MyIPHeader{
     packet_types type; 
     uint32_t source;
     uint32_t destination;
     uint32_t size;
     uint32_t ttl;
};