# Redis Clone 
This is a replication of Redis built using C++. This project was made by following the book "Build Your Own Redis" by James Smith.
This project was made to understand the internals of Redis and how it works. Think of it like a case study rather than a full on project.

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

