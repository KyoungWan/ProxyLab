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
typedef struct request_header
{
  char name[MAXLINE];
  char data[MAXLINE];
  struct request_header* next;
} request_header;

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3";
static request_header *root= NULL;
void handle_request(int);
void parse_line(request_line *, char*);
void parse_header(char*, request_line*);//todo remove request_line*
request_header* last_header();
void print_headers();
void insert_header(request_header*);
int find_header_by_key(char*);
void print_line(request_line*);
void send_request(int, request_line*);
void make_header(request_line*);
void free_line_header(request_line*, request_header*);
char* xstrncpy(char *, const char*, size_t);
request_header* find_in_header(char*);
void print_request(char []);

int main(int argc, char **argv)

{
  char *port;
  int listenfd, connfd;
  socklen_t clientlen;
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
   // hp = Gethostbyaddr((const char*)&clientaddr.sin_addr.s_addr, clientlen, AF_INET);
   // haddrp = inet_ntoa(clientaddr.sin_addr);
   // printf("server connected to %s (%s) \n", hp->h_name, haddrp);
    handle_request(connfd);
    //Close(connfd);
    printf("server closed\n");
  }
  return 0;
}

void handle_request(int fd)
{
  request_line* line=malloc(sizeof(request_line));
  //open rio of client input
  rio_t rio;
  char buf[MAXLINE];

  Rio_readinitb(&rio, fd);
  //read line
  Rio_readlineb(&rio, buf, MAXLINE); //printf("server received %d bytes\n", n);
  parse_line(line, buf);
  print_line(line);
  memset(&buf[0], 0, sizeof(buf)); //flushing buffer

  //read all header
  Rio_readlineb(&rio, buf, MAXLINE);
  while(strcmp(buf, "\r\n"))
  {
    printf("while buf : %s\n", buf);
    parse_header(buf, line);
    print_line(line);
    memset(&buf[0], 0, sizeof(buf)); //flushing buffer
    Rio_readlineb(&rio, buf, MAXLINE);
  }
  memset(&buf[0], 0, sizeof(buf)); //flushing buffer
  make_header(line);
  printf("/////////////final state/////////////\n");
  print_line(line);
  print_headers();
  send_request(fd, line);
  free_line_header(line, root);
  return;
}
void send_request(int fd, request_line* line)
{
  print_line(line);
  char* default_port="80";
  char* pport = NULL;

  char request_port[200];
  char request_domain[200];
  int requestfd;
  char request_buf[10000];
  request_header* header;

  //find domain
  if(strlen(line->hostname)) {
    strcpy(request_domain, line->hostname);
  }
  else if((header = find_in_header("Host")) != NULL) {
    strcpy(request_domain, header->data);
  }
  else {
    //host가 주어지지 않은경우
    printf("error occur: host not found\n");
    return;
  }
  pport = strstr(request_domain, ":");
  if(pport){
    *pport = '\0'; // remove port(e.g :8080) in domain
    pport = (pport+1);
    strcpy(request_port, pport);
  }else {
    strcpy(request_port, default_port);
  }
  //open request file descriptor
  printf("request_domain: %s, request_port %s\n", request_domain, request_port);
  requestfd= Open_clientfd(request_domain, request_port);
  printf("test\n");

  //make request
  strcat(request_buf, line->method);
  strcat(request_buf, " ");
  strcat(request_buf, line->path);
  strcat(request_buf, " ");
  strcat(request_buf, "HTTP/1.0\r\n");
  header = root;
  while(header)
  {
    strcat(request_buf, header->name);
    strcat(request_buf, ": ");
    strcat(request_buf, header->data);
    strcat(request_buf, "\r\n");
    header = header->next;
  }
  strcat(request_buf, "\r\n");
  //send request
  print_request(request_buf);
  Rio_writen(requestfd, request_buf, strlen(request_buf));
  //recieve response
  rio_t rio;
  char read_buf[MAXLINE];
  ssize_t n=0;
  Rio_readinitb(&rio, requestfd);
  while((n = Rio_readnb(&rio, read_buf, MAXLINE))>0) //cannot use lineb cause interface of Rio_writen
  {
    //write to client of proxy
    Rio_writen(fd, read_buf, (size_t)n);
  }
  printf("Rio_writen end\n");
  Close(requestfd);
  Close(fd);

}
void print_request(char requestbuf[]){
  printf("=====print_request=====\n");
    puts(requestbuf);
}
void make_header(request_line* line)
{
  int find=0;

  find = find_header_by_key("Host");
  if(!find){
    request_header* new_header=malloc(sizeof(request_header));
    strcpy(new_header->name, "Host");
    strcpy(new_header->data, line->hostname);
    insert_header(new_header);
  }

  find= find_header_by_key("User-Agent");
  if(!find){
    request_header* new_header=malloc(sizeof(request_header));
    strcpy(new_header->name, "User-Agent");
    strcpy(new_header->data, user_agent_hdr);
    insert_header(new_header);
  }

  find= find_header_by_key("Connection");
  if(!find){
    request_header* new_header=malloc(sizeof(request_header));
    strcpy(new_header->name, "Connection");
    strcpy(new_header->data, "close");
    insert_header(new_header);
  }

  find= find_header_by_key("Proxy-Connection");
  if(!find){
    request_header* new_header=malloc(sizeof(request_header));
    strcpy(new_header->name, "Proxy-Connection");
    strcpy(new_header->data, "close");
    insert_header(new_header);
  }

  //1.headers를 돌면서 HOSTㄹ인 헤더ㄹ를 찾아라 찾아서 Host에 넣어주기
  //2.없으면 URi에서 뜯어라
  //3.single line으로 USer-Agent header 보내주기
  //4.Connection: close
  //5.Proxy-Connection: close 헤더를 포함해라.
}

