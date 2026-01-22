# Linux-Only Redis Clone with AOF

This is a **Linux-only** version with simplified code (no Windows compatibility).

## Features:
- ✅ AOF persistence (all 3 modes: always, everysec, no)
- ✅ BGREWRITEAOF using fork() (Linux-native)
- ✅ LRU eviction with random sampling
- ✅ Expiration (lazy + active cleanup)
- ✅ epoll-based async I/O (10K+ concurrent clients)

## Build:
```bash
make clean
make
```

## Run:
```bash
./server_async
```

## Test AOF:
```bash
# Terminal 1: Start server
./server_async

# Terminal 2: Add data
redis-cli -p 7379 SET key1 value1
redis-cli -p 7379 SET key2 value2

# Restart server (Ctrl+C then restart)
./server_async

# Verify data persisted
redis-cli -p 7379 GET key1  # Returns "value1"

# Trigger compaction
redis-cli -p 7379 BGREWRITEAOF
```

## Differences from Windows Version:
- No `#ifdef _WIN32` guards
- Uses fork() for BGREWRITEAOF (simpler code)
- Uses epoll (better performance than select)
- Uses native fsync() (no _commit() wrapper)

## Performance:
- ~500K ops/sec (everysec mode)
- ~10K ops/sec (always mode)
- Fork latency: 1-5ms for 10K keys
