#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>

#define BUFFER_SIZE 1024

typedef struct {
    uint32_t sequence_number;
    uint8_t TTL;
    uint16_t payload_length;
    char payload[BUFFER_SIZE - 7]; // Subtracting size of sequence_number, TTL, and payload_length
} Datagram;

double calculate_rtt(struct timeval start, struct timeval end) {
    return (double)(end.tv_usec - start.tv_usec) / 1000.0;
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        printf("Usage: %s <ServerIP> <ServerPort> <P> <TTL> <NumPackets>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int P = atoi(argv[3]);
    int TTL = atoi(argv[4]);
    int num_packets = atoi(argv[5]);

    if (P < 100 || P > 1000 || TTL < 2 || TTL > 20 || (TTL % 2 != 0) || num_packets < 1 || num_packets > 50) {
        printf("Invalid input values\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in servaddr;
    Datagram packet;
    struct timeval start, end;
    double rtt_sum = 0;

    // Creating socket file descriptor
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &servaddr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    int n, len;
    for (int i = 0; i < num_packets; ++i) {
        packet.sequence_number = i;
        packet.TTL = TTL;
        sprintf(packet.payload, "Payload for packet %d", i);
        packet.payload_length = strlen(packet.payload);

        gettimeofday(&start, NULL);
        sendto(sockfd, (const char *)&packet, sizeof(packet), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));
        n = recvfrom(sockfd, (char *)&packet, sizeof(packet), MSG_WAITALL, NULL, NULL);
        gettimeofday(&end, NULL);

        double rtt = calculate_rtt(start, end);
        if (n < 0) {
            printf("Error receiving response for packet %d\n", i);
        } else {
            rtt_sum += rtt;
            printf("Round Trip Time for packet %d: %lf ms\n", i, rtt);
        }

        if (strcmp(packet.payload, "MALFORMED PACKET") == 0) {
            printf("Received MALFORMED PACKET for packet %d\n", i);
        }
    }

    printf("Average RTT: %lf ms\n", rtt_sum / num_packets);

    close(sockfd);
    return 0;
}