#include "../include/aof.h"
#include "../include/resp_encoder.h"
#include "../include/resp_parser.h"
#include <iostream>
#include <unistd.h>  // For fsync
#include <thread>
#include <chrono>

AOF::AOF(const std::string& filepath, const std::string& sync) 
    : filename(filepath), aofFile(nullptr), enabled(false), 
      syncMode(sync), rewriteChildPid(-1), rewriteInProgress(false) {
    
    // Open file in append mode ("a")
    // "a" = append, creates file if doesn't exist, writes at end
    aofFile = fopen(filename.c_str(), "a");
    
    if (aofFile == nullptr) {
        std::cerr << "Warning: Could not open AOF file: " << filename << std::endl;
        enabled = false;
        return;
    }
    
    enabled = true;
    
    // Start background fsync thread for everysec mode
    if (syncMode == "everysec") {
        running = true;
        fsyncThread = std::thread(&AOF::fsyncWorker, this);
    }
    
    std::cout << "AOF enabled: " << filename << " (mode: " << syncMode << ")" << std::endl;
}

// Destructor - close file
AOF::~AOF() {
    // Stop background thread if running
    if (running) {
        running = false;
        if (fsyncThread.joinable()) {
            fsyncThread.join();
        }
    }
    
    if (aofFile) {
        // Make sure everything is written to disk before closing
        fsync(fileno(aofFile));
        fclose(aofFile);
        aofFile = nullptr;
    }
}

// Background fsync worker for everysec mode
void AOF::fsyncWorker() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (aofFile) {
            fsync(fileno(aofFile));
        }
    }
}


// Log commands to AOF file
void AOF::log(const std::vector<std::string>& command) {
    if (!enabled || command.empty()) {
        return; // AOF not enabled or empty command
    }

    // Skip read-only commands - no need to log
    std::string cmd = command[0];
    if (cmd == "GET" || cmd == "TTL" || cmd == "EXISTS" || cmd == "PING") {
        return;
    }

    // Encode the command as per the RESP format
    std::string respCmd = RESPEncoder::encodeArray(command);

    // Write to the file
    size_t written = fwrite(respCmd.c_str(), 1, respCmd.size(), aofFile);

    if (written != respCmd.size()) {
        std::cerr << "Warning: Incomplete write to AOF file" << std::endl;
    }

    // Sync based on mode
    if (syncMode == "always") {
        fflush(aofFile);
        fsync(fileno(aofFile));  // Immediate sync (slow but 100% safe)
    } else {
        fflush(aofFile);  // Just flush to OS buffer (fast)
        // everysec: background thread handles fsync
        // no: OS decides when to fsync
    }
}

