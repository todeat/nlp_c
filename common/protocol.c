#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

// Funcții pentru cereri normale
int send_request(int sockfd, Request* req) {
    // Trimitere tip cerere
    if (write(sockfd, &req->type, sizeof(req->type)) < 0) {
        return -1;
    }
    
    // Trimitere dimensiune text
    size_t text_len = strlen(req->text) + 1;
    if (write(sockfd, &text_len, sizeof(text_len)) < 0) {
        return -1;
    }
    
    // Trimitere text
    if (write(sockfd, req->text, text_len) < 0) {
        return -1;
    }
    
    return 0;
}

int receive_request(int sockfd, Request* req) {
    // Primire tip cerere
    if (read(sockfd, &req->type, sizeof(req->type)) <= 0) {
        return -1;
    }
    
    // Primire dimensiune text
    size_t text_len;
    if (read(sockfd, &text_len, sizeof(text_len)) <= 0) {
        return -1;
    }
    
    if (text_len > MAX_TEXT_SIZE) {
        return -1;
    }
    
    // Primire text
    if (read(sockfd, req->text, text_len) <= 0) {
        return -1;
    }
    
    return 0;
}

int send_response(int sockfd, Response* resp) {
    // Trimitere status
    if (write(sockfd, &resp->status, sizeof(resp->status)) < 0) {
        return -1;
    }
    
    if (resp->status == STATUS_OK) {
        // Trimitere număr cuvinte (pentru toate tipurile de cereri)
        if (write(sockfd, &resp->word_count, sizeof(resp->word_count)) < 0) {
            return -1;
        }
        
        // Trimitere timp de procesare
        if (write(sockfd, &resp->processing_time, sizeof(resp->processing_time)) < 0) {
            return -1;
        }
        
        // Trimitere topic (dacă există)
        if (resp->topic) {
            size_t topic_len = strlen(resp->topic) + 1;
            if (write(sockfd, &topic_len, sizeof(topic_len)) < 0) {
                return -1;
            }
            if (write(sockfd, resp->topic, topic_len) < 0) {
                return -1;
            }
        } else {
            size_t topic_len = 0;
            if (write(sockfd, &topic_len, sizeof(topic_len)) < 0) {
                return -1;
            }
        }
        
        // Trimitere rezumat (dacă există)
        if (resp->summary) {
            size_t summary_len = strlen(resp->summary) + 1;
            if (write(sockfd, &summary_len, sizeof(summary_len)) < 0) {
                return -1;
            }
            if (write(sockfd, resp->summary, summary_len) < 0) {
                return -1;
            }
        } else {
            size_t summary_len = 0;
            if (write(sockfd, &summary_len, sizeof(summary_len)) < 0) {
                return -1;
            }
        }
    } else {
        // Trimitere mesaj de eroare
        size_t error_len = strlen(resp->error_message) + 1;
        if (write(sockfd, &error_len, sizeof(error_len)) < 0) {
            return -1;
        }
        if (write(sockfd, resp->error_message, error_len) < 0) {
            return -1;
        }
    }
    
    return 0;
}

int receive_response(int sockfd, Response* resp) {
    // Primire status
    if (read(sockfd, &resp->status, sizeof(resp->status)) <= 0) {
        return -1;
    }
    
    if (resp->status == STATUS_OK) {
        // Primire număr cuvinte
        if (read(sockfd, &resp->word_count, sizeof(resp->word_count)) <= 0) {
            return -1;
        }
        
        // Primire timp de procesare
        if (read(sockfd, &resp->processing_time, sizeof(resp->processing_time)) <= 0) {
            return -1;
        }
        
        // Primire topic
        size_t topic_len;
        if (read(sockfd, &topic_len, sizeof(topic_len)) <= 0) {
            return -1;
        }
        
        if (topic_len > 0) {
            resp->topic = (char*)malloc(topic_len);
            if (!resp->topic) {
                return -1;
            }
            if (read(sockfd, resp->topic, topic_len) <= 0) {
                free(resp->topic);
                return -1;
            }
        } else {
            resp->topic = NULL;
        }
        
        // Primire rezumat
        size_t summary_len;
        if (read(sockfd, &summary_len, sizeof(summary_len)) <= 0) {
            if (resp->topic) free(resp->topic);
            return -1;
        }
        
        if (summary_len > 0) {
            resp->summary = (char*)malloc(summary_len);
            if (!resp->summary) {
                if (resp->topic) free(resp->topic);
                return -1;
            }
            if (read(sockfd, resp->summary, summary_len) <= 0) {
                if (resp->topic) free(resp->topic);
                free(resp->summary);
                return -1;
            }
        } else {
            resp->summary = NULL;
        }
    } else {
        // Primire mesaj de eroare
        size_t error_len;
        if (read(sockfd, &error_len, sizeof(error_len)) <= 0) {
            return -1;
        }
        
        if (error_len > MAX_ERROR_MSG) {
            return -1;
        }
        
        if (read(sockfd, resp->error_message, error_len) <= 0) {
            return -1;
        }
    }
    
    return 0;
}

