# Socket Chat

Socket chat client and server project made in C with socket programming.

Working on **Linux** with **UNIX sockets**. It is completely **asynchronous**, 
implemented with an event loop that checks for file descriptor events using
`poll`. Partial reads and writes are handled by a structure of incoming 
and outgoing buffers.

It currently features the ability to send messages from `stdin` to all clients 
connected to the server.

Start the executable
```bash
./SocketChat
```
