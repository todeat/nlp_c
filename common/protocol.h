#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <time.h>

#define MAX_TEXT_SIZE 65536
#define MAX_ERROR_MSG 256
#define MAX_CLIENTS 10

// Tipuri de cereri
typedef enum {
    REQUEST_COUNT_WORDS = 1,
    REQUEST_DETERMINE_TOPIC = 2,
    REQUEST_GENERATE_SUMMARY = 3
} RequestType;

// Stări răspuns
typedef enum {
    STATUS_OK = 0,
    STATUS_ERROR = 1
} StatusCode;

// Structura pentru cereri
typedef struct {
    RequestType type;
    char text[MAX_TEXT_SIZE];
} Request;

// Structura pentru răspunsuri
typedef struct {
    StatusCode status;
    int word_count;
    char* topic;
    char* summary;
    double processing_time;
    char error_message[MAX_ERROR_MSG];
} Response;

// Tipuri de comenzi administrative
typedef enum {
    ADMIN_GET_CLIENTS = 1,
    ADMIN_GET_QUEUE_STATUS = 2
} AdminCommandType;

// Structura pentru cereri administrative
typedef struct {
    AdminCommandType command;
} AdminRequest;

// Informații despre un client
typedef struct {
    int fd;
    char address[50];
    time_t connect_time;
    int request_count;
} ClientInfo;

// Structura pentru răspunsuri administrative
typedef struct {
    StatusCode status;
    int client_count;
    ClientInfo clients[MAX_CLIENTS];
    int queue_size;
    int queue_capacity;
    char error_message[MAX_ERROR_MSG];
} AdminResponse;

// Funcții pentru comunicare
int send_request(int sockfd, Request* req);
int receive_request(int sockfd, Request* req);
int send_response(int sockfd, Response* resp);
int receive_response(int sockfd, Response* resp);
int send_admin_request(int sockfd, AdminRequest* req);
int receive_admin_request(int sockfd, AdminRequest* req);
int send_admin_response(int sockfd, AdminResponse* resp);
int receive_admin_response(int sockfd, AdminResponse* resp);

#endif