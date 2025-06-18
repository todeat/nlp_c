#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../common/protocol.h"

#define SERVER_IP "127.0.0.1"
#define PORT 12345
#define BUFFER_SIZE 8192

void print_help() {
    printf("Utilizare: client_bin COMANDA FIȘIER\n");
    printf("Comenzi disponibile:\n");
    printf("  --count-words FIȘIER       - Numără cuvintele din fișier\n");
    printf("  --determine-topic FIȘIER   - Determină domeniul tematic al fișierului\n");
    printf("  --generate-summary FIȘIER  - Generează un rezumat al fișierului\n");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        print_help();
        return 1;
    }

    RequestType request_type;
    
    if (strcmp(argv[1], "--count-words") == 0) {
        request_type = REQUEST_COUNT_WORDS;
    } else if (strcmp(argv[1], "--determine-topic") == 0) {
        request_type = REQUEST_DETERMINE_TOPIC;
    } else if (strcmp(argv[1], "--generate-summary") == 0) {
        request_type = REQUEST_GENERATE_SUMMARY;
    } else {
        printf("Comandă necunoscută: %s\n", argv[1]);
        print_help();
        return 1;
    }
    
    // Deschide fișierul specificat
    FILE *f = fopen(argv[2], "r");
    if (!f) {
        perror("Eroare la deschiderea fișierului");
        return 1;
    }
    
    // Determină mărimea fișierului
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (file_size > MAX_TEXT_SIZE) {
        printf("Fișierul este prea mare, maxim %d bytes permis\n", MAX_TEXT_SIZE);
        fclose(f);
        return 1;
    }
    
    // Citește conținutul fișierului
    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        perror("Eroare la alocarea memoriei");
        fclose(f);
        return 1;
    }
    
    size_t bytes_read = fread(buffer, 1, file_size, f);
    buffer[bytes_read] = '\0';
    fclose(f);
    
    // Conectare la server
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Eroare la crearea socket-ului");
        free(buffer);
        return 1;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Eroare la conectarea la server");
        free(buffer);
        close(sockfd);
        return 1;
    }
    
    // Pregătirea și trimiterea cererii
    Request request;
    request.type = request_type;
    strncpy(request.text, buffer, MAX_TEXT_SIZE - 1);
    request.text[MAX_TEXT_SIZE - 1] = '\0';
    
    if (send_request(sockfd, &request) < 0) {
        perror("Eroare la trimiterea cererii");
        free(buffer);
        close(sockfd);
        return 1;
    }
    
    // Primirea răspunsului
    Response response;
    if (receive_response(sockfd, &response) < 0) {
        perror("Eroare la primirea răspunsului");
        free(buffer);
        close(sockfd);
        return 1;
    }
    
    // Procesarea și afișarea răspunsului
    if (response.status == STATUS_OK) {
        switch (request_type) {
            case REQUEST_COUNT_WORDS:
                printf("Numărul de cuvinte: %d\n", response.word_count);
                printf("Timpul de procesare: %.2f secunde\n", response.processing_time);
                break;
                
            case REQUEST_DETERMINE_TOPIC:
                printf("Domeniul tematic: %s\n", response.topic);
                printf("Timpul de procesare: %.2f secunde\n", response.processing_time);
                break;
                
            case REQUEST_GENERATE_SUMMARY:
                printf("Rezumat:\n%s\n", response.summary);
                printf("Timpul de procesare: %.2f secunde\n", response.processing_time);
                break;
        }
    } else {
        printf("Eroare: %s\n", response.error_message);
    }
    
    // Bucla pentru cereri continue
    printf("\nVreți să faceți o altă cerere? (d/n): ");
    char choice;
    scanf(" %c", &choice);
    
    if (choice == 'd' || choice == 'D') {
        printf("Clientul rămâne conectat. Introduceți o nouă comandă...\n");
        // Aici ar trebui implementată logica pentru o nouă cerere
        // Pentru moment, doar menținem conexiunea deschisă
        printf("Pentru această demonstrație, clientul va rămâne conectat.\n");
        printf("Apăsați Enter pentru a închide conexiunea...\n");
        getchar(); // consumă newline
        getchar(); // așteaptă Enter
    }
    
    free(buffer);
    close(sockfd);
    
    return 0;
}