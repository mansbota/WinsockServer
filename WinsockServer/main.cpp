
#include <iostream>
#include "WinsockServer.h"
#include <thread>
#include <mutex>
#include <vector>
#include "Database.h"
#include "User.h"
#include "Timer.h"
#include "Utils.h"

#define LOGIN		0x5CD100F
#define REGISTER	0x20CC1D
#define ADDKEY      0x4411969
#define VALIDATE    0x988CCD

struct CONN_REQ {

	ULONGLONG	requestType;
	char		name[32];
	char		password[32];
	char		key[32];
	char		extra[32];
};

std::mutex userMutex;

std::vector<User> usersLoggedIn;

bool clientDisconnected(SOCKET connection) {

	char ping[64];

	while (recv(connection, ping, sizeof(ping), 0)) {

		if (send(connection, ping, sizeof(ping), 0) == SOCKET_ERROR)
			return true;

		return false;
	}

	return true;
}

void logoutThread() {

	while (true) {

		for (auto& user : usersLoggedIn) {

			if (clientDisconnected(user.getConnection())) {

				std::string name = user.getName();

				userMutex.lock();

				user.logout(usersLoggedIn);

				userMutex.unlock();

				std::cout << "User " << name << " disconnected.\n";

				break;
			}
		}

		Sleep(1000);
	}
}

void invalidator() {

	try {

		Database database{ "data.db" };

		database.open();

		auto result = database.invalidate();

		database.close();

		if (result) {

			std::cout << "Keys invalidated:\n";

			for (const auto& name : result.value()) {

				std::cout << name << std::endl;
			}
		}

		Timer timer;

		while (true) {

			Sleep(3600000);

			if (timer.elapsed() >= 4.0) {

				database.open();

				auto result = database.invalidate();

				database.close();

				if (result) {

					std::cout << "Keys invalidated:\n";

					for (const auto& name : result.value()) {

						std::cout << name << std::endl;
					}
				}

				timer.reset();
			}
		}
	}
	catch (std::exception& ex) {

		std::cerr << "Invalidator thread exception: " << ex.what() << std::endl;

	}
}

void handleConnection(SOCKET connection) {

	try {

		static Database database{ "data.db" };

		CONN_REQ request{};

		int receivedBytes = recv(connection, reinterpret_cast<char*>(&request), sizeof(request), 0);

		if (receivedBytes == SOCKET_ERROR) {

			std::cerr << "recv Error. Code: " << WSAGetLastError() << std::endl;

			return;
		}

		if (receivedBytes == 0) {

			std::cerr << "Error: Connection closed.\n";

			return;
		}

		database.open();

		switch (request.requestType) {

			case LOGIN:
			{
				User user{ connection, request.name, request.password, database.get() };

				userMutex.lock();

				std::string response = user.login(usersLoggedIn);

				userMutex.unlock();

				if (send(connection, response.c_str(), response.length(), 0) == SOCKET_ERROR)
					break;

				if (response != "Logged in")
					break;

				static std::thread logout{ logoutThread };

				std::cout << "User " << user.getName() << " connected.\n";

				if (!sendImage("C:\\Users\\grgic\\Desktop\\dawn\\Dawn.exe", connection)) {

					std::cout << "Failed to send image.\n";

					break;
				}

				std::cout << "Sent image bytes.\n";

				if (!sendOffsets("C:\\Users\\grgic\\Desktop\\dawn\\offsets.txt", connection)) {

					std::cout << "Failed to send offsets.\n";

					break;
				}

				std::cout << "Sent offsets.\n";

				break;
			}
			case REGISTER:
			{
				User user{ connection, request.name, request.password, database.get(), request.key };

				std::string response = user.registerUser();

				if (response == "User successfully registered")
					std::cout << "User " << user.getName() << " registered.\n";

				if (send(connection, response.c_str(), response.length(), 0) == SOCKET_ERROR)
					break;

				break;
			}
			case ADDKEY:
			{
				std::string adminName = "Filip", adminPassword = "mojaSifra";

				std::string response = "Error";

				if (adminName == request.name && adminPassword == request.password) {

					response = database.addKey(request.key);

					if (response == "Key added")
						std::cout << response << std::endl;
				}

				if (send(connection, response.c_str(), response.length(), 0) == SOCKET_ERROR)
					break;

				break;
			}
			case VALIDATE:
			{
				std::string adminName = "Filip", adminPassword = "mojaSifra";

				std::string response = "Error";

				if (adminName == request.name && adminPassword == request.password) {

					User user{ connection, request.extra, nullptr, database.get(), request.key };

					user.setKeyValid();

					response = "Key validated";
				}
				
				if (send(connection, response.c_str(), response.length(), 0) == SOCKET_ERROR)
					break;

				break;
			}
			default:
			{
				std::string response = "Unknown request";

				if (send(connection, response.c_str(), response.length(), 0) == SOCKET_ERROR)
					break;

				break;
			}
		}

		database.close();

	}
	catch (std::exception& ex) {

		std::cerr << ex.what() << std::endl;
	}
}

int main() {

	try {

		WinsockServer server{ "8401", handleConnection };

		std::thread invalidateKeys{ invalidator };

		while (true) {

			server.processConnection();
		}
	}
	catch (std::exception& ex) {

		std::cerr << ex.what();

		return -1;
	}

	return 0;
}