// Replay the AOF file to reconstruct the in-memory data store
void AOF::replay(Storage& storage) {
    // Try to open the file in read-only mode
    FILE* f = fopen(filename.c_str(), "r");
    if (f == nullptr) {
        std::cout << "No AOF file found, starting with empty database" << std::endl;
        return;
    }

    std::cout << "Replaying AOF file: " << filename << std::endl;
    int commandCount = 0;

    // Read entire file into string
    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (fileSize <= 0) {
        fclose(f);
        std::cout << "Empty AOF file" << std::endl;
        return;
    }
    
    std::string fileContent(fileSize, '\0');
    fread(&fileContent[0], 1, fileSize, f);
    fclose(f);
    
    // Parse commands
    RespParser parser;
    int pos = 0;
    
    while (pos < fileContent.size()) {
        try {
            RespValue result = parser.decodeInternal(fileContent, pos);
            
            if (result.type == RespType::Array && !result.arr_value.empty()) {
                // Convert RESP array to string vector
                std::vector<std::string> command;
                for (const auto& val : result.arr_value) {
                    if (val.type == RespType::BulkString) {
                        command.push_back(val.str_value);
                    }
                }
                
                // Execute command to restore state
                if (!command.empty()) {
                    std::string cmd = command[0];
                    
                    // Execute directly on storage (bypass network layer)
                    if (cmd == "SET") {
                        if (command.size() == 3) {
                            storage.set(command[1], command[2]);
                        } else if (command.size() >= 5 && command[3] == "PX") {
                            int64_t ttl = std::stoll(command[4]);
                            storage.setWithExpiry(command[1], command[2], ttl);
                        } else if (command.size() >= 5 && command[3] == "EX") {
                            int64_t seconds = std::stoll(command[4]);
                            storage.setWithExpiry(command[1], command[2], seconds * 1000);
                        }
                    } else if (cmd == "DEL") {
                        for (size_t i = 1; i < command.size(); i++) {
                            storage.del(command[i]);
                        }
                    } else if (cmd == "EXPIRE") {
                        if (command.size() == 3) {
                            int64_t seconds = std::stoll(command[2]);
                            storage.expire(command[1], seconds);
                        }
                    } else if (cmd == "INCR") {
                        // INCR during replay - just get current and increment
                        auto val = storage.get(command[1]);
                        if (val.has_value()) {
                            try {
                                int64_t num = std::stoll(val.value()) + 1;
                                storage.set(command[1], std::to_string(num));
                            } catch (...) {
                                // Non-integer, skip
                            }
                        } else {
                            storage.set(command[1], "1");
                        }
                    }
                    
                    commandCount++;
                }
            }
        } catch (...) {
            // Parsing error, skip to next
            break;
        }
    }
    
    std::cout << "AOF loaded: " << commandCount << " commands replayed" << std::endl;
}

// Manual fsync
void AOF::sync() {
    if (aofFile) {
        fsync(fileno(aofFile));
    }
}

// Background rewrite using fork (Linux only)
bool AOF::bgRewriteAOF(Storage& storage) {
    if (rewriteInProgress) {
        return false;  // Already running
    }
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // === CHILD PROCESS ===
        
        // Write to temp file
        FILE* tempFile = fopen("temp-rewrite.aof", "w");
        if (!tempFile) {
            std::cerr << "Failed to create temp AOF file" << std::endl;
            exit(1);
        }
        
        // Dump current state (iterate storage)
        auto allData = storage.getAll();
        
        for (const auto& [key, value] : allData) {
            // Build SET command
            std::vector<std::string> cmd = {"SET", key, value.value};
            
            // Add expiration if exists
            if (value.expiresAt != -1) {
                int64_t ttl = value.expiresAt - Storage::getCurrentTimeMs();
                if (ttl > 0) {
                    cmd.push_back("PX");
                    cmd.push_back(std::to_string(ttl));
                }
            }
            
            std::string respCmd = RESPEncoder::encodeArray(cmd);
            fwrite(respCmd.c_str(), 1, respCmd.size(), tempFile);
        }
        
        // Sync to disk
        fsync(fileno(tempFile));
        fclose(tempFile);
        
        // Atomic rename
        if (rename("temp-rewrite.aof", "appendonly.aof") != 0) {
            std::cerr << "Failed to rename AOF file" << std::endl;
            exit(1);
        }
        
        exit(0);  // Child exits
    } 
    else if (pid > 0) {
        // === PARENT PROCESS ===
        rewriteChildPid = pid;
        rewriteInProgress = true;
        std::cout << "Background AOF rewrite started (pid: " << pid << ")" << std::endl;
        return true;
    } 
    else {
        // Fork failed
        std::cerr << "Fork failed for AOF rewrite" << std::endl;
        return false;
    }
}

bool AOF::isRewriteInProgress() {
    if (!rewriteInProgress) return false;
    
    // Check if child finished (non-blocking)
    int status;
    pid_t result = waitpid(rewriteChildPid, &status, WNOHANG);
    
    if (result == 0) {
        return true;  // Still running
    } else {
        // Child finished
        rewriteInProgress = false;
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            std::cout << "Background AOF rewrite completed successfully" << std::endl;
            
            // Reopen AOF file to point to new file
            if (aofFile) {
                fclose(aofFile);
                aofFile = fopen(filename.c_str(), "a");
            }
        } else {
            std::cerr << "Background AOF rewrite failed" << std::endl;
        }
        
        return false;
    }
}