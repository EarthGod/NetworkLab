typedef enum {
	DATA = 0,
	ACK = 1,
	SYN = 2,
	FIN = 4
} packet_types;
// SYN + ACK represents SYN_ACK
struct MyTCPPacketHeader{
     packet_types type; 
	 uint8_t sequence;
	 uint8_t source;
	 uint8_t ack_num;
	 uint8_t destination;
     uint32_t size;
};