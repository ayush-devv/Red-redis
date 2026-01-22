# Build Commands for Redis Clone

## Using mingw32-make (Recommended)

You have `mingw32-make` installed. Use these commands:

```powershell
# Build server (default)
mingw32-make

# Build and run tests
mingw32-make test

# Clean build artifacts
mingw32-make clean

# Rebuild from scratch
mingw32-make rebuild

# Build and run server
mingw32-make run

# Show help
mingw32-make help
```

## Optional: Create 'make' Alias

To use `make` instead of `mingw32-make`:

```powershell
# Add to PowerShell profile (run once)
echo 'Set-Alias -Name make -Value mingw32-make' | Out-File -Append $PROFILE

# Reload profile
. $PROFILE

# Now you can use:
make
make test
make clean
```

## Quick Start

```powershell
# 1. Build
mingw32-make

# 2. Run tests
mingw32-make test

# 3. Start server
mingw32-make run

# 4. In another terminal, connect with redis-cli
cd ..\redis
.\redis-cli.exe -p 7379
```

## Test DEL and EXPIRE Commands

```bash
# In redis-cli
SET key1 "value1"
SET key2 "value2"
SET key3 "value3"

# Test DEL
DEL key1 key2        # Returns: (integer) 2
GET key1             # Returns: (nil)

# Test EXPIRE
SET mykey "data"
EXPIRE mykey 10      # Returns: (integer) 1
TTL mykey            # Returns: 9, 8, 7...
```
