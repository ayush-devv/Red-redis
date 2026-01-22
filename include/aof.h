#ifndef AOF_H
#define AOF_H

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <unistd.h>
#include <sys/wait.h>
#include "storage.h"

class AOF {
private:
    std::string filename;
    FILE* aofFile;
    bool enabled;
    std::string syncMode;  // "always", "everysec", "no"
    
    // For everysec mode - background fsync thread
    std::thread fsyncThread;
    std::atomic<bool> running{false};
    void fsyncWorker();  // Background thread function
    
    // For BGREWRITEAOF - fork-based rewrite
    pid_t rewriteChildPid;
    std::atomic<bool> rewriteInProgress{false};

public:
    AOF(const std::string& filepath = "appendonly.aof", 
        const std::string& sync = "everysec");
    ~AOF();
    
    // Main operations
    void log(const std::vector<std::string>& command);
    void replay(Storage& storage);
    
    // Rewrite (compaction)
    bool bgRewriteAOF(Storage& storage);
    bool isRewriteInProgress();
    
    // Utility
    bool isEnabled() const { return enabled; }
    void sync();  // Manual fsync
};

#endif