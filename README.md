# Multi-Client Chat Server

This is a simple multi-client chat server implemented in C, using POSIX threads for concurrency and socket programming to handle incoming connections. The server allows multiple clients to connect to a single port, and clients can send messages to each other through the server. 

## Usage

To use this server, simply compile the code using a C compiler such as gcc:

```
gcc -pthread chat_server.c -o chat_server
```

Then, run the resulting executable:

```
./chat_server
```

The server will start listening on port 8888. Clients can connect to this port using a TCP client such as telnet. For example:

```
telnet localhost 8888
```

Once connected, the client will be prompted to enter a username. After entering a username, the client can send messages to other connected clients. Messages will be broadcast to all clients except for the sender.

## Contributing

This project is open to contributions! If you find a bug, have an idea for a feature, or want to improve the code, feel free to submit a pull request or open an issue. 

## License

This project is licensed under the MIT License. See LICENSE.txt for details.
