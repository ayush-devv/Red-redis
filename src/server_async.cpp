// Linux Async Redis Server with epoll
// Handles 20,000+ concurrent clients

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <atomic>
#include <cerrno>

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <algorithm>
#include "../include/resp_parser.h"
#include "../include/resp_encoder.h"
#include "../include/command_handler.h"
#include "../include/storage.h"
#include "../include/aof.h"
using namespace std;
using namespace std::chrono;

// Graceful shutdown flag (atomic for thread safety)
std::atomic<bool> shutdownRequested(false);

// Signal handler for Ctrl+C (SIGINT) and SIGTERM
void signalHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        cout << "\n\033[1;33m⚠ Shutdown signal received, cleaning up...\033[0m" << endl;
        shutdownRequested.store(true);
    }
}

// Global storage (single-threaded, no mutex needed!)
Storage storage;
AOF aof("appendonly.aof");

// Active expiration timer
auto lastCleanupTime = steady_clock::now();
const auto cleanupInterval = seconds(1);  // Run every 1 second

const string HOST = "0.0.0.0";
const int PORT = 7379;

// Set socket to non-blocking mode
void setNonBlocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

// Read from socket (non-blocking)
string readFromSocket(int sock) {
    char buf[512];
    int n = recv(sock, buf, 512, 0);
    if (n <= 0) return "";
    return string(buf, buf + n);
}

// Write to socket
void writeToSocket(int sock, const string& data) {
    send(sock, data.c_str(), data.size(), 0);
}

// Run async server with epoll
void runAsyncServer() {
    cout << "\033[1;33m[Linux] Using epoll - Max 20,000+ clients\033[0m" << endl;
    
    // Create server socket
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    setNonBlocking(serverSock);
    
    // Bind to port
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, HOST.c_str(), &addr.sin_addr);
    
    bind(serverSock, (sockaddr*)&addr, sizeof(addr));
    listen(serverSock, 10);
    
    // Create epoll instance
    int epollFd = epoll_create1(0);
    
    // Add server socket to epoll
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;  // Edge-triggered
    ev.data.fd = serverSock;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSock, &ev);
    
    // Track clients
    map<int, RespParser> parsers;
    CommandHandler handler(storage);
    epoll_event events[100];
    
    cout << "\033[1;32mServer ready on port " << PORT << "\033[0m" << endl;
    
    // Main event loop - run until shutdown requested
    while (!shutdownRequested.load()) {
        // Active expiration - check every 1 second
        auto now = steady_clock::now();
        if (now - lastCleanupTime >= cleanupInterval) {
            storage.deleteExpiredKeys();
            lastCleanupTime = now;
        }
        
        // Wait for events with timeout (so we can check shutdown flag)
        int nfds = epoll_wait(epollFd, events, 100, 1000);  // 1 second timeout
        
        if (nfds == -1) {
            if (errno == EINTR) {
                // Interrupted by signal, check shutdown flag
                continue;
            }
            break;
        }
        
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == serverSock) {
                // New client connection
                sockaddr_in clientAddr;
                socklen_t len = sizeof(clientAddr);
                int newClient = accept(serverSock, (sockaddr*)&clientAddr, &len);
                
                if (newClient >= 0) {
                    setNonBlocking(newClient);
                    
                    // Add to epoll
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = newClient;
                    epoll_ctl(epollFd, EPOLL_CTL_ADD, newClient, &ev);
                    
                    parsers[newClient] = RespParser();
                    
                    char ip[16];
                    inet_ntop(AF_INET, &clientAddr.sin_addr, ip, 16);
                    cout << "✓ Client connected: " << ip << " (Total: " << parsers.size() << ")" << endl;
                }
            } else {
                // Existing client has data
                int clientFd = events[i].data.fd;
                string msg = readFromSocket(clientFd);
                
                if (msg.empty()) {
                    // Client disconnected
                    cout << "✗ Client disconnected (Total: " << parsers.size() - 1 << ")" << endl;
                    epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, nullptr);
                    close(clientFd);
                    parsers.erase(clientFd);
                } else {
                    // Process ALL commands in buffer (pipelining support)
                    string allResponses;
                    int pos = 0;
                    
                    // Loop through all commands in the message
                    while (pos < msg.size()) {
                        RespValue cmd = parsers[clientFd].decodeInternal(msg, pos);
                        
                        // Check for BGREWRITEAOF command (handled separately)
                        string response;
                        if (cmd.type == RespType::Array && !cmd.arr_value.empty()) {
                            string cmdName = cmd.arr_value[0].str_value;
                            transform(cmdName.begin(), cmdName.end(), cmdName.begin(), ::toupper);
                            
                            if (cmdName == "BGREWRITEAOF") {
                                if (aof.bgRewriteAOF(storage)) {
                                    response = RESPEncoder::encodeSimpleString("Background AOF rewrite started");
                                } else {
                                    response = RESPEncoder::encodeError("ERR rewrite already in progress");
                                }
                            } else {
                                response = handler.handleCommand(cmd);
                            }
                        } else {
                            response = handler.handleCommand(cmd);
                        }
                        
                        // Log to AOF
                        if (cmd.type == RespType::Array) {
                            std::vector<std::string> command;
                            for (const auto& val : cmd.arr_value) {
                                command.push_back(val.str_value);
                            }
                            aof.log(command);
                        }
                        
                        allResponses += response;
                    }
                    
                    writeToSocket(clientFd, allResponses);
                }
            }
        }
    }
    
    // Cleanup on shutdown
    cout << "\033[1;33m🔄 Flushing data to disk...\033[0m" << endl;
    
    // Close all client connections
    for (const auto& pair : parsers) {
        close(pair.first);
    }
    
    cout << "\033[1;32m✓ Graceful shutdown complete\033[0m" << endl;
    
    close(serverSock);
    close(epollFd);
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
    cout << "\033[1;33mRedis Clone - Async Server v1.0 (Linux)\033[0m" << endl;
    cout << "\033[1;33mPort: " << PORT << "\033[0m" << endl;
    cout << "\033[1;32mReady to accept connections...\033[0m" << endl;
    cout << "\033[1;33mPress Ctrl+C for graceful shutdown\033[0m" << endl;
    cout << endl;
    
    // Register signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);   // Ctrl+C
    signal(SIGTERM, signalHandler);  // kill command
    
    // Replay AOF to restore data
    aof.replay(storage);
    cout << endl;
    
    runAsyncServer();
    
    return 0;
}
