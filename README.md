TODO:
1.Client2TCP data structure
2.TCP2IP data structure
3.packet data structure


---------------------------------------------------------------

采用主动传输（client发起传输）
Elements:

1、client
接收一个待发送的文件，并发送到一个<IP地址，端口号>对。（目前先假设端口号单一）

如何实现socket,connet,send,rcv,close?
2、tcp_client/tcp_server
port_count返回值应为"2-/2-"
client和server行为不同实现为两个元素。暂时不考虑多路复用，处理后的包按约定push到不同的port上。
当连接已经建立时：
接收来自client的数据，加上TCP头，等待之前的确认，然后交给IP层。（超时重传）
接收来自IP的数据，检查并去掉TCP头，发送确认。

建立连接和释放连接：三次握手，FIN标志释放，FIN的ACK两倍超时则释放。（与实际一致）
认为IP提供不可靠无连接的服务。
维护的状态信息：
？

头格式：
待确定

3、IP（包括路由算法）
接收来自TCP的数据，加上IP头，发送到router。
接收来自router的数据，检查是否目标是自己，是的传给TCP，否则查找路由表并转发。然后更新路由信息。
不记录任何连接状态（不记录虚电路）。
无ARP部分，不记录mac地址。（凭本事传到目的地-_-)
校验和理应在这里计算。不过目前还没有考虑传输出错的实现和处理。

头格式：
待确定