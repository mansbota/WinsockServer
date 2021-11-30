#include "User.h"

User::User(SOCKET connection, const std::string& name, const std::string& password, sqlite3* sql, const std::string& code) :
	connection{ connection }, name { name }, password{ password }, code{ code }, sql{ sql } {}

std::string User::registerUser() {

	if (name.length() < 5)
		return std::string("Name too short");

	if (password.length() < 5)
		return std::string("Password too short");

	if (code.length() < 5)
		return std::string("Code too short");

	if (name.length() > 31)
		return std::string("Name too long");

	if (password.length() > 31)
		return std::string("Password too long");

	if (code.length() > 31)
		return std::string("Code too long");

	if (userExists())
		return std::string("User already exists");

	if (!keyExists())
		return std::string("Key doesn't exist");

	if (isKeyUsed())
		return std::string("Key already in use");

	std::string insert{
		"INSERT INTO users VALUES(?, ?, ?);"
	};

	sqlite3_stmt* stmt = nullptr;

	if (sqlite3_prepare_v2(sql, insert.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
		throw std::runtime_error("sqlite3_prepare_v2 failed with message " + std::to_string(sqlite3_errcode(sql)));

	if (sqlite3_bind_text(stmt, 1, name.c_str(), -1, nullptr) != SQLITE_OK) {

		sqlite3_finalize(stmt);

		throw std::runtime_error("sqlite3_bind_text failed with message " + std::to_string(sqlite3_errcode(sql)));
	}

	if (sqlite3_bind_text(stmt, 2, password.c_str(), -1, nullptr) != SQLITE_OK) {

		sqlite3_finalize(stmt);

		throw std::runtime_error("sqlite3_bind_text failed with message " + std::to_string(sqlite3_errcode(sql)));
	}

	if (sqlite3_bind_text(stmt, 3, code.c_str(), -1, nullptr) != SQLITE_OK) {

		sqlite3_finalize(stmt);

		throw std::runtime_error("sqlite3_bind_text failed with message " + std::to_string(sqlite3_errcode(sql)));
	}

	int status = sqlite3_step(stmt);

	if (status != SQLITE_DONE) {

		sqlite3_finalize(stmt);

		throw std::runtime_error("sqlite3_step failed with message " + std::to_string(sqlite3_errcode(sql)));
	}

	sqlite3_finalize(stmt);

	setKeyUsed();

	setKeyValid();

	return std::string("User successfully registered");
}

std::string User::login(std::vector<User>& usersLoggedIn) {

	if (name.length() < 5)
		return std::string("Name too short");

	if (password.length() < 5)
		return std::string("Password too short");

	if (name.length() > 31)
		return std::string("Name too long");

	if (password.length() > 31)
		return std::string("Password too long");

	if (!userExists())
		return std::string("User doesn't exist");

	code = getUserKey();

	if (!keyExists())
		return std::string("Key doesn't exist");

	if (!isKeyValid())
		return std::string("Key expired");

	if (!correctPassword())
		return std::string("Wrong password");

	if (isLoggedIn(usersLoggedIn))
		return std::string("Already logged in");

	usersLoggedIn.push_back(User{ connection, name, password, sql, code });

	return std::string("Logged in");
}

bool User::correctPassword() {

	std::string query{
		"SELECT password FROM users WHERE name = ?;"
	};

	sqlite3_stmt* stmt;

	if (sqlite3_prepare_v2(sql, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
		throw std::runtime_error("sqlite3_prepare_v2 failed with message " + std::to_string(sqlite3_errcode(sql)));

	if (sqlite3_bind_text(stmt, 1, name.c_str(), -1, nullptr) != SQLITE_OK) {

		sqlite3_finalize(stmt);

		throw std::runtime_error("sqlite3_bind_text failed with message " + std::to_string(sqlite3_errcode(sql)));
	}

	int status = sqlite3_step(stmt);

	if (status != SQLITE_ROW && status != SQLITE_DONE) {

		sqlite3_finalize(stmt);

		throw std::runtime_error("sqlite3_step failed with message " + std::to_string(sqlite3_errcode(sql)));
	}

	if (status == SQLITE_DONE) {

		sqlite3_finalize(stmt);

		return false;
	}

	std::string result = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

	sqlite3_finalize(stmt);

	return result == password;
}

std::string User::logout(std::vector<User>& usersLoggedIn) {

	auto user = std::find_if(usersLoggedIn.begin(), usersLoggedIn.end(), [&](const User& user) { return user.getName() == name; });

	usersLoggedIn.erase(user);

	return std::string("Logged out");
}

void User::setKeyUsed() {

	std::string query{
		"UPDATE keys SET used = 1 WHERE name = ?;"
	};

	sqlite3_stmt* stmt = nullptr;

	if (sqlite3_prepare_v2(sql, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
		throw std::runtime_error("sqlite3_prepare_v2 failed with message " + std::to_string(sqlite3_errcode(sql)));

	if (sqlite3_bind_text(stmt, 1, code.c_str(), -1, nullptr) != SQLITE_OK) {

		sqlite3_finalize(stmt);

		throw std::runtime_error("sqlite3_bind_text failed with message " + std::to_string(sqlite3_errcode(sql)));
	}

	if (sqlite3_step(stmt) != SQLITE_DONE) {

		sqlite3_finalize(stmt);

		throw std::runtime_error("sqlite3_step failed with message " + std::to_string(sqlite3_errcode(sql)));
	}

	sqlite3_finalize(stmt);
}

void User::setKeyValid() {

	std::string query{
		"UPDATE keys SET valid = 1, lastValidated = date('now') WHERE name = ?;"
	};

	sqlite3_stmt* stmt = nullptr;

	if (sqlite3_prepare_v2(sql, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
		throw std::runtime_error("sqlite3_prepare_v2 failed with message " + std::to_string(sqlite3_errcode(sql)));

	if (sqlite3_bind_text(stmt, 1, code.c_str(), -1, nullptr) != SQLITE_OK) {

		sqlite3_finalize(stmt);

		throw std::runtime_error("sqlite3_bind_text failed with message " + std::to_string(sqlite3_errcode(sql)));
	}

	if (sqlite3_step(stmt) != SQLITE_DONE) {

		sqlite3_finalize(stmt);

		throw std::runtime_error("sqlite3_step failed with message " + std::to_string(sqlite3_errcode(sql)));
	}

	sqlite3_finalize(stmt);
}

bool User::userExists() {

	std::string query{
		"SELECT name FROM users WHERE name = ?;"
	};

	sqlite3_stmt* stmt;

	if (sqlite3_prepare_v2(sql, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
		throw std::runtime_error("sqlite3_prepare_v2 failed with message " + std::to_string(sqlite3_errcode(sql)));

	if (sqlite3_bind_text(stmt, 1, name.c_str(), -1, nullptr) != SQLITE_OK) {

		sqlite3_finalize(stmt);

		throw std::runtime_error("sqlite3_bind_text failed with message " + std::to_string(sqlite3_errcode(sql)));
	}

	int status = sqlite3_step(stmt);

	if (status != SQLITE_ROW && status != SQLITE_DONE) {

		sqlite3_finalize(stmt);

		throw std::runtime_error("sqlite3_step failed with message " + std::to_string(sqlite3_errcode(sql)));
	}

	if (status == SQLITE_DONE) {

		sqlite3_finalize(stmt);

		return false;
	}

	sqlite3_finalize(stmt);

	return true;
}

bool User::keyExists() {

	std::string query{
		"SELECT name FROM keys WHERE name = ?;"
	};

	sqlite3_stmt* stmt;

	if (sqlite3_prepare_v2(sql, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
		throw std::runtime_error("sqlite3_prepare_v2 failed with message " + std::to_string(sqlite3_errcode(sql)));

	if (sqlite3_bind_text(stmt, 1, code.c_str(), -1, nullptr) != SQLITE_OK) {

		sqlite3_finalize(stmt);

		throw std::runtime_error("sqlite3_bind_text failed with message " + std::to_string(sqlite3_errcode(sql)));
	}

	int status = sqlite3_step(stmt);

	if (status != SQLITE_ROW && status != SQLITE_DONE) {

		sqlite3_finalize(stmt);

		throw std::runtime_error("sqlite3_step failed with message " + std::to_string(sqlite3_errcode(sql)));
	}

	if (status == SQLITE_DONE) {

		sqlite3_finalize(stmt);

		return false;
	}

	sqlite3_finalize(stmt);

	return true;
}

bool User::isKeyUsed() {

	std::string query{
		"SELECT used FROM keys WHERE name = ?;"
	};

	sqlite3_stmt* stmt;

	if (sqlite3_prepare_v2(sql, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
		throw std::runtime_error("sqlite3_prepare_v2 failed with message " + std::to_string(sqlite3_errcode(sql)));

	if (sqlite3_bind_text(stmt, 1, code.c_str(), -1, nullptr) != SQLITE_OK) {

		sqlite3_finalize(stmt);

		throw std::runtime_error("sqlite3_bind_text failed with message " + std::to_string(sqlite3_errcode(sql)));
	}

	int status = sqlite3_step(stmt);

	if (status != SQLITE_ROW && status != SQLITE_DONE) {

		sqlite3_finalize(stmt);

		throw std::runtime_error("sqlite3_step failed with message " + std::to_string(sqlite3_errcode(sql)));
	}

	int isUsed = sqlite3_column_int(stmt, 0);

	sqlite3_finalize(stmt);

	return static_cast<bool>(isUsed);
}

bool User::isKeyValid() {

	std::string query{
		"SELECT valid FROM keys WHERE name = ?;"
	};

	sqlite3_stmt* stmt;

	if (sqlite3_prepare_v2(sql, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
		throw std::runtime_error("sqlite3_prepare_v2 failed with message " + std::to_string(sqlite3_errcode(sql)));

	if (sqlite3_bind_text(stmt, 1, code.c_str(), -1, nullptr) != SQLITE_OK) {

		sqlite3_finalize(stmt);

		throw std::runtime_error("sqlite3_bind_text failed with message " + std::to_string(sqlite3_errcode(sql)));
	}

	int status = sqlite3_step(stmt);

	if (status != SQLITE_ROW && status != SQLITE_DONE) {

		sqlite3_finalize(stmt);

		throw std::runtime_error("sqlite3_step failed with message " + std::to_string(sqlite3_errcode(sql)));
	}

	int isValid = sqlite3_column_int(stmt, 0);

	sqlite3_finalize(stmt);

	return static_cast<bool>(isValid);
}

std::string User::getUserKey() {

	std::string query{
		"SELECT code FROM users WHERE name = ?;"
	};

	sqlite3_stmt* stmt = nullptr;

	if (sqlite3_prepare_v2(sql, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
		throw std::runtime_error("sqlite3_prepare_v2 failed with code " + std::to_string(sqlite3_errcode(sql)));

	if (sqlite3_bind_text(stmt, 1, name.c_str(), -1, nullptr) != SQLITE_OK) {

		sqlite3_finalize(stmt);

		throw std::runtime_error("sqlite3_bind_text failed with message " + std::to_string(sqlite3_errcode(sql)));
	}

	int status = sqlite3_step(stmt);

	if (status != SQLITE_ROW && status != SQLITE_DONE) {

		sqlite3_finalize(stmt);

		throw std::runtime_error("sqlite3_step failed with message " + std::to_string(sqlite3_errcode(sql)));
	}

	std::string result = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));

	sqlite3_finalize(stmt);

	return result;
}

bool User::isLoggedIn(const std::vector<User>& usersLoggedIn) {

	auto result = std::find_if(usersLoggedIn.begin(), usersLoggedIn.end(), [&](const User& user) { return user.getName() == name; });
	
	return result != usersLoggedIn.end();
}