TODO:
1.Client2TCP data structure
2.TCP2IP data structure
3.packet data structure





Elements:

1、client
接收一个待发送的文件，并发送到一个<IP地址，端口号>对
2、TCP
当连接已经建立时：
接收来自client的数据，加上TCP头，等待之前的确认，然后交给IP层。（超时重传）
接收来自IP的数据，检查并去掉TCP头，发送确认。

建立连接和释放连接：三次握手，FIN标志释放，FIN的ACK两倍超时则释放。（与实际一致）
维护的状态信息：
？

头格式：
待确定

3、IP（包括路由算法）
接收来自TCP的数据，加上IP头，发送到router。
接收来自router的数据，检查是否目标是自己，是的传给TCP，否则查找路由表并转发。然后更新路由信息。

头格式：
待确定