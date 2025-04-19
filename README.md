# Redis Clone 
This is a replication of Redis built using C++. This project was made by following the book "Build Your Own Redis" by James Smith.
This project was made to understand the internals of Redis and how it works. Think of it like a case study rather than a full on project.

Note: This project uses syscalls on Linux and will not work on other operating systems. 

## Features

- A server that stores the database in memory
- A client that can interact with the server
- The database stores key-value pairs 
- The values can either be strings or a sorted set

## What I learned
- The design choices that Redis made
- How to build a tcp server client model in C++ 
- The usage of Event Loops to handle multiple clients 
- How to avoid latency spikes by spreading work over multiple iterations of the event loop
- Implementing multiple datastructures like hashmap (chaining), AVL tree etc 
- Multi-threading for handling long running tasks like deleting a large sorted set

# User Guide:

## Building the project:
You can build the binaries using CMake. 
```bash
mkdir build 
cd build
cmake ..
make
```
This will place the server and client binaries in the `bin` directory.

## How to use:

Start by running the server binary.
Next, run the client binary. The client will establish a connection to the server and you will be able to run commands.

## Commands:

### Key Value Commands:

- `get <key>`: Get the value of a key.
- `set <key> <value>`: Set the value of a key.
- `del <key>`: Delete a key.
- `pexpire <key> <milliseconds>`: Set a key to expire after a certain amount of time.
- `pttl <key>`: Get the time to live of a key in milliseconds.
- `keys`: Get all the keys in the database.

### Sorted Set Commands:

- `zadd <key> <score> <value>`: Add a value to a sorted set with a score (set is sorted by score).
- `zrem <key> <value>`: Remove a value from a sorted set.
- `zscore <key> <value>`: Get the score of a value in a sorted set.
- `zquery <key> <score> <value> <offset> <limit>`: Get limit values in a sorted set starting from given value + offset.




