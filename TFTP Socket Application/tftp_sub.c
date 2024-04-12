#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define SERVER_PORT 69  // Default TFTP port
#define BUFFER_SIZE 516 // 4 bytes header + 512 bytes data
#define TIMEOUT_SEC 5   // Timeout in seconds

// TFTP Opcodes
#define OPCODE_RRQ 1
#define OPCODE_WRQ 2
#define OPCODE_DATA 3
#define OPCODE_ACK 4
#define OPCODE_ERROR 5

void create_rrq_packet(char *buffer, const char *filename, const char *mode)
{
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

void create_wrq_packet(char *buffer, const char *filename, const char *mode)
{
    int position = 0;
    // Opcode for WRQ
    buffer[position++] = 0;
    buffer[position++] = OPCODE_WRQ;
    // Filename
    strcpy(&buffer[position], filename);
    position += strlen(filename) + 1;
    // Mode
    strcpy(&buffer[position], mode);
    position += strlen(mode) + 1;
}

void die_with_error(char *errorMessage)
{
    perror(errorMessage);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];
    char *server_ip;
    char *filename;
    char *operation;

    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <Server IP> <get/put> <Filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    server_ip = argv[1];
    operation = argv[2];
    filename = argv[3];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        die_with_error("socket() failed");
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(server_ip);

    if (strcmp(operation, "get") == 0)
    {
        create_rrq_packet(buffer, filename, "octet");
    }
    else if (strcmp(operation, "put") == 0)
    {
        create_wrq_packet(buffer, filename, "octet");
    }
    else
    {
        fprintf(stderr, "Invalid operation. Must be 'get' or 'put'\n");
        exit(EXIT_FAILURE);
    }

    if (sendto(sockfd, buffer, strlen(filename) + strlen("octet") + 4, 0,
               (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        die_with_error("sendto() failed");
    }

    struct sockaddr_in fromAddr;
    unsigned int fromAddrLen = sizeof(fromAddr);
    int recvMsgSize;
    unsigned short block = 1;
    FILE *file;

    if (strcmp(operation, "get") == 0)
    {
        file = fopen(filename, "wb");
        if (!file)
        {
            die_with_error("Failed to open file for writing");
        }

        while (1)
        {
            if ((recvMsgSize = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                        (struct sockaddr *)&fromAddr, &fromAddrLen)) < 0)
            {
                die_with_error("recvfrom() failed");
            }

            // Check for DATA opcode
            if (buffer[1] == OPCODE_DATA)
            {
                unsigned short receivedBlock = (unsigned char)buffer[2] << 8 | (unsigned char)buffer[3];
                if (receivedBlock == block)
                {
                    int dataLen = recvMsgSize - 4; // Subtract the 4-byte header
                    fwrite(buffer + 4, sizeof(char), dataLen, file);

                    // Send ACK
                    buffer[1] = OPCODE_ACK;
                    if (sendto(sockfd, buffer, 4, 0, (struct sockaddr *)&fromAddr, fromAddrLen) < 0)
                    {
                        die_with_error("sendto() failed on ACK");
                    }

                    block++;
                    if (dataLen < 512)
                    { // Last packet
                        printf("File transfer complete.\n");
                        break;
                    }
                }
            }
            else if (buffer[1] == OPCODE_ERROR)
            {
                fprintf(stderr, "Received ERROR packet: %s\n", buffer + 4); // Print error message
                exit(EXIT_FAILURE);                                         // Exit program
            }
            else
            {
                fprintf(stderr, "Received unexpected packet.\n");
                break;
            }
        }

        fclose(file);
    }
    else if (strcmp(operation, "put") == 0)
    {
        file = fopen(filename, "rb");
        if (!file)
        {
            die_with_error("Failed to open file for reading");
        }

       
	if ((recvMsgSize = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                        (struct sockaddr *)&fromAddr, &fromAddrLen)) < 0)
            {
                die_with_error("recvfrom() failed");
            }
	printf("Giving data blocks\n");

       	while (1)
        {
            // Read data from file
            int dataLen = fread(buffer + 4, sizeof(char), 512, file);
            if (dataLen <= 0)
            {
                // End of file reached
                break;
            }

            // Construct DATA packet
            buffer[0] = 0;
            buffer[1] = OPCODE_DATA;
            buffer[2] = (block >> 8) & 0xFF; // Block number high byte
            buffer[3] = block & 0xFF;        // Block number low byte

            // Send DATA packet
            if (sendto(sockfd, buffer, dataLen + 4, 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
            {
                die_with_error("sendto() failed on DATA");
            }

            // Receive ACK
            if ((recvMsgSize = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                        (struct sockaddr *)&fromAddr, &fromAddrLen)) < 0)
            {
                die_with_error("recvfrom() failed");
            }
		
	   
            // Check ACK
           /*
	    if (buffer[1] != OPCODE_ACK && ((unsigned char)buffer[2] << 8 | (unsigned char)buffer[3]) != block)
            {
                fprintf(stderr, "Received invalid ACK.\n");
                break;
            }

            block++;
	    */

	    if (buffer[1] == OPCODE_ACK) {
                unsigned short receivedBlock = (unsigned char)buffer[2] << 8 | (unsigned char)buffer[3];
                if (receivedBlock == block) {
                    block++;
                    if (dataLen < 512) {
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
    }

    close(sockfd);

    return 0;
}