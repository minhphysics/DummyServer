#ifndef __SOCKET_H
#define __SOCKET_H

#include <cstdint>
#include <iostream>
#include <functional>
#include <string>
#include <map>

#define MAX_CONNECTION 10
#define MAX_FILE_SIZE	4096
#define MAX_MESSAGE_SIZE 4096
#define HEADER_SIZE 128

enum class MsgType {
	TYPE_ATTACHMENT,
	TYPE_MESSAGE,
	TYPE_NONE,
};

class Header
{
public:
	MsgType	type;
	std::string file_name;
	uint16_t size; // size of message or file
};

class Socket
{
private:
	int sockfd;
	int connfd;
	struct sockaddr_in *serv_addr;
	std::string ip_addr;
	uint16_t port;
	std::string save_file_path;
	std::string message;
	Header header;
	bool verbose;
	bool is_server;

	void serve();
	void request();
	std::string composeHeader(Header &header);
	bool isServerAddressAvailable();
	int parseHeader(char* buf, std::map<std::string, std::string>& map);


public:
	Socket(const std::string& _ip = "0.0.0.0",
  	       uint16_t _port=8080,
		bool _verbose=false,
		bool _is_server=false);	
	~Socket();
	bool isVerbose();
	bool isServer();
	int sendMessage(std::string& message);
	int sendFile(const std::string& path);
	std::string showConnection();
	int receiveMessage(std::string& message);
	int receiveFile(std::string path);
	int receiveAll(std::string& path);
};

#endif //__SOCKET_H
