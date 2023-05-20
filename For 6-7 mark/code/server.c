#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define MAX_POT_SIZE 5
#define SPECTATOR_PORT 5000

int pot_size = 0;
pthread_mutex_t pot_mutex;

int spectator_sock;  // Socket for sending data to spectator
struct sockaddr_in spectator_addr;  // Address info for the spectator
socklen_t spectator_addr_len;

void update_spectator(const char* msg) {
    if (send(spectator_sock, msg, strlen(msg), 0) < 0) {
        perror("Spectator update error");
    }
}

void cook_food() {
    pthread_mutex_lock(&pot_mutex);
    pot_size = MAX_POT_SIZE;
    char msg[128];
    sprintf(msg, "Cook is preparing food. Pot is filled with %d pieces of stewed missionary.\n", pot_size);
    update_spectator(msg);
    pthread_mutex_unlock(&pot_mutex);
}

void eat_food() {
    pthread_mutex_lock(&pot_mutex);
    if (pot_size > 0) {
        pot_size--;
        char msg[128];
        sprintf(msg, "A barbarian is eating. Remaining pieces in pot: %d\n", pot_size);
        update_spectator(msg);
        pthread_mutex_unlock(&pot_mutex);
    } else {
        char msg[128];
        sprintf(msg, "The pot is empty. Barbarian is waking the cook.\n");
        update_spectator(msg);
        pthread_mutex_unlock(&pot_mutex);
        cook_food();
    }
}

void *spectator_handler(void *arg) {
    int server_sock = *(int *)arg;  // Retrieve server socket descriptor
    spectator_addr_len = sizeof(spectator_addr);
    if ((spectator_sock = accept(server_sock, (struct sockaddr *)&spectator_addr, &spectator_addr_len)) < 0) {
        perror("Accept spectator error");
    }

    update_spectator("Spectator connected.\n");
    return NULL;
}

void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);

        // Получение данных от клиента
        if (recv(client_sock, buffer, BUFFER_SIZE, 0) <= 0) {
            perror("Receive error");
            break;
        }

        // Обработка полученных данных
        if (strcmp(buffer, "eat") == 0) {
            eat_food();
        } else if (strcmp(buffer, "wake") == 0) {
            printf("Client is waking the cook.\n");
            cook_food();
        }
    }

    close(client_sock);
    free(arg);
    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    pthread_t tid;
    pthread_mutex_init(&pot_mutex, NULL);
    pthread_t spectator_tid;
    if (pthread_create(&spectator_tid, NULL, spectator_handler, &server_sock) != 0) {
        perror("Spectator thread creation error");
        return 1;
    }

    // Создание сокета
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return 1;
    }

    // Настройка адреса сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    // Привязка сокета к заданному адресу и порту
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding error");
        return 1;
    }

    // Прослушивание входящих соединений
    if (listen(server_sock, MAX_CLIENTS) < 0) {
        perror("Listening error");
        return 1;
    }

    printf("Server started. Waiting for clients...\n");

    while (1) {
        // Принятие входящего соединения
        client_addr_len = sizeof(client_addr);
        if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
            perror("Accept error");
            return 1;
        }

        printf("New client connected: %s\n", inet_ntoa(client_addr.sin_addr));

        // Создание отдельного потока для обработки клиента
        int *client_sock_ptr = (int *)malloc(sizeof(int));
        *client_sock_ptr = client_sock;

        if (pthread_create(&tid, NULL, handle_client, client_sock_ptr) != 0) {
            perror("Thread creation error");
            return 1;
        }

        // Освобождение ресурсов потока после завершения
        pthread_detach(tid);
    }

    pthread_mutex_destroy(&pot_mutex);
    close(server_sock);

    return 0;
}
