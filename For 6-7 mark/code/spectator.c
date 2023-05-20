#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define SPECTATOR_PORT 8081

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <ip>\n", argv[0]);
        return 1;
    }

    int spectator_sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    struct in_addr addr;
    if (inet_aton(argv[1], &addr) == 0) {
	fprintf(stderr, "Invalid IP address\n");
	return 1;
    }
    server_addr.sin_addr = addr;

    // Create socket
    if ((spectator_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return 1;
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(SPECTATOR_PORT);

    // Connect to server
    if (connect(spectator_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection error");
        return 1;
    }

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);

        // Receive data from server
        if (recv(spectator_sock, buffer, BUFFER_SIZE, 0) <= 0) {
            perror("Receive error");
            break;
        }

        // Print received data
        printf("%s\n", buffer);
    }

    close(spectator_sock);

    return 0;
}
