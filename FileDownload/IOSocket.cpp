#include "IOSocket.h"
#include<winsock2.h>
#include<iostream>


using namespace std;


IOSocket::IOSocket()
{
	sock = INVALID_SOCKET;
	_server_port = 20001;
	_server_addr = "192.168.66.130";
}

IOSocket::~IOSocket(void)
{

}

IOSocket& IOSocket::GetInstance()
{
	static IOSocket socketInstance;
	return socketInstance;
}


bool IOSocket::Connect(int timeout)
{
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		cout << "Fail to create socket" << endl;
		return false;
	}

	long tmSend = 3 * 1000L;
	long tmRecv = 3 * 1000L;
	long noDelay = 1;
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (LPSTR)&noDelay, sizeof(long));
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (LPSTR)&tmSend, sizeof(long));
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (LPSTR)&tmRecv, sizeof(long));

	//将socket设置成非阻塞的
	unsigned long on = 1;
	if (ioctlsocket(sock, FIONBIO, &on) == SOCKET_ERROR)
		return false;

	sockaddr_in saddr;
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(_server_port);
	saddr.sin_addr.s_addr = inet_addr(_server_addr.c_str());
	int ret = connect(sock, (sockaddr*)&saddr, sizeof(saddr));

	if (ret == 0) {
		cout << "connect server success" << endl;
	}
	else if(ret == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK){
		cout << "fail to connect server" << endl;
		return false;

	}

	fd_set writeset;
	FD_ZERO(&writeset);
	FD_SET(sock, &writeset);
	struct timeval tv = { timeout, 0 };
	if (::select(sock + 1, NULL, &writeset, NULL, &tv) != 1)
	{
		cout << "Could not connect to file server." << endl;
		return false;
	}
	return true;

	
}

void IOSocket::CloseFileServerConnection()
{
	if(sock == INVALID_SOCKET)
		return;

	shutdown(sock, SD_BOTH);
	closesocket(sock);
	sock = INVALID_SOCKET;
	cout << "Disconnect from server" << endl;
}

bool IOSocket::Send(const char* pBuffer, int64_t nSize, int nTimeout)
{
	int64_t nStartTime = time(NULL);

	int64_t nSentBytes = 0;
	int nRet = 0;
	int64_t now;
	do
	{
		//FIXME: 将int64_t强制转换成int32可能会有问题
		nRet = send(sock, pBuffer + nSentBytes, (int)(nSize - nSentBytes), 0);
		if (nRet == SOCKET_ERROR && ::WSAGetLastError() == WSAEWOULDBLOCK)
		{
			::Sleep(1);
			now = (int64_t)time(NULL);
			if (now - nStartTime < (int64_t)nTimeout)
				continue;
			else
			{
				//超时了,关闭socket,并返回false
				CloseFileServerConnection();
				cout<<"Send data timeout."<<endl;
				return false;
			}
		}
		else if (nRet < 1)
		{
			//一旦出现错误就立刻关闭Socket
			cout << "Send data error, disconnect file server"<<endl;
			CloseFileServerConnection();
			return false;
		}

		nSentBytes += (int64_t)nRet;

		if (nSentBytes >= nSize)
			break;

		::Sleep(1);

	} while (true);

	return true;
}

bool IOSocket::Recv(char* pBuffer, int64_t nSize, int nTimeout)
{
	int64_t nStartTime = time(NULL);

	int nRet = 0;
	int64_t nRecvBytes = 0;
	int64_t now;

	do
	{
		nRet = ::recv(sock, pBuffer + nRecvBytes, (int)(nSize - nRecvBytes), 0);
		if (nRet == SOCKET_ERROR && ::WSAGetLastError() == WSAEWOULDBLOCK)
		{
			::Sleep(1);
			now = time(NULL);
			if (now - nStartTime < (int64_t)nTimeout)
				continue;
			else
			{
				//超时了,关闭socket,并返回false
				CloseFileServerConnection();
				cout<<"Recv data timeout."<<endl;
				return false;
			}
		}
		//一旦出现错误就立刻关闭Socket
		else if (nRet < 1)
		{
			cout<<"Recv data error"<<endl;
			CloseFileServerConnection();
			return false;
		}

		nRecvBytes += (int64_t)nRet;
		if (nRecvBytes >= nSize)
			break;

		::Sleep(1);

	} while (true);


	return true;
}


