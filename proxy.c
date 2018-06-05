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

typedef struct cache_node
{
  char hostname[200];
  char path[1000];
  size_t size;
  char* data;
  struct cache_node* next;
  clock_t used;
} cache_node;

cache_node* root_c;
int cache_volume=0;

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3";
static request_header *root= NULL;
void *handle_request(void*);
void parse_line(request_line *, char*);
void parse_header(char*, request_line*);//todo remove request_line*
request_header* last_header();
void print_headers();
void insert_header(request_header*);
request_header* find_header_by_key(char*);
void print_line(request_line*);
void send_request(int, request_line*);
void make_header(request_line*);
void free_line_header(request_line*, request_header*);
char* xstrncpy(char *, const char*, size_t);
request_header* find_in_header(char*);
void print_request(char []);
void initialize_cache();
void delete_cache(cache_node*);
void update_time(cache_node*);
cache_node* search_cache(char [], char []);
void print_cache();

int main(int argc, char **argv)

{
  char *port;
  int listenfd, *connfd;
  socklen_t clientlen;
  struct sockaddr_in clientaddr;
  pthread_t tid;
  printf("%s", user_agent_hdr);

  //show usage error message
  if (argc != 2)
  {
    printf("Usage: ./proxy <port_number>\n");
  }
  //initialize cache
  root_c = malloc(sizeof(cache_node));
  initialize_cache();
  port  = argv[1];
  listenfd = Open_listenfd(port); //Open_listenfd => socket(), bind(), listen()

  while(1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Malloc(sizeof(int));
    *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //typedef struct sockaddr SA;
    Pthread_create(&tid, NULL, handle_request, connfd);
  }
  return 0;
}

void initialize_cache() {
  strcpy(root_c->hostname, "");
  strcpy(root_c->path, "");
  root_c->size= 0;
  root_c->data= NULL;
  root_c->next = NULL;
  root_c->used= clock();
}

void *handle_request(void* vargp)
{
  int fd = *((int*)vargp);
  free(vargp);
  Pthread_detach(pthread_self());
  request_line* line=Malloc(sizeof(request_line));
  //open rio of client input
  rio_t rio;
  char buf[MAXLINE];

  Rio_readinitb(&rio, fd);
  //read line
  Rio_readlineb(&rio, buf, MAXLINE);
  parse_line(line, buf);
  memset(&buf[0], 0, sizeof(buf)); //flushing buffer

  //read all header
  Rio_readlineb(&rio, buf, MAXLINE);
  while(strcmp(buf, "\r\n"))
  {
    parse_header(buf, line);
    memset(&buf[0], 0, sizeof(buf)); //flushing buffer
    Rio_readlineb(&rio, buf, MAXLINE);
  }
  memset(&buf[0], 0, sizeof(buf)); //flushing buffer
  make_header(line);
  //print_line(line);
  //print_headers();
  send_request(fd, line);
  //free_line_header(line, root); //why... not working?
  return NULL;
}

cache_node* search_cache(char path[], char hostname[])
{
  cache_node* temp = root_c;
  while(temp!= NULL)
  {
    if(strcmp(path, temp-> path) == 0){
      if(strcmp(hostname, temp-> hostname) == 0){
        return temp;
      }
    }
    temp = temp->next;
  }
  return NULL;
}
cache_node* create_cache() {
  cache_node* temp = root_c;
  cache_node* target;
  //go to next most
  while(temp -> next !=NULL){
    temp = temp->next;
  }
  target =malloc(sizeof(cache_node));
  temp->next = target;
  target->next = NULL;
  return target;
}

void update_time(cache_node* target)
{
  target->used = clock();
  return;
}
cache_node* LRU() {
  cache_node* temp = root_c;
  temp = temp->next;
  cache_node* eviction = temp;
  clock_t least = temp -> used;
  while(temp){
    if(least > temp->used){
      least = temp->used;
      eviction = temp;
    }
    temp = temp->next;
  }
  return eviction;
}

void delete_cache(cache_node* eviction) {
  cache_node* temp = root_c;
  if(!eviction) return;
  if(!temp) return;
  while((temp->next != eviction) && (temp)){
    temp = temp->next;
  }
  if(!temp) return;
  temp->next = eviction ->next;
  cache_volume -= (eviction->size);
  free(eviction->data);
  free(eviction);
  return;
}


