# socket-chat

Socket chat client and server project made in C with socket programming.

Working on **Linux** AND **Windows** using system socket APIs, along
with C11 threads for user input. 

Start the executable
```bash
./socketchat
```

It is completely **asynchronous**, implemented with an event loop that 
checks for file descriptor events using `poll`. 
Partial reads and writes are handled by a structure of incoming and outgoing 
buffers.

Console input is managed by a **separate (blocking) thread for input** without
distrubing the event loop. 
Stdin event updates are handled on the client by a UDP loopback connection 
between the main loop thread and the stdin thread. 

It currently features the ability to send messages from `stdin` to all clients 
connected to the server.
