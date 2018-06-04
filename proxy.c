#include <string.h>
#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct
{
  char method[10];
  char uri[200];
  char hostname[200];
  char path[1000];
  char version[200];
} request_line;
typedef struct
{
  char name[MAXLINE];
  char data[MAXLINE];
  struct request_header* next;
} request_header;

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3";
static request_header *root= NULL;
void handle_request(int);
void parse_line(request_line *, char*);
void parse_header(request_header *, char*, request_line*);//todo remove request_line*
request_header* last_header();
void print_headers();
void insert_header(request_header*);
request_header* find_header_by_key(char*);
void print_line(request_line*);

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

  Rio_readinitb(&rio, fd);
  //read line
  n = Rio_readlineb(&rio, buf, MAXLINE); //printf("server received %d bytes\n", n);
  parse_line(line, buf);
  print_line(line);
  memset(&buf[0], 0, sizeof(buf)); //flushing buffer

  //read all header
  Rio_readlineb(&rio, buf, MAXLINE);
  print_line(line);
  while(strcmp(buf, "\r\n"))
  {
    printf("while buf : %s\n", buf);
    parse_header(NULL, buf, line);
    print_line(line);
    memset(&buf[0], 0, sizeof(buf)); //flushing buffer
    Rio_readlineb(&rio, buf, MAXLINE);
  }
    memset(&buf[0], 0, sizeof(buf)); //flushing buffer
  print_line(line);
  make_header(line);
  printf("/////////////final state/////////////\n");
  print_line(line);
  print_headers();
  free_line_header(line, root);
  return;
}
void make_header(request_line* line)
{
  request_header* temp=NULL;

  temp = find_header_by_key("Host");
  if(!temp){
    request_header* new_header=malloc(sizeof(request_header*));
    strcpy(new_header->name, "Host");
    strcpy(new_header->data, line->hostname);//TODO: URI로부터 host추출하기
    insert_header(new_header);
    temp=NULL;
  }

  temp = find_header_by_key("User-Agent");
  if(!temp){
    request_header* new_header=malloc(sizeof(request_header*));
    strcpy(new_header->name, "User-Agent");
    strcpy(new_header->data, user_agent_hdr);
    insert_header(new_header);
    temp=NULL;
  }

  temp = find_header_by_key("Connection");
  if(!temp){
    request_header* new_header=malloc(sizeof(request_header*));
    strcpy(new_header->name, "Connection");
    strcpy(new_header->data, "close");
    insert_header(new_header);
    temp=NULL;
  }

  temp = find_header_by_key("Proxy-Connection");
  if(!temp){
    request_header* new_header=malloc(sizeof(request_header*));
    strcpy(new_header->name, "Proxy-Connection");
    strcpy(new_header->data, "close");
    insert_header(new_header);
    temp=NULL;
  }

  //1.headers를 돌면서 HOSTㄹ인 헤더ㄹ를 찾아라 찾아서 Host에 넣어주기
  //2.없으면 URi에서 뜯어라
  //3.single line으로 USer-Agent header 보내주기
  //4.Connection: close
  //5.Proxy-Connection: close 헤더를 포함해라.
}

request_header* find_header_by_key(char* type)
{
  // Host, User-Agent, Connection, Proxy-Connection
  request_header* temp=malloc(sizeof(request_header*));
  temp = root;
  while(temp){
    if(strcmp(temp->name, type)==0) {
      return temp;
    }
    else {
      temp = temp->next;
    }
  }
  free(temp);
  return NULL;
}

void insert_header(request_header *header) {
  request_header* last=NULL;
  if(!root) {
    root = header;
    header->next = NULL;
  }else {
    last = last_header();
    last->next = header;
    header->next = NULL;
  }
}


void parse_line(request_line* line, char* buf)
{
  printf("enter into parse line\n");
  char method[10];
  char uri[300];
  char version[100];
  char *host_start;
  char *path_start;
  //TODO: fix for error handling
  /*
  sscanf(buf, "%s %s %s\n", line->method, line->uri, line->version);
  strcpy(method, line->method);
  strcpy(uri, line->uri);
  strcpy(version, line->version);
  */
  sscanf(buf, "%s %s %s\n", method, uri, version);
  strcpy(line->method, method);
  strcpy(line->uri, uri);
  strcpy(line->version, version);

  // URL to hostname && path
  host_start = strstr(uri, "http://");
  if(!host_start) {
    //no hostname
    return;
  }
  host_start += 7;
  path_start = strstr(host_start, "/");
  if(!path_start) {
    //no path -> add default path '/'
    strcpy(line->hostname, path_start);
    strcpy(line->path, "/");
  } else {
    //URL has hostname && path
    memcpy(line->hostname, host_start, path_start-host_start); //strcpy_s is only for windows
    strcpy(line->path, path_start);
  }

  if(strcmp("GET", method) !=0){
    printf("Only GET method can be accepted\n");
    return ; //TODO: error handling 
  }

  printf("method: %s uri: %s version: %s\n", line->method, line->uri, line->version);
  //buf: GET http://www.example.com HTTP/1.0
}

void parse_header(request_header* nouse, char *buf, request_line* line)
{
  request_header* header = malloc(sizeof(request_header));
  char* pname= strstr(buf, ": ");
  char* pdata= strstr(buf, "\r\n");
  request_header* last=NULL;
  //TODO: handling error during memcpy, ex. HOST:::::123
    memcpy(header->name, buf, pname-buf);
    print_line(line);
  //아래줄을 실행하면 line의 uri가 깨지는 버그가 있다..
   memcpy(header->data, pname+2, pdata-pname-2);
    print_line(line);
    if((!pname)||(!pdata))
    {
      //bad header format
      printf("bad header format error\n");
    }
    insert_header(header);
  printf("name: %s data: %s\n", header->name, header->data);
}

request_header* last_header() {
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
void print_line(request_line* line){
  printf("=====print_line start====\n");
  printf("method: %s\n", line->method);
  printf("uri: %s\n", line->uri);
  printf("hostname: %s\n", line->hostname);
  printf("path: %s\n",line->path);
  printf("version: %s\n", line->version);
  printf("=====print_line end======\n");
}
void free_line_header(request_line* line, request_header* root) {
  //free header
  request_header* current = root;
  request_header* temp = root;
  if(current){
    while(current->next){
      temp = current->next;
      free(current);
      current = temp;
    }
    free(current);
  }
  //free line
  free(line);
}
