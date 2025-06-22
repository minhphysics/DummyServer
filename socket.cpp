#include "socket.hpp"
#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <filesystem>
#include <fstream>
#include <netinet/in.h>
#include <sstream>
#include <arpa/inet.h>

Socket::Socket(const std::string& _ip, uint16_t _port, bool _verbose, bool _is_server)
 : ip_addr(_ip), port(_port), verbose(_verbose), is_server(_is_server)
{
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		throw std::runtime_error("socket creation failed");
	else if (verbose)
		std::cout << "Socket successfully created" << std::endl;

	serv_addr = new sockaddr_in;
	memset(serv_addr, 0, sizeof(sockaddr_in));

	serv_addr->sin_family = AF_INET;
	serv_addr->sin_port = htons(port);
	if (ip_addr == "0.0.0.0")
		serv_addr->sin_addr.s_addr = htonl(INADDR_ANY);
	else if (inet_pton(AF_INET, ip_addr.c_str(), &(serv_addr->sin_addr)) <= 0)
		throw std::runtime_error("Invalid IP address");

	is_server ? serve() : request();
}

Socket::~Socket()
{
	if (serv_addr)
		delete serv_addr;
	if (sockfd >= 0)
		close(sockfd);
}

bool Socket::isVerbose() { return verbose; }
bool Socket::isServer() { return is_server; }

void Socket::serve()
{
	if (bind(sockfd, (struct sockaddr*)serv_addr, sizeof(sockaddr_in)) < 0)
		throw std::runtime_error("socket bind failed");
	if (verbose)
		std::cout << "Socket successfully bound" << std::endl;

	if (listen(sockfd, 1) < 0)
		throw std::runtime_error("listen failed");
}

void Socket::request()
{
	if (connect(sockfd, (struct sockaddr*)serv_addr, sizeof(sockaddr_in)) < 0)
		throw std::runtime_error("connect failed");
	if (verbose)
		std::cout << "Connected to server successfully" << std::endl;

	connfd = sockfd;
}

std::string Socket::composeHeader(Header& header)
{
	std::ostringstream oss;
	oss << "fileType:" << (header.type == MsgType::TYPE_ATTACHMENT ? "attachment" : "message")
		<< ",fileName:" << header.file_name
		<< ",fileSize:" << std::to_string(header.size) << ";";

	std::string head = oss.str();
	head.resize(HEADER_SIZE, ' ');
	return head;
}

bool Socket::isServerAddressAvailable()
{
	return serv_addr && serv_addr->sin_port != 0 && serv_addr->sin_addr.s_addr != 0;
}

int Socket::sendMessage(std::string& message)
{
	if (!isServerAddressAvailable() || message.size() > MAX_MESSAGE_SIZE)
		return -1;

	Header header{ MsgType::TYPE_MESSAGE, static_cast<uint16_t>(message.size()), "null" };
	std::string socket_stream = composeHeader(header) + message;

	if (is_server) {
		connfd = accept(sockfd, nullptr, nullptr);
		if (connfd < 0) return -1;
	}
	return send(connfd, socket_stream.c_str(), socket_stream.size(), 0);
}

int Socket::sendFile(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) return -1;

	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	if (size > MAX_FILE_SIZE) return -1;
	file.seekg(0);

	std::string content(size, '\0');
	file.read(&content[0], size);
	file.close();

	Header header{ MsgType::TYPE_ATTACHMENT, static_cast<uint16_t>(size), std::filesystem::path(path).filename().string() };
	std::string socket_stream = composeHeader(header) + content;

	if (is_server) {
		connfd = accept(sockfd, nullptr, nullptr);
		if (connfd < 0) return -1;
	}
	return send(connfd, socket_stream.c_str(), socket_stream.size(), 0);
}

int Socket::receiveAll(std::string& path)
{
	Header header;
	std::map<std::string, std::string> header_map;
	std::vector<char> buffer(MAX_MESSAGE_SIZE);

	if (is_server) {
		connfd = accept(sockfd, nullptr, nullptr);
		if (connfd < 0) return -1;
	}
	ssize_t received = recv(connfd, buffer.data(), buffer.size(), 0);
	if (received <= 0) return -1;

	if (parseHeader(buffer.data(), header_map)) return -1;

	char* msg_buf = buffer.data() + HEADER_SIZE;
	header.type = (header_map["fileType"] == "message") ? MsgType::TYPE_MESSAGE : MsgType::TYPE_ATTACHMENT;
	header.size = static_cast<uint16_t>(std::stoi(header_map["fileSize"]));
	header.file_name = header_map["fileName"];

	if (header.type == MsgType::TYPE_ATTACHMENT) {
		std::ofstream outFile(path + "/" + header.file_name, std::ios::binary);
		if (!outFile) return -1;
		outFile.write(msg_buf, header.size);
	} else {
		std::string message(msg_buf, header.size);
		if (verbose) std::cout << "Message: " << message << std::endl;
	}
	return 0;
}

int Socket::parseHeader(char* buf, std::map<std::string, std::string>& map)
{
	if (!buf) return -1;
	std::string str(buf, HEADER_SIZE);
	std::stringstream ss(str);
	std::string pair;

	while (std::getline(ss, pair, ',')) {
		size_t colon = pair.find(':');
		if (colon != std::string::npos) {
			std::string key = pair.substr(0, colon);
			std::string val = pair.substr(colon + 1);
			if (!val.empty() && val.back() == ';') val.pop_back();
			map[key] = val;
		}
	}
	return 0;
}
