#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <poll.h>
#include "../common/nlp.h"
#include "../common/protocol.h"
#include <arpa/inet.h> 

#define TCP_PORT 12345
#define UNIX_SOCKET_PATH "/tmp/nlp_admin_socket"
#define MAX_CLIENTS 10
#define BUFFER_SIZE 8192
#define MAX_QUEUE_SIZE 100

// Structura pentru o cerere de procesare
typedef struct {
    int client_fd;
    char* text;
    RequestType type; // Definit în protocol.h
} ProcessingRequest;

// Coada FIFO pentru cererile de procesare
typedef struct {
    ProcessingRequest queue[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} RequestQueue;


// Variabile globale
RequestQueue request_queue;
ClientInfo clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int client_count = 0;


void init_queue() {
    request_queue.front = 0;
    request_queue.rear = -1;
    request_queue.count = 0;
    pthread_mutex_init(&request_queue.mutex, NULL);
    pthread_cond_init(&request_queue.not_empty, NULL);
    pthread_cond_init(&request_queue.not_full, NULL);
}

// Adaugare cerere in coada
int enqueue(ProcessingRequest request) {
    pthread_mutex_lock(&request_queue.mutex);
    
    while (request_queue.count >= MAX_QUEUE_SIZE) {
        pthread_cond_wait(&request_queue.not_full, &request_queue.mutex);
    }
    
    request_queue.rear = (request_queue.rear + 1) % MAX_QUEUE_SIZE;
    request_queue.queue[request_queue.rear] = request;
    request_queue.count++;
    
    pthread_cond_signal(&request_queue.not_empty);
    pthread_mutex_unlock(&request_queue.mutex);
    return 0;
}

// Extragere cerere din coada
ProcessingRequest dequeue() {
    pthread_mutex_lock(&request_queue.mutex);
    
    while (request_queue.count <= 0) {
        pthread_cond_wait(&request_queue.not_empty, &request_queue.mutex);
    }
    
    ProcessingRequest request = request_queue.queue[request_queue.front];
    request_queue.front = (request_queue.front + 1) % MAX_QUEUE_SIZE;
    request_queue.count--;
    
    pthread_cond_signal(&request_queue.not_full);
    pthread_mutex_unlock(&request_queue.mutex);
    return request;
}

void* processing_thread(void* arg) {
    // Init clasificatorul Bayes si colectia de documente
    static BayesClassifier* classifier = NULL;
    static DocumentCollection* collection = NULL;
    
    if (!classifier) {
        classifier = init_bayes_classifier();
        
        train_bayes_classifier(classifier, 
            "Meciul de fotbal s-a terminat cu scorul de 2-1. Jucătorii au fost foarte buni.",
            "Sport");
        train_bayes_classifier(classifier,
            "Echipa națională a câștigat campionatul. Fotbaliștii au jucat excelent în finală.",
            "Sport");
        
        train_bayes_classifier(classifier,
            "Președintele a anunțat noi măsuri economice. Parlamentul va dezbate legea mâine.",
            "Politică");
        train_bayes_classifier(classifier,
            "Guvernul a aprobat noul buget. Opoziția critică deciziile luate de partidul de guvernare.",
            "Politică");
        
        train_bayes_classifier(classifier,
            "Noul smartphone are funcții avansate de inteligență artificială și baterie performantă.",
            "Tehnologie");
        train_bayes_classifier(classifier,
            "Inteligența artificială revoluționează industria. Sistemele de învățare automată procesează date masive.",
            "Tehnologie");
        train_bayes_classifier(classifier,
            "Algoritmii de machine learning și rețelele neurale sunt la baza multor aplicații moderne.",
            "Tehnologie");
        train_bayes_classifier(classifier,
            "Companiile tech investesc în dezvoltarea de soluții bazate pe AI și automatizare.",
            "Tehnologie");
        
    }
    
    if (!collection) {
        collection = (DocumentCollection*)malloc(sizeof(DocumentCollection));
        collection->document_count = 0;
        collection->documents = NULL;
    }
    
    while (1) {
        ProcessingRequest request = dequeue();
        time_t start_time = time(NULL);
        
        Response response;
        response.status = STATUS_OK;
        response.topic = NULL;
        response.summary = NULL;
        

        collection->documents = realloc(collection->documents, 
                                       (collection->document_count + 1) * sizeof(char*));
        if (collection->documents) {
            collection->documents[collection->document_count] = strdup(request.text);
            collection->document_count++;
        }
        
        switch (request.type) {
            case REQUEST_COUNT_WORDS:
                response.word_count = count_words(request.text);
                break;
                
                case REQUEST_DETERMINE_TOPIC:
                {
                    
                    response.topic = classify_text_bayes(classifier, request.text);
                    
                    
                    if (strcmp(response.topic, "Necunoscut") == 0) {
                        free(response.topic); 
                        response.topic = determine_topic(request.text);
                    }
                    
                    if (strcmp(response.topic, "Necunoscut") != 0 && 
                        strcmp(response.topic, "Eroare la procesare") != 0) {
                        train_bayes_classifier(classifier, request.text, response.topic);
                    }
                    break;
                }                
                
            case REQUEST_GENERATE_SUMMARY:
                
                response.summary = generate_summary(request.text, 3, collection);
                break;

                
            default:
                response.status = STATUS_ERROR;
                strcpy(response.error_message, "Tip de cerere necunoscut");
        }
        
        if (response.status == STATUS_OK && request.type != REQUEST_COUNT_WORDS) {
            response.word_count = count_words(request.text);
        }
        
        time_t end_time = time(NULL);
        response.processing_time = difftime(end_time, start_time);
        
        send_response(request.client_fd, &response);
        
        
        free(request.text);
        if (response.topic) free(response.topic);
        if (response.summary) free(response.summary);
    }
    
    return NULL;
}


void remove_client(int client_fd) {
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < client_count; i++) {
        if (clients[i].fd == client_fd) {
            // Muta ultimul client pe poz curr
            if (i < client_count - 1) {
                clients[i] = clients[client_count - 1];
            }
            client_count--;
            break;
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

void* client_handler(void* arg) {
    int client_fd = *((int*)arg);
    free(arg);
    
    // Bucla pentru gestionarea mai multor cereri de la acc client
    while (1) {
        // Primire cerere
        Request req;
        if (receive_request(client_fd, &req) < 0) {
            // ELIMINARE CLIENT LA DECONECTARE
            remove_client(client_fd);
            close(client_fd);
            return NULL;
        }
        
        // Creare cerere de procesare
        ProcessingRequest proc_req;
        proc_req.client_fd = client_fd;
        proc_req.type = req.type;
        proc_req.text = strdup(req.text);
        
        // Add in coada de procesare
        enqueue(proc_req);
        
        // Actualizare info client
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < client_count; i++) {
            if (clients[i].fd == client_fd) {
                clients[i].request_count++;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }
    
    return NULL;
}

// Administrare client 
void handle_admin_client(int admin_fd) {
    AdminRequest admin_req;
    if (receive_admin_request(admin_fd, &admin_req) < 0) {
        close(admin_fd);
        return;
    }
    
    AdminResponse admin_resp;
    memset(&admin_resp, 0, sizeof(AdminResponse)); // Init completa
    admin_resp.status = STATUS_OK; // Setare status implicit OK
    
    switch (admin_req.command) {
        case ADMIN_GET_CLIENTS:
            pthread_mutex_lock(&clients_mutex);
            admin_resp.client_count = client_count;
            // Copiaza info despre clienti
            for (int i = 0; i < client_count && i < MAX_CLIENTS; i++) {
                admin_resp.clients[i] = clients[i];
            }
            
            pthread_mutex_lock(&request_queue.mutex);
            admin_resp.queue_size = request_queue.count;
            admin_resp.queue_capacity = MAX_QUEUE_SIZE;
            pthread_mutex_unlock(&request_queue.mutex);
            pthread_mutex_unlock(&clients_mutex);
            break;
            
        case ADMIN_GET_QUEUE_STATUS:
            admin_resp.client_count = 0; 
            pthread_mutex_lock(&request_queue.mutex);
            admin_resp.queue_size = request_queue.count;
            admin_resp.queue_capacity = MAX_QUEUE_SIZE;
            pthread_mutex_unlock(&request_queue.mutex);
            break;
            
        default:
            admin_resp.status = STATUS_ERROR;
            strcpy(admin_resp.error_message, "Comandă de administrare necunoscută");
            break;
    }
    
    if (send_admin_response(admin_fd, &admin_resp) < 0) {
        perror("Eroare la trimiterea răspunsului administrativ");
    }
    
    close(admin_fd);
}

int main() {
    int tcp_fd, unix_fd;
    struct sockaddr_in tcp_addr;
    struct sockaddr_un unix_addr;
    
    
    init_queue();
    
    
    tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_fd < 0) {
        perror("Eroare la crearea socket-ului TCP");
        exit(1);
    }
    
    int opt = 1;
    setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    tcp_addr.sin_port = htons(TCP_PORT);
    
    if (bind(tcp_fd, (struct sockaddr*)&tcp_addr, sizeof(tcp_addr)) < 0) {
        perror("Eroare la bind pentru socket-ul TCP");
        exit(1);
    }
    
    listen(tcp_fd, 5);
    
    unix_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (unix_fd < 0) {
        perror("Eroare la crearea socket-ului UNIX");
        exit(1);
    }
    
    unlink(UNIX_SOCKET_PATH); 
    
    unix_addr.sun_family = AF_UNIX;
    strcpy(unix_addr.sun_path, UNIX_SOCKET_PATH);
    
    if (bind(unix_fd, (struct sockaddr*)&unix_addr, sizeof(unix_addr)) < 0) {
        perror("Eroare la bind pentru socket-ul UNIX");
        exit(1);
    }
    
    listen(unix_fd, 5);
    



    pthread_t processing_tid;
    pthread_create(&processing_tid, NULL, processing_thread, NULL);
    
    struct pollfd fds[2];
    fds[0].fd = tcp_fd;
    fds[0].events = POLLIN;
    fds[1].fd = unix_fd;
    fds[1].events = POLLIN;
    
    printf("Serverul așteaptă conexiuni...\n");
    
    while (1) {
        int poll_result = poll(fds, 2, -1);
        
        if (poll_result < 0) {
            perror("Eroare la poll");
            break;
        }
        
        if (fds[0].revents & POLLIN) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            int* client_fd = malloc(sizeof(int));
            *client_fd = accept(tcp_fd, (struct sockaddr*)&client_addr, &client_len);
            
            if (*client_fd < 0) {
                perror("Eroare la accept pentru client normal");
                free(client_fd);
                continue;
            }
            
            pthread_mutex_lock(&clients_mutex);
            if (client_count < MAX_CLIENTS) {
                clients[client_count].fd = *client_fd;
                inet_ntop(AF_INET, &client_addr.sin_addr, clients[client_count].address, sizeof(clients[client_count].address));
                clients[client_count].connect_time = time(NULL);
                clients[client_count].request_count = 0;
                client_count++;
            }
            pthread_mutex_unlock(&clients_mutex);
            
            pthread_t client_tid;
            pthread_create(&client_tid, NULL, client_handler, client_fd);
            pthread_detach(client_tid);
        }
        
        if (fds[1].revents & POLLIN) {
            struct sockaddr_un admin_addr;
            socklen_t admin_len = sizeof(admin_addr);
            
            int admin_fd = accept(unix_fd, (struct sockaddr*)&admin_addr, &admin_len);
            
            if (admin_fd < 0) {
                perror("Eroare la accept pentru client de administrare");
                continue;
            }
            

            handle_admin_client(admin_fd);
        }
    }
    
    close(tcp_fd);
    close(unix_fd);
    unlink(UNIX_SOCKET_PATH);
    
    return 0;
}