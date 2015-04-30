/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */


void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void dostuff(int); /* function prototype */
void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, pid;
     int portno = 50000;
     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;
     struct sigaction sa;          // for signal SIGCHLD

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     // portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     
     listen(sockfd,5);
     
     clilen = sizeof(cli_addr);
     
     /****** Kill Zombie Processes ******/
     sa.sa_handler = sigchld_handler; // reap all dead processes
     sigemptyset(&sa.sa_mask);
     sa.sa_flags = SA_RESTART;
     if (sigaction(SIGCHLD, &sa, NULL) == -1) {
         perror("sigaction");
         exit(1);
     }
     /*********************************/
     
     while (1) {
         newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
         
         if (newsockfd < 0) 
             error("ERROR on accept");
         
         pid = fork(); //create a new process
         if (pid < 0)
             error("ERROR on fork");
         
         if (pid == 0)  { // fork() returns a value of 0 to the child process
             close(sockfd);
             dostuff(newsockfd);
             exit(0);
         }
         else //returns the process ID of the child process to the parent
             close(newsockfd); // parent doesn't need this 
     } /* end of while */
     return 0; /* we never get here */
}

/******** DOSTUFF() *********************
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
void dostuff (int sock)
{
  int n;
  char buffer[256];
  char name[256];
  char file_name[256];
  char *token;
  char *str = "successfully connect  but fail to find/open the request file";
  char *reply = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
  char extra[1000];
     
  //read
  bzero(buffer,256);
  n = read(sock,buffer,255);
  if (n < 0) error("ERROR reading from socket");

  //make sure client delivers whole message before writing
  if (n == 255){
    while(read(sock,extra,1000) == 1000){
      continue;
    }
  }

  //output buffer
  printf("Here is the message:\n%s\n",buffer);
  strcpy(name, buffer);

  //parse for filename
  token = strtok(name, " /"); /* get the first token */
  token = strtok(NULL, " /"); 
  strcpy(file_name, token);

  //open and read from file, then write
  FILE *fp = fopen(file_name,"r");

  if(fp==NULL){
    write(sock,str,strlen(str)); 
  } 

  else{
    char buff[265];
    write(sock, reply, strlen(reply));

    while(fread(buff,1,265,fp)){
      write(sock,buff,sizeof(buff));  //use sizeof(buff) rather than strlen(buff)
      }
    fclose(fp);
  }
}
