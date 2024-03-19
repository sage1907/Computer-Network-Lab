#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

typedef struct {
    uint32_t sequence_number;
    uint8_t TTL;
    uint16_t payload_length;
    char payload[BUFFER_SIZE - 7]; // Subtracting size of sequence_number, TTL, and payload_length
} Datagram;

void handle_packet(int sockfd, struct sockaddr_in client_addr, Datagram packet) {
    if (packet.payload_length != strlen(packet.payload)) {
        printf("Received MALFORMED PACKET\n");
        sendto(sockfd, "MALFORMED PACKET", sizeof("MALFORMED PACKET"), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
        return;
    }

    packet.TTL--;
    sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <ServerPort>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    if (port <= 0) {
        printf("Invalid port number\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr, client_addr;
    int sockfd, len, n;
    Datagram packet;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    // Filling server information
    serv_addr.sin_family = AF_INET; // IPv4
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);

    while (1) {
        len = sizeof(client_addr);
        n = recvfrom(sockfd, (char *)&packet, sizeof(packet), MSG_WAITALL, (struct sockaddr *)&client_addr, &len);
        handle_packet(sockfd, client_addr, packet);
    }

    close(sockfd);
    return 0;
}