// Funcții pentru cereri administrative
int send_admin_request(int sockfd, AdminRequest* req) {
    // Trimitere tip comandă
    if (write(sockfd, &req->command, sizeof(req->command)) < 0) {
        return -1;
    }
    
    return 0;
}

int receive_admin_request(int sockfd, AdminRequest* req) {
    // Primire tip comandă
    if (read(sockfd, &req->command, sizeof(req->command)) <= 0) {
        return -1;
    }
    
    return 0;
}

int send_admin_response(int sockfd, AdminResponse* resp) {
    // Trimitere status
    if (write(sockfd, &resp->status, sizeof(resp->status)) < 0) {
        return -1;
    }
    
    if (resp->status == STATUS_OK) {
        // Trimitere tipul de răspuns prin numărul de clienți
        if (write(sockfd, &resp->client_count, sizeof(resp->client_count)) < 0) {
            return -1;
        }
        
        // Trimitere queue status în orice caz (pentru compatibilitate)
        if (write(sockfd, &resp->queue_size, sizeof(resp->queue_size)) < 0) {
            return -1;
        }
        
        if (write(sockfd, &resp->queue_capacity, sizeof(resp->queue_capacity)) < 0) {
            return -1;
        }
        
        // Dacă avem clienți, trimitem informațiile despre ei
        if (resp->client_count > 0) {
            for (int i = 0; i < resp->client_count; i++) {
                if (write(sockfd, &resp->clients[i], sizeof(ClientInfo)) < 0) {
                    return -1;
                }
            }
        }
    } else {
        // Trimitere mesaj de eroare
        size_t error_len = strlen(resp->error_message) + 1;
        if (write(sockfd, &error_len, sizeof(error_len)) < 0) {
            return -1;
        }
        if (write(sockfd, resp->error_message, error_len) < 0) {
            return -1;
        }
    }
    
    return 0;
}

int receive_admin_response(int sockfd, AdminResponse* resp) {
    // Inițializare structură
    memset(resp, 0, sizeof(AdminResponse));
    
    // Primire status
    if (read(sockfd, &resp->status, sizeof(resp->status)) <= 0) {
        return -1;
    }
    
    if (resp->status == STATUS_OK) {
        // Primire client_count
        if (read(sockfd, &resp->client_count, sizeof(resp->client_count)) <= 0) {
            return -1;
        }
        
        // Primire queue status (mereu prezent)
        if (read(sockfd, &resp->queue_size, sizeof(resp->queue_size)) <= 0) {
            return -1;
        }
        
        if (read(sockfd, &resp->queue_capacity, sizeof(resp->queue_capacity)) <= 0) {
            return -1;
        }
        
        // Dacă avem clienți, primim informațiile despre ei
        if (resp->client_count > 0) {
            if (resp->client_count > MAX_CLIENTS) {
                return -1; // Protecție contra overflow
            }
            
            for (int i = 0; i < resp->client_count; i++) {
                if (read(sockfd, &resp->clients[i], sizeof(ClientInfo)) <= 0) {
                    return -1;
                }
            }
        }
    } else {
        // Primire mesaj de eroare
        size_t error_len;
        if (read(sockfd, &error_len, sizeof(error_len)) <= 0) {
            return -1;
        }
        
        if (error_len > MAX_ERROR_MSG) {
            return -1;
        }
        
        if (read(sockfd, resp->error_message, error_len) <= 0) {
            return -1;
        }
    }
    
    return 0;
}