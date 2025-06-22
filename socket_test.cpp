#include "socket.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

void run_server(std::string& ip, uint16_t port, std::string& save_path) {
    try {
        Socket server(ip, port, true, true);

        while (true) {
            std::cout << "[Server] Waiting for a client...\n";
            if (server.receiveAll(save_path) == 0) {
                std::cout << "[Server] Received successfully.\n";
            } else {
                std::cerr << "[Server] Failed to receive.\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[Server] Exception: " << e.what() << std::endl;
    }
}

void run_client(std::string& ip, uint16_t port, std::string& message, std::string& file_path) {
    try {
        Socket client(ip, port, true, false);

        std::cout << "[Client] Sending message...\n";
        std::string msg = message;
        if (client.sendMessage(msg))
            std::cout << "[Client] Message sent successfully.\n";
        else
            std::cerr << "[Client] Failed to send message.\n";

        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::cout << "[Client] Sending file...\n";
        if (client.sendFile(file_path))
            std::cout << "[Client] File sent successfully.\n";
        else
            std::cerr << "[Client] Failed to send file.\n";

    } catch (const std::exception& e) {
        std::cerr << "[Client] Exception: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " server <ip> <port> <save_path>\n"
                  << "   or: " << argv[0] << " client <ip> <port> <message> <file_path>\n";
        return 1;
    }

    std::string mode = argv[1];
    std::string ip = argv[2];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[3]));

    if (mode == "server" && argc == 5) {
        std::string save_path = argv[4];
        run_server(ip, port, save_path);
    } else if (mode == "client" && argc == 6) {
        std::string message = argv[4];
        std::string file_path = argv[5];
        run_client(ip, port, message, file_path);
    } else {
        std::cerr << "Invalid arguments.\n";
        return 1;
    }

    return 0;
}
