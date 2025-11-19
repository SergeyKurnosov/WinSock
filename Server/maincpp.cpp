#define _CRT_SECURE_NO_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN   
#endif 


#include<Windows.h>
#include<WinSock2.h>
#include<ws2tcpip.h>
#include<iphlpapi.h>
#include<iostream>

#pragma comment(lib, "WS2_32.lib")

#define DEFAULT_PORT	"27015"
#define BUFFER_LENGTH	1460
#define CLIENTS_MAX		3
#define g_sz_SORRY		"Error: слишком много подключений"
#define IP_STR_MAX_LENGTH	16

INT n = 0; // количество активных клиентов
SOCKET client_sockets[CLIENTS_MAX] = {};
DWORD threadIDs[CLIENTS_MAX] = {};
HANDLE hThreads[CLIENTS_MAX] = {};

VOID WINAPI HandleClient(SOCKET client_socket);

using namespace std;

int main()
{
	setlocale(LC_ALL, "");
	DWORD dwLastError = 0;
	INT iResult = 0;

	//0)инициализируем winsock
	WSADATA wsaData;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		dwLastError = WSAGetLastError();
		cout << "WSA init failed with: " << dwLastError << endl;
		return dwLastError;
	}
	//1)инициализируем переменную для сокета 
	addrinfo* result = NULL;
	addrinfo* ptr = NULL;
	addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	//2)задаем параметры сокета
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0)
	{
		dwLastError = WSAGetLastError();
		cout << "getaddrinfo failed with error: " << dwLastError << endl;
		freeaddrinfo(result);
		WSACleanup();
		return dwLastError;
	}

	//3)создаем сокет котрый будет прослушивать сервер
	SOCKET listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listen_socket == INVALID_SOCKET)
	{
		dwLastError = WSAGetLastError();
		cout << "Socket creation failed with error: " << dwLastError << endl;
		freeaddrinfo(result);
		WSACleanup();
		return dwLastError;
	}

	//4) Bind socket
	iResult = bind(listen_socket, result->ai_addr, result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		dwLastError = WSAGetLastError();
		cout << "Bind failed with error: " << dwLastError << endl;
		closesocket(listen_socket);
		freeaddrinfo(result);
		WSACleanup();
		return dwLastError;
	}

	//5) запускаем прослушивание сокета
	iResult = listen(listen_socket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		dwLastError = WSAGetLastError();
		cout << "Listen faild with error: " << dwLastError << endl;
		closesocket(listen_socket);
		freeaddrinfo(result);
		WSACleanup();
		return dwLastError;
	}

	//6) обработка запросов от клиентов

	cout << "Acept client connections...." << endl;
	do
	{
		SOCKET client_socket = accept(listen_socket, NULL, NULL);
		//if (client_sockets[n] == INVALID_SOCKET)
		//{
		//	dwLastError = WSAGetLastError();
		//	cout << "Accept failed with error: " << dwLastError << endl;
		//	closesocket(listen_socket);
		//	freeaddrinfo(result);
		//	WSACleanup();
		//	return dwLastError;
		//}
		////HandleClient(client_socket);
		if (n < CLIENTS_MAX)
		{
			client_sockets[n] = client_socket;
			hThreads[n] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)HandleClient, (LPVOID)client_sockets[n], 0, threadIDs + n);
			n++;
		}
		else
		{
			CHAR recv_buffer[BUFFER_LENGTH] = {};
			INT iResult = recv(client_socket, recv_buffer, BUFFER_LENGTH, 0);
			if (iResult > 0)
			{
				cout << "Bytes received: " << iResult << endl;
				cout << "Message: " << recv_buffer << endl;
				INT iSendResult = send(client_socket, g_sz_SORRY, strlen(g_sz_SORRY), 0);
				closesocket(client_socket);
			}
		}
	} while (true);


	WaitForMultipleObjects(CLIENTS_MAX, hThreads, TRUE, INFINITE);

	closesocket(listen_socket);
	freeaddrinfo(result);
	WSACleanup();
	return dwLastError;
}

INT GetSlotIndex(DWORD dwID)
{
	for (int i = 0; i < CLIENTS_MAX; i++)
	{
		if (threadIDs[i] == dwID)return i;
	}
}


VOID Shift(INT start)
{
	for (INT i = start; i < CLIENTS_MAX; i++)
	{
		client_sockets[i] = client_sockets[i + 1];
		threadIDs[i] = threadIDs[i + 1];
		hThreads[i] = hThreads[i + 1];
	}
	client_sockets[CLIENTS_MAX - 1] = NULL;
	threadIDs[CLIENTS_MAX - 1] = NULL;
	hThreads[CLIENTS_MAX - 1] = NULL;
	n--;
}

VOID Broadcast(CONST CHAR message[], SOCKET source)
{
	for (int i = 0; i < n; i++)
	{
		if (client_sockets[i] != source)
		{
			INT iResult = send(client_sockets[i], message, strlen(message), 0);
		}
	}
}


VOID WINAPI HandleClient(SOCKET client_socket)
{
	SOCKADDR_IN peer{};
	CHAR address[IP_STR_MAX_LENGTH] = {};
	INT address_LENGTH = 16;
	getpeername(client_socket, (SOCKADDR*)&peer, &address_LENGTH);
	inet_ntop(AF_INET, &peer.sin_addr, address, address_LENGTH);
	INT port = ((peer.sin_port & 0xFF) << 8) + (peer.sin_port >> 8);
	cout << address << ":" << port << endl;
	/////////////////////////////////////
	INT iResult = 0;
	DWORD dwLastError = 0;
	//7) получение запросов от клиента
	CHAR send_buffer[BUFFER_LENGTH] = "Привет клиент";
	CHAR recv_buffer[BUFFER_LENGTH] = {};
	

	do
	{
		INT iSendResult = 0;
		ZeroMemory(send_buffer, BUFFER_LENGTH);
		ZeroMemory(recv_buffer, BUFFER_LENGTH);
		iResult = recv(client_socket, recv_buffer, BUFFER_LENGTH, 0);
		if (iResult > 0)
		{
			cout << iResult << " Bytes received from " << address << ":" << port << "-" << recv_buffer << endl;
			sprintf(send_buffer, "%i Bytes received from %s:%i - %s;\n", iResult, address, port, recv_buffer);
			Broadcast(send_buffer, client_socket);
			iSendResult = send(client_socket, recv_buffer, strlen(recv_buffer), 0);
			if (iSendResult == SOCKET_ERROR)
			{
				dwLastError = WSAGetLastError();
				cout << "Send failed with error: " << dwLastError << endl;
				break;
			}
			cout << "Byte sent: " << iSendResult << endl;
		}
		else if (iResult == 0) cout << "Connection closing" << endl;
		else
		{
			dwLastError = WSAGetLastError();
			cout << "Receive failed with error: " << dwLastError << endl;
			break;
		}
	} while (iResult > 0 && !strstr(recv_buffer, "quit"));
	DWORD dwID = GetCurrentThreadId();
	Shift(GetSlotIndex(dwID));
	cout << address << ":" << port << " leaved" << endl;
	ExitThread(0);
	closesocket(client_socket);


}
