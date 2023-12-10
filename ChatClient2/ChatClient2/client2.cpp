#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <process.h>
#include <conio.h>
#define SERVER_PORT 18000 //侦听端口
#define USERNAME "Client2"

#pragma comment(lib,"ws2_32.lib")

unsigned __stdcall recvThread(void* param)
{
	char buf[128] = { 0 };
	while (1)
	{
		int ret = recv(*(SOCKET*)param, buf, sizeof(buf), 0);
		if (ret == SOCKET_ERROR)
		{
			Sleep(100);
			continue;
		}
		if (strlen(buf) != 0)
		{
			printf("%s\n", buf);
		}
	}
	return 0;
}

unsigned __stdcall sendThread(void* param)
{
	char buffer[128] = { 0 };
	int ret = 0;
	while (1)
	{
		_getch();
		printf(USERNAME);
		printf(":");
		gets_s(buffer);
		ret = send(*(SOCKET*)param, buffer, sizeof(buffer), 0);
		if (ret == SOCKET_ERROR)
			return 1;
	}
	return 0;
}

int main()
{
	WORD wVersionRequested;
	WSADATA wsaData = { 0 };
	int ret;
	SOCKET sClient = INVALID_SOCKET; //连接套接字
	SOCKADDR_IN saServer = { 0 }; //地址信息
	//WinSock初始化
	wVersionRequested = MAKEWORD(2, 2); //希望使用的WinSock DLL的版本
	ret = WSAStartup(wVersionRequested, &wsaData);
	if (ret != 0)
	{
		printf("WSAStartup() failed!\n");
		return -1;
	}
	//确认WinSock DLL支持版本2.2
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		WSACleanup();
		printf("Invalid WinSock version!\n");
		return -1;
	}
	//创建Socket,使用TCP协议
	sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sClient == INVALID_SOCKET)
	{
		WSACleanup();
		printf("socket() failed!\n");
		return -1;
	}
	//构建服务器地址信息
	saServer.sin_family = AF_INET; //地址家族
	saServer.sin_port = htons(SERVER_PORT); //注意转化为网络节序
	saServer.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	//连接服务器
	ret = connect(sClient, (SOCKADDR*)&saServer, sizeof(saServer));
	if (ret == SOCKET_ERROR)
	{
		printf("connect() failed!\n");
		closesocket(sClient); //关闭套接字
		WSACleanup();
		return -1;
	}

	send(sClient, USERNAME, sizeof(USERNAME), 0);

	_beginthreadex(NULL, 0, recvThread, &sClient, 0, NULL);
	_beginthreadex(NULL, 0, sendThread, &sClient, 0, NULL);
	for (int k = 0; k < 1000; k++)
		Sleep(1000000);

	closesocket(sClient);
	WSACleanup();
	return 0;
}
