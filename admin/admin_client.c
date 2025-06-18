#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include "../common/protocol.h"

#define UNIX_SOCKET_PATH "/tmp/nlp_admin_socket"

void print_help() {
    printf("Utilizare: admin_client COMANDA\n");
    printf("Comenzi disponibile:\n");
    printf("  --clients        - Afișează informații despre clienții conectați\n");
    printf("  --queue-status   - Afișează starea cozii de procesare\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        print_help();
        return 1;
    }

    AdminCommandType command_type;
    
    if (strcmp(argv[1], "--clients") == 0) {
        command_type = ADMIN_GET_CLIENTS;
    } else if (strcmp(argv[1], "--queue-status") == 0) {
        command_type = ADMIN_GET_QUEUE_STATUS;
    } else {
        printf("Comandă necunoscută: %s\n", argv[1]);
        print_help();
        return 1;
    }
    
    // connect to UNIX socket
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Eroare la crearea socket-ului");
        return 1;
    }
    
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, UNIX_SOCKET_PATH);
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Eroare la conectarea la server");
        close(sockfd);
        return 1;
    }
    
    // send req
    AdminRequest request;
    request.command = command_type;
    
    if (send_admin_request(sockfd, &request) < 0) {
        perror("Eroare la trimiterea cererii administrative");
        close(sockfd);
        return 1;
    }
    
    // get resp
    AdminResponse response;
    if (receive_admin_response(sockfd, &response) < 0) {
        perror("Eroare la primirea răspunsului administrativ");
        close(sockfd);
        return 1;
    }
    
    // proccess and print resp
    if (response.status == STATUS_OK) {
        switch (command_type) {
            case ADMIN_GET_CLIENTS: {
                printf("Număr total de clienți: %d\n", response.client_count);
                printf("%-5s %-20s %-25s %-15s\n", "ID", "Adresă", "Conectat la", "Cereri");
                printf("-------------------------------------------------------------\n");
                
                for (int i = 0; i < response.client_count; i++) {
                    char time_str[30];
                    struct tm *tm_info = localtime(&response.clients[i].connect_time);
                    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
                    
                    printf("%-5d %-20s %-25s %-15d\n", 
                           i + 1, 
                           response.clients[i].address, 
                           time_str, 
                           response.clients[i].request_count);
                }
                break;
            }
                
            case ADMIN_GET_QUEUE_STATUS:
                printf("Starea cozii de procesare:\n");
                printf("Cereri în așteptare: %d / %d\n", 
                       response.queue_size, 
                       response.queue_capacity);
                break;
        }
    } else {
        printf("Eroare: %s\n", response.error_message);
    }
    
    close(sockfd);
    return 0;
}