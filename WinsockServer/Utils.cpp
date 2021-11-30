#include "Utils.h"
#include <fstream>
#include <filesystem>

bool getImageBytes(const std::string& imagePath, std::vector<char>& imageBytes) {

	if (!std::filesystem::exists(imagePath))
		return false;

	std::ifstream ifile{ imagePath, std::ios::binary };

	imageBytes.assign(std::istreambuf_iterator<char>(ifile), std::istreambuf_iterator<char>());

	ifile.close();

	return true;
}

bool sendImage(const std::string& path, SOCKET connection)
{
	std::vector<char> imageBytes;

	if (getImageBytes(path, imageBytes)) {

		auto size = imageBytes.size();

		if (send(connection, reinterpret_cast<const char*>(&size), sizeof(size), 0) == SOCKET_ERROR)
			return false;

		if (send(connection, imageBytes.data(), size, 0) == SOCKET_ERROR)
			return false;

		return true;
	}

	return false;
}

bool getOffsets(const std::string& path, std::vector<uintptr_t>& offsets) {

	if (!std::filesystem::exists(path))
		return false;

	std::ifstream ifile{ path, std::ios::beg };

	std::string str;

	while (std::getline(ifile, str)) {

		unsigned int offset;

		if (sscanf_s(str.c_str(), "%*s %x", &offset) != 1) {

			ifile.close();

			return false;
		}

		offsets.push_back(static_cast<uintptr_t>(offset));
	}

	ifile.close();

	return true;
}

bool sendOffsets(const std::string& path, SOCKET connection) {

	std::vector<uintptr_t> offsets;

	if (getOffsets(path, offsets)) {

		if (send(connection, reinterpret_cast<const char*>(offsets.data()), offsets.size() * sizeof(uintptr_t), 0) == SOCKET_ERROR)
			return false;

		return true;
	}

	return false;
}
