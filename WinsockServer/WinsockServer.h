#pragma once

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>
#include <stdexcept>
#include <thread>
#include <functional>
#include <fstream>
#include <ctime>

#pragma comment (lib, "Ws2_32.lib")

#pragma warning (disable : 4996)

class WinsockServer
{
	WSADATA							wsaData;
	SOCKET							listenSocket;
	std::function<void(SOCKET)>		handlerPtr;
	std::ofstream					logger;

public:

	WinsockServer(const char* port, std::function<void(SOCKET)> handlerPtr) : handlerPtr{ handlerPtr } {

		if (WSAStartup(MAKEWORD(2, 2), &wsaData))
			throw std::runtime_error("WSAStartup failed with code " + std::to_string(WSAGetLastError()));

		ADDRINFO hints{}, *serverInfo;

		hints.ai_family		= AF_INET;
		hints.ai_socktype	= SOCK_STREAM;
		hints.ai_protocol	= IPPROTO_TCP;

		int status{};

		if (status = getaddrinfo("192.168.0.49", port, &hints, &serverInfo))          
			throw std::runtime_error("getaddrinfo failed with status: " + std::string(gai_strerrorA(status)));

		listenSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

		if (listenSocket == INVALID_SOCKET) {

			freeaddrinfo(serverInfo);

			WSACleanup();

			throw std::runtime_error("Error creating socket with code " + std::to_string(WSAGetLastError()));
		}

		if (bind(listenSocket, serverInfo->ai_addr, serverInfo->ai_addrlen) == SOCKET_ERROR) {

			closesocket(listenSocket);

			freeaddrinfo(serverInfo);

			WSACleanup();

			throw std::runtime_error("Error binding socket with code " + std::to_string(WSAGetLastError()));
		}

		freeaddrinfo(serverInfo);

		if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {

			closesocket(listenSocket);

			WSACleanup();

			throw std::runtime_error("Error listening with code " + std::to_string(WSAGetLastError()));
		}
	}

	~WinsockServer() {

		closesocket(listenSocket);

		WSACleanup();
	}

	WinsockServer(const WinsockServer& other)				= delete;

	WinsockServer& operator=(const WinsockServer& other)	= delete;

	void processConnection() {

		SOCKADDR_IN incomingConnectionInfo{};
		
		int addrLen = sizeof(incomingConnectionInfo);

		SOCKET connection = accept(listenSocket, reinterpret_cast<SOCKADDR*>(&incomingConnectionInfo), &addrLen);

		if (connection) {

			logConnection(incomingConnectionInfo);

			std::thread thread(handlerPtr, connection);

			thread.detach();
		}
	}

	void logConnection(const SOCKADDR_IN& incomingConnectionInfo) {

		logger.open("logs.txt", std::ios::binary | std::ios::app);

		std::time_t connectionTime = std::time(nullptr);

		char ipAddress[INET_ADDRSTRLEN]{};

		if (inet_ntop(AF_INET, &incomingConnectionInfo.sin_addr, ipAddress, INET_ADDRSTRLEN) != nullptr) {

			logger << std::ctime(&connectionTime) << ipAddress << "\n\n";
		}

		logger.close();
	}
};

