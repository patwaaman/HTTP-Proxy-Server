# HTTP-Proxy-Server

================================================================

           Name: Aman Raj Patwa
 Username/Login: IIT2018038
 
================================================================

The aim of the assignment was to implement a concurrent http proxy server, which receives and parses http GET requests from clients, forwards them to the remote servers, and returns the data from the remote server back to the clients.

I have used multiple functions as per the need. All these functions except main() are only called from the child process, which is forked at the time a socket connection is made between proxyserver and client. The parent process makes sure to reap children whose process has terminated.

main(): 

This function contains the serverfd and clientfd and the portno as argument in the main function. Makes call to createServerSocket() function to bind socket to the port no. All waiting request is then processed in fifo order and alloted a new clifd to the client process. Further there are process creation steps through fork(). In child process, the original socket must be closed and all the request is then handled by handleRequest() funtion. 

createServerSocket():

This funtion creates the socket and initialise the server address, then socket binding is done wherein allocates a portno to the socket .listen() have wait queue to max of 5 client. At last this function returns the sockfd to the main(). 

readFromClient():

This function reads the request from the client using the clifd. Here, 2 buffers namely buffer and buf_temp stores the incoming request in itself. finally it returns the length of the request made by the client (req_len) to the handleRequest().

handleRequest():

This function has the aim to handle request made by the client.The incoming request from readFromClient() is parsed here with error checking . finally our most important server_proxy_act() is called here.

server_proxy_act():

This function contains 2 parameter req and clifd. Here,
clifd:- original client;
sockfd:- original server;
serv_addr_actual:- actual address of the original server.
Here, it makes connection to the actual server i.e. original server, there's it read the data from there and writes back to the original client. after that closes all sockets.

error():
return the error message and finally exits.

-------------------------------------------------------------------------------------------------------------

GIVEN TESTING FILE:

* proxy tester.py
For the same http GET request, this script compares the output of our proxy server with that of a direct
telnet session.

* proxy tester conc.py
For the same http GET request, this script runs multiple tests, like fetches(both single and concurrent),
split fetches(both single and concurrent).

-------------------------------------------------------------------------------------------------------------------

The proxy server accepts the incoming connection from the client, forwards the request to the remote server
and returns the result of the remote server back to the client. The connection with the client is established
using a streaming TCP socket.
The test-script compares the output of the proxy to a direct connection, and checks equality. The telnet
protocol is used to connect to the remote and proxy server.

Since the proxy server forks a new process for every new incoming connection, it supports concurrent requests
from multiple clients. Gives details about concurrent and split requests to the proxy server.
A split fetch is one where the http request is transmitted in 2 different messages to the proxy server. The server
has to wait for the client to send a message which terminates with a CRLF(Carriage Return Line Feed), to
then preprocess and send to the remote server.
