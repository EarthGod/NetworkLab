typedef enum {
	TEST = 0,
	ACK,
	SYN,
} packet_types;

struct PacketHeader{
	uint8_t type; 
	uint8_t sequence;
	uint8_t source;
	uint8_t destination;
    uint32_t size;
};