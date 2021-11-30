#pragma once

#include <string>
#include "sqlite3.h"
#include <stdexcept>
#include <vector>
#include <WinSock2.h>
#include <ws2tcpip.h>

class User
{
	std::string		name;
	std::string		password;
	std::string		code;
	SOCKET			connection;
	sqlite3*		sql;

public:

	User(SOCKET connection, const std::string& name = "", const std::string& password = "", sqlite3* sql = nullptr, const std::string& code = "");

	std::string registerUser();

	std::string login(std::vector<User>& usersLoggedIn);

	std::string logout(std::vector<User>& usersLoggedIn);

	bool userExists();

	bool isLoggedIn(const std::vector<User>& usersLoggedIn);

	bool correctPassword();


	std::string getUserKey();

	void setKeyUsed();

	void setKeyValid();

	bool keyExists();

	bool isKeyUsed();

	bool isKeyValid();

	std::string getName() const {
		return name;
	}

	std::string getPassword() const {
		return password;
	}

	std::string getCode() const {
		return code;
	}

	SOCKET getConnection() const {
		return connection;
	}
};

