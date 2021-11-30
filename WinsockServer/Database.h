#pragma once

#include "sqlite3.h"
#include <string>
#include <stdexcept>
#include <optional>

class Database
{
	sqlite3* sql = nullptr;
	std::string name;

public:
	
	Database(const std::string& databaseName) : name{ databaseName } {

		if (sqlite3_open_v2(databaseName.c_str(), &sql, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr) != SQLITE_OK) {
			throw std::runtime_error("sqlite3_open_v2 failed with code " + std::to_string(sqlite3_errcode(sql)));
		}

		std::string userTable {
			"CREATE TABLE IF NOT EXISTS users("
			"name		VARCHAR(50)	   NOT NULL, "
			"password	VARCHAR(50)	   NOT NULL, "
			"code		VARCHAR(50)	   NOT NULL)"
		};

		char* msg = nullptr;

		if (sqlite3_exec(sql, userTable.c_str(), nullptr, nullptr, &msg) != SQLITE_OK) {
			
			std::string error = msg;

			sqlite3_free(msg);

			sqlite3_close_v2(sql);

			throw std::runtime_error("sqlite3_exec failed with message " + error);
		}

		std::string keyTable {
			"CREATE TABLE IF NOT EXISTS keys("
			"name		   VARCHAR(50)  NOT NULL, "
			"used		   INT          NOT NULL, "
			"valid		   INT 			NOT NULL, "
			"lastValidated DATE)"
		};

		if (sqlite3_exec(sql, keyTable.c_str(), nullptr, nullptr, &msg) != SQLITE_OK) {

			std::string error = msg;

			sqlite3_free(msg);

			sqlite3_close_v2(sql);

			throw std::runtime_error("sqlite3_exec failed with message " + error);
		}

		sqlite3_close_v2(sql);
	}

	void open() {

		if (sqlite3_open_v2(name.c_str(), &sql, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr) != SQLITE_OK) {

			throw std::runtime_error("sqlite3_open_v2 failed with code " + std::to_string(sqlite3_errcode(sql)));
		}
	}

	std::string addKey(const std::string& key) {

		if (key.length() < 8)
			return std::string("Key too short");

		if (key.length() > 25)
			return std::string("Key too long");

		std::string query{
		"SELECT name FROM keys WHERE name = ?;"
		};

		sqlite3_stmt* stmt;

		if (sqlite3_prepare_v2(sql, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
			throw std::runtime_error("sqlite3_prepare_v2 failed with message " + std::to_string(sqlite3_errcode(sql)));

		if (sqlite3_bind_text(stmt, 1, key.c_str(), -1, nullptr) != SQLITE_OK) {

			sqlite3_finalize(stmt);

			throw std::runtime_error("sqlite3_bind_text failed with message " + std::to_string(sqlite3_errcode(sql)));
		}

		int status = sqlite3_step(stmt);

		if (status != SQLITE_ROW && status != SQLITE_DONE) {

			sqlite3_finalize(stmt);

			throw std::runtime_error("sqlite3_step failed with message " + std::to_string(sqlite3_errcode(sql)));
		}

		if (status == SQLITE_ROW) {

			sqlite3_finalize(stmt);

			return std::string("Key already exists");
		}

		sqlite3_finalize(stmt);

		query = "INSERT INTO keys VALUES(?, 0, 0, NULL);";

		if (sqlite3_prepare_v2(sql, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
			throw std::runtime_error("sqlite3_prepare_v2 failed with code " + std::string(sqlite3_errmsg(sql)));

		if (sqlite3_bind_text(stmt, 1, key.c_str(), -1, nullptr) != SQLITE_OK) {

			sqlite3_finalize(stmt);

			throw std::runtime_error("sqlite3_bind_text failed with message " + std::to_string(sqlite3_errcode(sql)));
		}

		if (sqlite3_step(stmt) != SQLITE_DONE) {

			sqlite3_finalize(stmt);

			throw std::runtime_error("sqlite3_step failed with message " + std::to_string(sqlite3_errcode(sql)));
		}
		
		sqlite3_finalize(stmt);

		return std::string("Key added");
	}

	std::optional<std::vector<std::string>> invalidate() {
		
		std::string query{
			"SELECT name FROM keys WHERE valid = 1 AND julianday('now') - julianday(lastValidated) >= 30;"
		};

		sqlite3_stmt* stmt;

		if (sqlite3_prepare_v2(sql, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
			throw std::runtime_error("sqlite3_prepare_v2 failed with code " + std::string(sqlite3_errmsg(sql)));

		int ret{};

		std::vector<std::string> names;

		while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {

			names.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
		}

		sqlite3_finalize(stmt);

		if (names.empty())
			return {};

		query = "UPDATE keys SET valid = 0 WHERE valid = 1 AND julianday('now') - julianday(lastValidated) >= 30;";
		
		char* msg;

		if (sqlite3_exec(sql, query.c_str(), nullptr, nullptr, &msg) != SQLITE_OK) {

			std::string error = msg;

			sqlite3_free(msg);

			throw std::runtime_error("sqlite3_exec failed with message " + error);
		}

		return names;
	}

	void close() {

		sqlite3_close_v2(sql);
	}

	Database(const Database& other)				= delete;

	Database& operator=(const Database& other)	= delete;

	sqlite3* get() const {

		return sql;
	}
};

