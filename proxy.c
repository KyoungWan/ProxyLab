#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
void handle_request(int);

int main(int argc, char **argv)
{
  char *port;
  int listenfd, connfd, clientlen;
  struct sockaddr_in clientaddr;
    printf("%s", user_agent_hdr);

  //show usage error message
  if (argc != 2)
  {
    printf("Usage: ./proxy <port_number>\n");
  }
  port  = argv[1];
  printf("port :%s\n", port);
  listenfd = Open_listenfd(port); //open socket && port and listen

  while(1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //typedef struct sockaddr SA;
    handle_request(connfd);
    Close(connfd);
  }
  return 0;
}

void handle_request(int fd)
{
  printf("fd :%d \n", fd);
  return;
}
