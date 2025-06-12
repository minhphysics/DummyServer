#include "socket.h"
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
#include <sstream>

Socket::Socket(const std::string& _ip, uint16_t _port, bool _verbose, bool _is_server)
 : ip_addr(_ip), port(_port), verbose(verbose), is_server(_is_server)
{
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		throw std::runtime_error("socket creation failed");
	else {
		if (verbose)
			std::cout << "Socket successfully created" << std::endl;
	}

	serv_addr = new sockaddr_in;
	memset(serv_addr, 0, sizeof(serv_addr));

	// Assign IP address and port
	serv_addr->sin_family = AF_INET;

	if(ip_addr == "0.0.0.0")
		serv_addr->sin_addr.s_addr = htonl(INADDR_ANY);
	else
		serv_addr->sin_addr.s_addr = inet_addr(ip_addr.c_str());

	serv_addr->sin_port = htons(port);

	if(is_server)
		serve();
	else
		request();
}

Socket::~Socket()
{
	close(sockfd);
	if(serv_addr)
		delete serv_addr;
}

bool Socket::isVerbose()
{
	return verbose;
}

bool Socket::isServer()
{
	return is_server;
}

void Socket::serve()
{
	// Binding newly created socket to given IP
	if (bind(sockfd, (struct sockaddr*)serv_addr, sizeof(sockaddr_in)) != 0)
		throw std::runtime_error("socket bind failed");
	else {
    		if(verbose)
			std::cout << "socket successfully binded" << std::endl;
	}
	// listening to the assigned socket
	listen(sockfd, 1);
}
void Socket::request()
{
	if(connect(sockfd,(struct sockaddr*)serv_addr, sizeof(sockaddr_in)) < 0)
		throw std::runtime_error("ERROR connecting");
	else {
		if(verbose)
			std::cout << "connect to server successfully" << std::endl;
	}

	connfd = sockfd;
}

std::string composeHeader(Header &header)
{
	std::ostringstream oss;
	std::string head;

	/* Compose header but 128 bytes maximum */
	if (header.type == MsgType::TYPE_ATTACHMENT) {
		oss << "fileType:attachment,fileName:" << header.file_name << ",fileSize:" << std::to_string(header.size) << ";";
		head = oss.str();
		head.resize(HEADER_SIZE);
	} else {
		oss << "fileType:message,fileName:null,fileSize:" << std::to_string(header.size) << ";";
		head = oss.str();
		head.resize(HEADER_SIZE);
	}

	return head;
}

bool Socket::isServerAddressAvailable()
{
    /* If port or address is zero, it's likely not bound */
    if (serv_addr->sin_port == 0 || serv_addr->sin_addr.s_addr == 0)
        return false;

    return true;
}

int Socket::sendMessage(std::string& message)
{
	Header header;
	std::string socket_stream;

	if (!isServerAddressAvailable()) {
		if (verbose)
			std::cout << "unbound server address" << std::endl;
		return -1;
	}

	if (message.size() > MAX_MESSAGE_SIZE) {
		if (verbose)
			std::cout << "too long message" << std::endl;
		return -1;
	}
	/* Compose message to send */
	header.type = MsgType::TYPE_MESSAGE;
	header.size = message.size();
	//header.file_name = "0";
	socket_stream = composeHeader(header) + message;

	if (is_server) {
		// get the client socket
		connfd = accept(sockfd, nullptr, nullptr);
		if (connfd < 0) {
			if (verbose)
				std::cout << "cannot find any connected socket" << std::endl;
			return -1;
		}
		// currenly only accept 1 client
		send(connfd, (const char*)socket_stream.c_str(), socket_stream.size() + 1, 0);
	} else {
		send(connfd, (const char*)socket_stream.c_str(), socket_stream.size() + 1, 0);
	}

	return 0;
}
 
