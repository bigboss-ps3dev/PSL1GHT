#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/net.h>
#include <arpa/inet.h>


//Debug variables
static int SocketFD;
#define DEBUG_IP "192.168.0.4"    //change to your PC workstation or Mac  ip
#define DEBUG_PORT 18194
#define printf debugPrintf //if you have not native printf support use my network friendly printf

void debugPrintf(const char* fmt, ...)
{
	char buffer[0x800];
	va_list arg;
	va_start(arg, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg);
	va_end(arg);
	netSend(SocketFD, buffer, strlen(buffer), 0);
}
void debugInit()
{

	struct sockaddr_in stSockAddr;
	netInitialize();
	SocketFD = netSocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	memset(&stSockAddr, 0, sizeof stSockAddr);

	stSockAddr.sin_family = AF_INET;
	stSockAddr.sin_port = htons(DEBUG_PORT);
	inet_pton(AF_INET, DEBUG_IP, &stSockAddr.sin_addr);

	netConnect(SocketFD, (struct sockaddr *)&stSockAddr, sizeof stSockAddr);
	

	
	debugPrintf("network debug module initialized\n") ;
	debugPrintf("ready to have a lot of fun\n") ;
	
	
}