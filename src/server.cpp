// Linux Multi-threaded Redis Server
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define closesocket close

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include "../include/resp_parser.h"
#include "../include/command_handler.h"
#include "../include/storage.h"
using namespace std;

// Global components (SOLID principle - each has one responsibility)
Storage storage;               // Handles data storage
mutex mtx;                     // Thread safety for connection counting

string HOST = "0.0.0.0";
int PORT = 7379;
int clientCount = 0;

// Read from socket (like cin >> input)
string readFromClient(int sock) {
    char buf[512];
    int n = recv(sock, buf, 512, 0);
    if (n <= 0) return "";
    return string(buf, buf + n);
}

// Write to socket (like cout << output)
void writeToClient(int sock, string data) {
    send(sock, data.c_str(), data.size(), 0);
}

// Handle one client (runs in separate thread)
// RESPONSIBILITY: Network I/O only - receive and send bytes
void handleOneClient(int sock, string ip) {
    // Client connected
    {
        lock_guard<mutex> lock(mtx);
        clientCount++;
        cout << "Connected: " << ip << " | Total: " << clientCount << endl;
    }
    
    // Create components for this client
    RespParser parser;
    CommandHandler handler(storage);
    
    // Keep reading and processing commands
    while (true) {
        // 1. Receive data (Network I/O)
        string msg = readFromClient(sock);
        if (msg.empty()) break;  // Client disconnected
        
        cout << ip << ": " << msg << endl;
        
        // 2. Parse RESP (Delegation to RespParser)
        RespValue cmd = parser.decode(msg);
        
        // 3. Handle command (Delegation to CommandHandler)
        string response = handler.handleCommand(cmd);
        
        // 4. Send response (Network I/O)
        writeToClient(sock, response);
    }
    
    // Client disconnected
    closesocket(sock);
    {
        lock_guard<mutex> lock(mtx);
        clientCount--;
        cout << "Disconnected: " << ip << " | Total: " << clientCount << endl;
    }
}

int main() {
    // Display RED REDIS banner
    cout << "\033[1;31m" << endl;
    cout << "██████╗ ███████╗██████╗     ██████╗ ███████╗██████╗ ██╗███████╗" << endl;
    cout << "██╔══██╗██╔════╝██╔══██╗    ██╔══██╗██╔════╝██╔══██╗██║██╔════╝" << endl;
    cout << "██████╔╝█████╗  ██║  ██║    ██████╔╝█████╗  ██║  ██║██║███████╗" << endl;
    cout << "██╔══██╗██╔══╝  ██║  ██║    ██╔══██╗██╔══╝  ██║  ██║██║╚════██║" << endl;
    cout << "██║  ██║███████╗██████╔╝    ██║  ██║███████╗██████╔╝██║███████║" << endl;
    cout << "╚═╝  ╚═╝╚══════╝╚═════╝     ╚═╝  ╚═╝╚══════╝╚═════╝ ╚═╝╚══════╝" << endl;
    cout << "\033[0m" << endl;
    cout << "\033[1;33mRedis Clone Server v1.0\033[0m" << endl;
    cout << "\033[1;33mPort: " << PORT << "\033[0m" << endl;
    cout << "\033[1;32mReady to accept connections...\033[0m" << endl;
    cout << endl;
    
    // 1. Create server socket
    int server = socket(AF_INET, SOCK_STREAM, 0);
    
    // 2. Setup address (like filling a struct)
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, HOST.c_str(), &addr.sin_addr);
    
    // 3. Bind socket to port
    bind(server, (sockaddr*)&addr, sizeof(addr));
    
    // 4. Start listening
    listen(server, 10);
    
    // 5. Accept clients in loop (like while(t--) in CP)
    while (true) {
        sockaddr_in clientAddr = {};
        int len = sizeof(clientAddr);
        
        int client = accept(server, (sockaddr*)&clientAddr, &len);
        
        char ip[16];
        inet_ntop(AF_INET, &clientAddr.sin_addr, ip, 16);
        
        // Spawn thread for this client (like async task)
        thread(handleOneClient, client, string(ip)).detach();
    }
    
    return 0;
}