void send_request(int fd, request_line* line)
{
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
  //print_request(request_buf);

  request_header* header_host= find_header_by_key("Host");
  //search cashe
  cache_node* pcached= search_cache(line->path, header_host->data); //path && host 
  if(pcached) {
    Rio_writen(fd, pcached->data, pcached->size);
    update_time(pcached);
    Close(fd);
    return;
  }

  //open request file descriptor
  requestfd= Open_clientfd(request_domain, request_port);

  Rio_writen(requestfd, request_buf, strlen(request_buf));
  //recieve response
  rio_t rio;
  char read_buf[MAXLINE];
  ssize_t n=0;
  // caching
  char cache_candidate[MAX_OBJECT_SIZE];
  char *cache_ptr = cache_candidate;
  int cachable=1;
  Rio_readinitb(&rio, requestfd);
  while((n = Rio_readnb(&rio, read_buf, MAXLINE))>0) //cannot use lineb cause interface of Rio_writen
  {
    //write to client of proxy
    Rio_writen(fd, read_buf, (size_t)n);
    if(cachable)
    {
      if((n+(cache_ptr-cache_candidate)) <= MAX_OBJECT_SIZE){
        xstrncpy(cache_ptr, read_buf, n); 
        cache_ptr += n;
      }
      else {
        cachable=0;
      }
    }
  }
  //do caching
  if(cachable) {
    int current_size= cache_ptr - cache_candidate;
    if((cache_volume + current_size) <= MAX_CACHE_SIZE){
      // noeviction
      cache_node* new_cache = create_cache();
      new_cache ->size = current_size;
      strcpy(new_cache -> hostname, header_host->data);
      strcpy(new_cache -> path, line->path);
      new_cache -> data = malloc(current_size);
      xstrncpy(new_cache ->data, cache_candidate, current_size);
      cache_volume += current_size;
      update_time(new_cache);
      //print_cache();
    }
    else {
      //eviction
      while((cache_volume + current_size > MAX_CACHE_SIZE)){
        cache_node* eviction = LRU();
        delete_cache(eviction);
      }
      cache_node* new_cache = create_cache();
      new_cache->size = current_size;
      strcpy(new_cache -> hostname, header_host->data);
      strcpy(new_cache -> path, line->path);
      new_cache -> data = malloc(current_size);
      xstrncpy(new_cache->data, cache_candidate, current_size);
      cache_volume += current_size;
      update_time(new_cache);
    }
  }
  Close(requestfd);
  Close(fd);
}

void print_request(char requestbuf[]){
  printf("=====print_request=====\n");
  puts(requestbuf);
}
void make_header(request_line* line)
{
  request_header* find=NULL;

  find = find_header_by_key("Host");
  if(!find){
    request_header* new_header=Malloc(sizeof(request_header));
    strcpy(new_header->name, "Host");
    strcpy(new_header->data, line->hostname);
    insert_header(new_header);
  }
  find = NULL;

  find= find_header_by_key("User-Agent");
  if(!find){
    request_header* new_header=Malloc(sizeof(request_header));
    strcpy(new_header->name, "User-Agent");
    strcpy(new_header->data, user_agent_hdr);
    insert_header(new_header);
  }
  find = NULL;

  find= find_header_by_key("Connection");
  if(!find){
    request_header* new_header=Malloc(sizeof(request_header));
    strcpy(new_header->name, "Connection");
    strcpy(new_header->data, "close");
    insert_header(new_header);
  }
  find = NULL;

  find= find_header_by_key("Proxy-Connection");
  if(!find){
    request_header* new_header=Malloc(sizeof(request_header));
    strcpy(new_header->name, "Proxy-Connection");
    strcpy(new_header->data, "close");
    insert_header(new_header);
  }
  find = NULL;

  //1.headers를 돌면서 HOSTㄹ인 헤더ㄹ를 찾아라 찾아서 Host에 넣어주기
  //2.없으면 URi에서 뜯어라
  //3.single line으로 USer-Agent header 보내주기
  //4.Connection: close
  //5.Proxy-Connection: close 헤더를 포함해라.
}

request_header* find_header_by_key(char* type)
{
  // Host, User-Agent, Connection, Proxy-Connection
  request_header* temp= malloc(sizeof(request_header*));
  temp = root;
  while(temp){
    if(strcmp(temp->name, type)==0) {
      return temp;
    }
    else {
      temp = temp->next;
    }
  }
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
  char method[10];
  char uri[300];
  char version[100];
  char *host_start;
  char *path_start;
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
    xstrncpy(line->hostname, host_start, path_start-host_start); //strcpy_s is only for windows
    strcpy(line->path, path_start);
  }

  if(strcmp("GET", method) !=0){
    printf("Only GET method can be accepted\n");
  }
  //buf: GET http://www.example.com HTTP/1.0
}

void parse_header(char *buf, request_line* line)
{
  request_header* header = Malloc(sizeof(request_header));
  char* pname= strstr(buf, ": ");
  char* pdata= strstr(buf, "\r\n");
  if((!pname)||(!pdata))
  {
    //bad header format
    printf("bad header format error\n");
    return;
  }
  xstrncpy(header->name, buf, pname-buf);
  xstrncpy(header->data, pname+2, pdata-pname-2);
  insert_header(header);
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
void print_cache() {
  cache_node* temp = root_c;
  int i=0;
  printf("=== print_cache start: === \n");
  while(temp){
    printf("cache[%d] : \n", i++);
    printf("hostname: %s\n", temp->hostname);
    printf("path: %s\n", temp->path);
    //printf("data: \n(%s)\n", temp->data);
    printf("used: %ld\n", temp->used);
    temp = temp -> next;
  }
  printf("=== print_cache ended:  === \n");
}