int Socket::sendFile(const std::string& path)
{
	Header header;
	std::ifstream file(path, std::ios::binary);
	std::filesystem::path p(path);
	std::string socket_stream;

	if (!isServerAddressAvailable()) {
		if (verbose)
			std::cout << "unbound server address" << std::endl;
		return -1;
	}

	if (!file.is_open()) {
		std::cerr << "Failed to open file: " << path << std::endl;;
	}


	/* Read the content of file */
	file.seekg(0, std::ios::end);
	std::streamsize size = file.tellg();
	file.seekg(0);
	if (size > MAX_FILE_SIZE) {
		if (verbose)
			std::cout << "file too large" << std::endl;

		file.close();
		return -1;
	}

	std::string content(size, '\0');
	file.read(&content[0], size);

	/* Init header */
	header.type = MsgType::TYPE_ATTACHMENT;
	header.size = (uint16_t)size;
	header.file_name = p.filename();

	socket_stream = composeHeader(header) + content;

	if (is_server) {
		// get the client socket
		connfd = accept(sockfd, nullptr, nullptr);
		if (connfd < 0) {
			if (verbose)
				std::cout << "cannot find any connected socket" << std::endl;
			file.close();
			return -1;
		}
		// currenly only accept 1 client
		send(connfd, (const char*)socket_stream.c_str(), socket_stream.size() + 1, 0);
	} else {
		send(connfd, (const char*)socket_stream.c_str(), socket_stream.size() + 1, 0);
	}

	file.close();
	return 0;
}

int Socket::receiveAll(std::string& path) {
	Header header;
	std::map<std::string, std::string> header_map;
	char* buffer = new char[MAX_MESSAGE_SIZE];
	std::string message;

	if (is_server) {
		//get the client socket
		connfd = accept(sockfd, nullptr, nullptr);
		if (connfd < 0) {
			if (verbose)
				std::cout << "cannot find any connected socket" << std::endl;
			delete[] buffer;
			return -1;
		}
		// currently only accept 1 client
		recv(connfd, buffer, sizeof(buffer), 0);
	} else {
		recv(connfd, buffer, sizeof(buffer), 0);
	}

	if (parseHeader(buffer, header_map)) {
		if (verbose)
			std::cout << "cannot parse header" << std::endl;
		delete[] buffer;
		return -1;
	}

	char* msg_buf = buffer + HEADER_SIZE;
	/* Fill header */
	if (header_map["fileType"] == "message") {
		header.type = MsgType::TYPE_MESSAGE;
		header.size = static_cast<uint16_t>(std::stoi(header_map["fileSize"]));
		std::string str(msg_buf);
		message = str;
	} else if (header_map["fileType"] == "attachment") {
		header.type = MsgType::TYPE_ATTACHMENT;
		header.size = static_cast<uint16_t>(std::stoi(header_map["fileSize"]));
		header.file_name = header_map["fileName"];
		/* Save file at path */
		std::string fullPath = path + "/" + header.file_name;
		std::ofstream outFile(fullPath, std::ios::out | std::ios::binary);
		if (!outFile) {
			std::cerr << "Failed to open file: " << fullPath << std::endl;
    		}
		outFile.write(msg_buf, header.size);
		outFile.close();
	}

	delete[] buffer;
	return 0;
}
/*
std::string Socket::showConnection();
int Socket::receiveFile(std::string path);
int Socket::receive(std::string path);
*/
int Socket::parseHeader(char* buf, std::map<std::string, std::string>& map)
{
	std::string str(buf, HEADER_SIZE);
	std::stringstream ss(buf);
	std::string pair;

	if (!buf)
		return -1;

	while (std::getline(ss, pair, ',')) {
		size_t colon = pair.find(':');
		if (colon != std::string::npos) {
			std::string key = pair.substr(0, colon);
			std::string val = pair.substr(colon + 1);

			/* Check the key */
			if (key != "fileType" && key != "fileName" && key != "fileSize") {
				if (verbose)
					std::cout << "inaccurate header: " << key << std::endl;
				return -1;
			}
			/* remove trailing semicolon if any */
			if (!val.empty() && val.back() == ';')
				val.pop_back();
			map[key] = val;
        }
    }
	return 0;
}
