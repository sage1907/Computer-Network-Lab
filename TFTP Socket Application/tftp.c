#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define SERVER_PORT 69 // Default TFTP port
#define BUFFER_SIZE 516 // 4 bytes header + 512 bytes data
#define TIMEOUT_SEC 5 // Timeout in seconds

// TFTP Opcodes
#define OPCODE_RRQ 1
#define OPCODE_DATA 3
#define OPCODE_ACK 4

void create_rrq_packet(char *buffer, const char *filename, const char *mode) {
    int position = 0;
    // Opcode for RRQ
    buffer[position++] = 0;
    buffer[position++] = OPCODE_RRQ;
    // Filename
    strcpy(&buffer[position], filename);
    position += strlen(filename) + 1;
    // Mode
    strcpy(&buffer[position], mode);
    position += strlen(mode) + 1;
}

void die_with_error(char *errorMessage) {
    perror(errorMessage);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];
    char *server_ip;
    char *filename;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server IP> <Filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    server_ip = argv[1];
    filename = argv[2];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        die_with_error("socket() failed");
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(server_ip);

    create_rrq_packet(buffer, filename, "octet");

    if (sendto(sockfd, buffer, strlen(filename) + strlen("octet") + 4, 0,
               (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        die_with_error("sendto() failed");
    }

    struct sockaddr_in fromAddr;
    unsigned int fromAddrLen = sizeof(fromAddr);
    int recvMsgSize;
    unsigned short block = 1;
    FILE *file = fopen(filename, "wb");
    if (!file) {
        die_with_error("Failed to open file for writing");
    }

    while (1) {
        if ((recvMsgSize = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                    (struct sockaddr *)&fromAddr, &fromAddrLen)) < 0) {
            die_with_error("recvfrom() failed");
        }

        // Check for DATA opcode
        if (buffer[1] == OPCODE_DATA) {
            unsigned short receivedBlock = (unsigned char)buffer[2] << 8 | (unsigned char)buffer[3];
            if (receivedBlock == block) {
                int dataLen = recvMsgSize - 4; // Subtract the 4-byte header
                fwrite(buffer + 4, sizeof(char), dataLen, file);

                // Send ACK
                buffer[1] = OPCODE_ACK;
                if (sendto(sockfd, buffer, 4, 0, (struct sockaddr *)&fromAddr, fromAddrLen) < 0) {
                    die_with_error("sendto() failed on ACK");
                }

                block++;
                if (dataLen < 512) { // Last packet
                    printf("File transfer complete.\n");
                    break;
                }
            }
        } else {
            fprintf(stderr, "Received unexpected packet.\n");
            break;
        }
    }

    fclose(file);
    close(sockfd);

    return 0;
}
