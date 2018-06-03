#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct
{
  char method[MAXLINE];
  char uri[MAXLINE];
  char version[MAXLINE];
} request_line;
typedef struct
{
  char name[MAXLINE];
  char data[MAXLINE];
  struct request_header* next;
} request_header;

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static request_header *root= NULL;
void handle_request(int);
void parse_line(request_line *, char*);
void parse_header(request_header *, char*);
request_header* last_node();
void print_headers();

int main(int argc, char **argv)

{
  char *port;
  int listenfd, connfd, clientlen;
  struct sockaddr_in clientaddr;
  struct hostent *hp;
  char *haddrp;
    printf("%s", user_agent_hdr);

  //show usage error message
  if (argc != 2)
  {
    printf("Usage: ./proxy <port_number>\n");
  }
  port  = argv[1];
  printf("port :%s\n", port);
  listenfd = Open_listenfd(port); //Open_listenfd => socket(), bind(), listen()

  while(1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //typedef struct sockaddr SA;

    /* Determine the domain name and IP adress of the client */
    hp = Gethostbyaddr((const char*)&clientaddr.sin_addr.s_addr, clientlen, AF_INET);
    haddrp = inet_ntoa(clientaddr.sin_addr);
    printf("server connected to %s (%s) \n", hp->h_name, haddrp);
    handle_request(connfd);
    Close(connfd);
  }
  return 0;
}

void handle_request(int fd)
{
  request_line* line=malloc(sizeof(line));
  //open rio of client input
  rio_t rio;
  char buf[MAXLINE];
  size_t n;
  //void Rio_readinitb(rio_t *rp, int fd)
  Rio_readinitb(&rio, fd);
  //read line
  n = Rio_readlineb(&rio, buf, MAXLINE);
  printf("server received %d bytes\n", n);
  parse_line(line, buf);
  memset(&buf[0], 0, sizeof(buf)); //flushing buffer

  //read all header
  while(strcmp(buf, "\r\n"))
  {
    Rio_readlineb(&rio, buf, MAXLINE);
    request_header* header=malloc(sizeof(header));
    parse_header(header, buf);
    print_headers();
    memset(&buf[0], 0, sizeof(buf)); //flushing buffer
  }
  return;
}

void parse_line(request_line* line, char* buf)
{
  sscanf(buf, "%s %s %s\n", line->method, line->uri, line->version);

  printf("enter parse_line\n");
  printf("method: %s uri: %s version: %s\n", line->method, line->uri, line->version);
  //buf: GET http://www.example.com HTTP/1.0
}

void parse_header(request_header* header, char *buf)
{
  printf("enter parse_header\n");
    char* name= strstr(buf, ": ");
    char* data= strstr(buf, "\r\n");
    request_header* last=NULL;
    //TODO: handling error during memcpy, ex. HOST:::::123
    memcpy(header->name, buf, name-buf);
    memcpy(header->data, name+2, data-name-2);
    header->next= NULL;
    if((!name)||(!data))
    {
      //bad header format
      printf("bad header format error\n");
    }
    if(!root) {
      root = header;
    } else {
      last = last_node();
      last->next = header;
    }
  printf("name: %s data: %s\n", header->name, header->data);
}

request_header* last_node() {
  request_header* temp = malloc(sizeof(request_header*));
  temp = root;
  while(temp && temp->next){
    temp = temp ->next;
  }
  return temp;
}

void print_headers() {
  printf("=====print_headers start====\n");
  request_header* temp=root;
  printf("print_root %p\n", root);
  int i=0;
  while(temp){
    printf("header[%d] name: %s data: %s -> \n", i++, temp->name, temp->data);
    temp = temp ->next;
  }
  printf("=====print_headers end======\n");
  printf("\n");
}