int find_header_by_key(char* type)
{
  // Host, User-Agent, Connection, Proxy-Connection
  request_header* temp;
  temp = root;
  while(temp){
    if(strcmp(temp->name, type)==0) {
      return 1;
    }
    else {
      temp = temp->next;
    }
  }
  return 0;
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
request_header* find_in_header(char* name)
{
  request_header* temp;
  temp = root;
  while(temp) {
    if(strcmp(temp->name, name)==0) return temp;
    else temp = temp->next;
  }
  return NULL;
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
    strcpy(line->hostname, host_start);
    strcpy(line->path, "/");
  } else {
    //URL has hostname && path
    //memcpy(line->hostname, host_start, path_start-host_start); //strcpy_s is only for windows
    //strncpy(line->hostname, host_start, path_start-host_start); //strcpy_s is only for windows
    xstrncpy(line->hostname, host_start, path_start-host_start); //strcpy_s is only for windows
    printf("hostname test: %s\n", line->hostname);
    strcpy(line->path, path_start);
  }

  if(strcmp("GET", method) !=0){
    printf("Only GET method can be accepted\n");
    return ; //TODO: error handling 
  }

  printf("method: %s uri: %s version: %s\n", line->method, line->uri, line->version);
  printf("hostname: %s path: %s\n", line->hostname, line->path);
  //buf: GET http://www.example.com HTTP/1.0
}

void parse_header(char *buf, request_line* line)
{
  request_header* header = malloc(sizeof(request_header));
  char* pname= strstr(buf, ": ");
  char* pdata= strstr(buf, "\r\n");
  if((!pname)||(!pdata))
  {
    //bad header format
    printf("bad header format error\n");
    return;
  }
  //TODO: handling error during memcpy, ex. HOST:::::123
  //memcpy(header->name, buf, pname-buf);
  xstrncpy(header->name, buf, pname-buf);
  //아래줄을 실행하면 line의 uri가 깨지는 버그가 있다..
  //memcpy(header->data, pname+2, pdata-pname-2);
  xstrncpy(header->data, pname+2, pdata-pname-2);
  insert_header(header);
  printf("name: %s data: %s\n", header->name, header->data);
}

request_header* last_header() {
  request_header* temp;
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
  printf("line: %p\n", line);
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
//wrapper for safe strncpy
char* xstrncpy(char *dst, const char *src, size_t n)
{
    dst[0] = '\0';
      return strncat(dst, src, n );
}
