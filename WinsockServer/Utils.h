#pragma once

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>

bool sendImage(const std::string& path, SOCKET connection);

bool sendOffsets(const std::string& path, SOCKET connection);