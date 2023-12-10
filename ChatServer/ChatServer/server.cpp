#include <stdio.h>
#include <winsock2.h>
#include <process.h>
#include <stdlib.h>
#define SERVER_PORT 18000

#pragma comment(lib,"ws2_32.lib")

int sended = 0;
SOCKET sListen = INVALID_SOCKET;
SOCKADDR_IN saClient = { 0 };	
int length = sizeof(saClient);
HANDLE recvThread0 = NULL;
HANDLE recvThread1 = NULL;

typedef struct _Client
{
	SOCKET sServer;   
	char buffer[128];		
	char USERNAME[16]; 
	char IP[20];	
	UINT_PTR ID;  
}Client;

Client chatClient[2] = { 0 };             

unsigned __stdcall sendThread(void* param)
{
	int ret = 0;
	int ID = *(int*)param;
	SOCKET client = INVALID_SOCKET;			
	char context[128] = { 0 };				
	memcpy(context, chatClient[!ID].buffer, sizeof(context));
	sprintf_s(chatClient[ID].buffer, "%s: %s", chatClient[!ID].USERNAME, context);

	if (strlen(context) != 0 && sended == 0) 
		ret = send(chatClient[ID].sServer, chatClient[ID].buffer, sizeof(chatClient[ID].buffer), 0);
	if (ret == SOCKET_ERROR)
		return 1;
	sended = 1;   
	return 0;
}

unsigned __stdcall recvThread(void* param)
{
	SOCKET client = INVALID_SOCKET;
	int ID = 0;
	if (*(int*)param == chatClient[0].ID)       
	{
		client = chatClient[0].sServer;
		ID = 0;
	}
	else if (*(int*)param == chatClient[1].ID)
	{
		client = chatClient[1].sServer;
		ID = 1;
	}
	char context[128] = { 0 }; 
	while (1)
	{
		memset(context, 0, sizeof(context));
		int ret = recv(client, context, sizeof(context), 0);
		if (ret == SOCKET_ERROR)
			continue;
		sended = 0;							
		ID = client == chatClient[0].sServer ? 1 : 0; 
		memcpy(chatClient[!ID].buffer, context, sizeof(chatClient[!ID].buffer));
		_beginthreadex(NULL, 0, sendThread, &ID, 0, NULL); 
	}

	return 0;
}

unsigned __stdcall closeThread(void* param)
{
	while (1)
	{
		if (send(chatClient[0].sServer, "", sizeof(""), 0) == SOCKET_ERROR)
		{
			if (chatClient[0].sServer != 0)
			{
				CloseHandle(recvThread0);
				CloseHandle(recvThread1);
				closesocket(chatClient[0].sServer);
				chatClient[0] = { 0 };
			}
		}
		if (send(chatClient[1].sServer, "", sizeof(""), 0) == SOCKET_ERROR)
		{
			if (chatClient[1].sServer != 0)
			{
				CloseHandle(recvThread0);
				CloseHandle(recvThread1);
				closesocket(chatClient[1].sServer);
				chatClient[1] = { 0 };
			}
		}
		Sleep(2000);
	}
    return 0;
}

unsigned __stdcall acceptThread(void* param)
{
	int Client0 = 0, Client1 = 0;
	_beginthreadex(NULL, 0, closeThread, NULL, 0, NULL);
	while (1)
	{
		int i = 0;
		while (i < 2)
		{
			if (chatClient[i].ID != 0)
			{
				++i;
				continue;
			}
			chatClient[i].sServer = accept(sListen, (SOCKADDR*)&saClient, &length);
			if (chatClient[i].sServer == INVALID_SOCKET)
			{
				printf("accept() faild! code:%d\n", WSAGetLastError());
				closesocket(sListen); //关闭套接字
				WSACleanup();
				return -1;
			}
			recv(chatClient[i].sServer, chatClient[i].USERNAME, sizeof(chatClient[i].USERNAME), 0);
			memcpy(chatClient[i].IP, inet_ntoa(saClient.sin_addr), sizeof(chatClient[i].IP));
			chatClient[i].ID = chatClient[i].sServer;
			i++;
		}

		if (chatClient[0].ID != 0 && chatClient[1].ID != 0)		
		{
			if (chatClient[0].ID != Client0)
			{
				recvThread0 = (HANDLE)_beginthreadex(NULL, 0, recvThread, &chatClient[0].ID, 0, NULL);
			}
			if (chatClient[1].ID != Client1)
			{
				recvThread1 = (HANDLE)_beginthreadex(NULL, 0, recvThread, &chatClient[1].ID, 0, NULL);
			}
		}

		Client0 = chatClient[0].ID; 
		Client1 = chatClient[1].ID;
	}
    return 0;
}

int main()
{
	WORD wVersionRequested;
	WSADATA wsaData = { 0 };
	int ret;
	SOCKADDR_IN saServer = { 0 };			
	//WinSock初始化
	wVersionRequested = MAKEWORD(2, 2); //希望使用的WinSock DLL 的版本
	ret = WSAStartup(wVersionRequested, &wsaData);
	if (ret != 0)
	{
		printf("WSAStartup() failed!\n");
		return -1;
	} 
	//创建Socket,使用TCP协议
	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sListen == INVALID_SOCKET)
	{
		WSACleanup();
		printf("socket() faild!\n");
		return -1;
	}
	//构建本地地址信息
	saServer.sin_family = AF_INET; //地址家族
	saServer.sin_port = htons(SERVER_PORT); //注意转化为网络字节序
	saServer.sin_addr.S_un.S_addr = htonl(INADDR_ANY); //使用INADDR_ANY 指示任意地址

	//绑定
	ret = bind(sListen, (SOCKADDR*) & saServer, sizeof(saServer));
	if (ret == SOCKET_ERROR)
	{
		printf("bind() faild! code:%d\n", WSAGetLastError());
		closesocket(sListen); //关闭套接字
		WSACleanup();
		return -1;
	}

	//侦听连接请求
	ret = listen(sListen, 20000);
	if (ret == SOCKET_ERROR)
	{
		printf("listen() faild! code:%d\n", WSAGetLastError());
		closesocket(sListen); //关闭套接字
		return -1;
	}

	printf("Waiting for client connecting!\n");
	printf("Tips: Ctrl+c to quit!\n");
	//阻塞等待接受客户端连接

	_beginthreadex(NULL, 0, acceptThread, NULL, 0, 0);
	for (int k = 0; k < 100; k++)
		Sleep(10000000);

	closesocket(sListen);
	for (int j = 0; j < 2; j++)
	{
		if (chatClient[j].sServer != INVALID_SOCKET)
			closesocket(chatClient[j].sServer);
	}
	WSACleanup();
    return 0;
}