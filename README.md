# Socket Chat Room

>The program is a CLI-styled chat room. It's a demo of using C socket to do network programming.

## Tech
Using select() to do I/O multiplexing, that is, to handle multiple file descriptor I/O simultaneously.

## Specification
### Server
- send hello message to all the clients
- send offline message to all the clients

### Client
- Command available:
    - `tell <USERNAME> <MESSAGE>`
    - `yell <MESSAGE>`
    - `name <NEW USERNAME>`
    - `who`

## Usage
```
make
./server     #open server
./client 
./client     #can serves multiple clients(up to 15 by default) by open multiple clients simultaneously 
```
