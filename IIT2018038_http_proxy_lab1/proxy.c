#include "proxy_parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAXN 65536
int n=0;               // return values from write() or read()      
char buffer[MAXN];    //request buffer
char buf_temp[MAXN];   //temporary buffer

////function to handle error.
void error(char *msg)
{
    perror(msg);
    exit(1);
}

////function to Send GET request to actual server : proxy acts as a client Now.
int server_proxy_act(struct ParsedRequest *req, int clifd ){

  int sockfd, portno;                   //originalserverfd
  struct sockaddr_in serv_addr_actual;  //address of actual server.
  struct hostent *server_actual;        //host of server.

  portno = (req->port == NULL) ? 80 : strtol(req->port,(char **)NULL,10);

  sockfd = socket(AF_INET,SOCK_STREAM,0);
  if(sockfd < 0)    error("Couldn't open socket in child proc");

  //gethostbyname() queries databases around the country.
  server_actual = gethostbyname(req->host);

  // ERROR 404!! if lookup to the host fails, it returns a null pointer. 
  if(server_actual==NULL){      
    memset(&buffer,0,MAXN);
    sprintf(buffer,"HTTP/1.0 404 Not Found\r\n\r\n");
    n = write(clifd,buffer,strlen(buffer));
    if(n<0)   error("Couldn't write to socket");
  }

  //set fields of address.
  memset(&serv_addr_actual, 0,sizeof(serv_addr_actual));
  serv_addr_actual.sin_family = AF_INET;
  memcpy(&serv_addr_actual.sin_addr.s_addr,server_actual->h_addr,server_actual->h_length);
  serv_addr_actual.sin_port = htons(portno);

  // Establish connection to the Original Server.
  if(connect(sockfd,(struct sockaddr *)&serv_addr_actual,sizeof(serv_addr_actual)) < 0)
    error("Couldn't establish connection to server");
  
  //Modify request to Server
  n = ParsedHeader_set(req,"Host",req->host);
  if(n < 0)   error("parse set host failed");
  
  n = ParsedHeader_set(req,"Connection","close");
  if(n < 0)   error("parse_set connection failed");

  // Length of modified Request.
  int mod_len = ParsedRequest_totalLen(req);
  char* mod_buf = (char*)malloc(mod_len+1);
  n=ParsedRequest_unparse(req,mod_buf,mod_len);
  if(n < 0)    error("unparse failed");

  mod_buf[mod_len] = '\0';

  //Write to original Server.
  n = write(sockfd,mod_buf,mod_len);
  if (n < 0)    error("couldn't write to original server");

  // Simultaneous read and write : Read result from original server. 
	while(1){
	memset(&buffer,0,MAXN);
  	n = read(sockfd,buffer,MAXN);
    if(n==0)
		  break;

  	if(n<0){
      error("couldn't read from original server");
			break;
  	}

    n = write(clifd,buffer,strlen(buffer));
    if(n<0)   error("couldn't write to client");
	}
  //close original server side socket
  close(sockfd);
  //close client side socket.
  close(clifd);
  return 0; 
}

//// function to bind socket to the portno.
int createServerSocket(int portno ){

  // initialise server socket. 
  struct sockaddr_in serv_addr; 
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)   error("error opening socket");

  memset(&serv_addr,0,sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  // Binding socket ~ allocating a portno to the socket.
  if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
    error("error on binding");

  //listen with wait queue of 5 client max.
  listen(sockfd,5);
  return sockfd;
}

//// function to read incoming message/request from the client.
int readFromClient(int clifd) {

  int req_len=0;
  memset(&buffer,0,MAXN);
  memset(&buf_temp,0,MAXN);

  n = read(clifd,buf_temp,MAXN);
  if(n<0)   error("Couldn't read from socket");
  
  req_len = strlen(buf_temp);	
  sprintf(buffer+strlen(buffer),buf_temp);

  if(req_len > 0 && strcmp(&buf_temp[req_len-4],"\r\n\r\n")!=0){
    memset(&buf_temp,0,MAXN);
    n = read(clifd,buf_temp,MAXN);
	  if(n<0)   error("Couldn't read from socket");

	  req_len += strlen(buf_temp);
	  sprintf(buffer+strlen(buffer),buf_temp);
  }
  return req_len;
}

////function to Handle request from the newly clientfd. 
void handleRequest(int clifd)
{ 
    int req_len =  readFromClient(clifd);

    //Parse Request from the incoming clientfd.
    struct ParsedRequest*  req = ParsedRequest_create();

    if (ParsedRequest_parse(req, buffer, req_len)< 0) {
      memset(&buffer,0,MAXN);
      sprintf(buffer,"HTTP/1.0 400 Bad Request\r\n\r\n");
      n = write(clifd,buffer,strlen(buffer));
      if(n<0)   error("Couldn't write to socket");
    }

    if(server_proxy_act(req, clifd) == 0)
      exit(0);
}

int main(int argc, char * argv[]) {

  int sockfd, clifd;      //serverfd and clientfd
  int portno, clilen; 
  int pid;                 // child process id
  struct sockaddr_in cli_addr;  // client address

  if (argc < 2)   error("error, no port provided\n");
  
  portno = atoi(argv[1]);         
  sockfd=createServerSocket(portno); 
  clilen = sizeof(cli_addr);
  
  while(1){
    
    //accepts waiting request and creates new socket and allocates a new fd for the socket.
    clifd = accept(sockfd,(struct sockaddr* ) &cli_addr, (socklen_t*)&clilen);
    if(clifd < 0)   error("Couldn't accept incoming connection");
    
    pid = fork();    
    if(pid < 0)   error("error on forking new process");
    
    if(pid==0){
      //Enter in child process : close the original socket connection.
      close(sockfd);  
      handleRequest(clifd);
    }
    else 
      close(clifd);  //in parent process close the newly clientfd.
  }
  return 0;
}
