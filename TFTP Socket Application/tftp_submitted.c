#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERV_PORT 69    // Default TFTP port
#define BUF_SIZE 516    // 4 bytes header + 512 bytes data
#define TIMEOUT_SECS 5  // Timeout in seconds

// TFTP Opcodes
#define OP_RRQ 1
#define OP_WRQ 2
#define OP_DATA 3
#define OP_ACK 4
#define OP_ERROR 5

void create_request_packet(char *buffer, const char *file_name, const char *mode, int opcode)
{
    int pos = 0;
    // Opcode for RRQ or WRQ
    buffer[pos++] = 0;
    buffer[pos++] = opcode;
    // Filename
    strcpy(&buffer[pos], file_name);
    pos += strlen(file_name) + 1;
    // Mode
    strcpy(&buffer[pos], mode);
    pos += strlen(mode) + 1;
}

void print_error_and_exit(char *error_msg)
{
    perror(error_msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int sock_fd;
    struct sockaddr_in serv_addr;
    char buf[BUF_SIZE];
    char *server_ip;
    char *file_name;
    char *operation;

    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <Server IP> <get/put> <Filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    server_ip = argv[1];
    operation = argv[2];
    file_name = argv[3];

    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        print_error_and_exit("socket() failed");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (strcmp(operation, "get") == 0)
    {
        create_request_packet(buf, file_name, "octet", OP_RRQ);
    }
    else if (strcmp(operation, "put") == 0)
    {
        create_request_packet(buf, file_name, "octet", OP_WRQ);
    }
    else
    {
        fprintf(stderr, "Invalid operation. Must be 'get' or 'put'\n");
        exit(EXIT_FAILURE);
    }

    if (sendto(sock_fd, buf, strlen(file_name) + strlen("octet") + 4, 0,
               (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        print_error_and_exit("sendto() failed");
    }

    struct sockaddr_in from_addr;
    unsigned int from_addr_len = sizeof(from_addr);
    int recv_msg_size;
    unsigned short block_num = 1;
    FILE *file_ptr;

    if (strcmp(operation, "get") == 0)
    {
        file_ptr = fopen(file_name, "wb");
        if (!file_ptr)
        {
            print_error_and_exit("Failed to open file for writing");
        }

        while (1)
        {
            if ((recv_msg_size = recvfrom(sock_fd, buf, BUF_SIZE, 0,
                                          (struct sockaddr *)&from_addr, &from_addr_len)) < 0)
            {
                print_error_and_exit("recvfrom() failed");
            }

            // Check for DATA opcode
            if (buf[1] == OP_DATA)
            {
                unsigned short recv_block = (unsigned char)buf[2] << 8 | (unsigned char)buf[3];
                if (recv_block == block_num)
                {
                    int data_len = recv_msg_size - 4; // Subtract the 4-byte header
                    fwrite(buf + 4, sizeof(char), data_len, file_ptr);

                    // Send ACK
                    buf[1] = OP_ACK;
                    if (sendto(sock_fd, buf, 4, 0, (struct sockaddr *)&from_addr, from_addr_len) < 0)
                    {
                        print_error_and_exit("sendto() failed on ACK");
                    }

                    block_num++;
                    if (data_len < 512)
                    { // Last packet
                        printf("File transfer complete.\n");
                        break;
                    }
                }
            }
            else if (buf[1] == OP_ERROR)
            {
                fprintf(stderr, "Received ERROR packet: %s\n", buf + 4); // Print error message
                exit(EXIT_FAILURE);                                      // Exit program
            }
            else
            {
                fprintf(stderr, "Received unexpected packet.\n");
                break;
            }
        }

        fclose(file_ptr);
    }
    else if (strcmp(operation, "put") == 0)
    {
        file_ptr = fopen(file_name, "rb");
        if (!file_ptr)
        {
            print_error_and_exit("Failed to open file for reading");
        }

        if ((recv_msg_size = recvfrom(sock_fd, buf, BUF_SIZE, 0,
                                      (struct sockaddr *)&from_addr, &from_addr_len)) < 0)
        {
            print_error_and_exit("recvfrom() failed");
        }
        printf("Giving data blocks\n");
        block_num = 1;
        while (1)
        {
            // Read data from file
            int data_len = fread(buf + 4, sizeof(char), 512, file_ptr);
            if (data_len <= 0)
            {
                // End of file reached
                break;
            }

            // Construct DATA packet
            buf[0] = 0;
            buf[1] = OP_DATA;
            buf[2] = (block_num >> 8) & 0xFF; // Block number high byte
            buf[3] = block_num & 0xFF;        // Block number low byte

            // Send DATA packet
            if (sendto(sock_fd, buf, data_len + 4, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
            {
                print_error_and_exit("sendto() failed on DATA");
            }

            // Receive ACK
            if ((recv_msg_size = recvfrom(sock_fd, buf, BUF_SIZE, 0,
                                          (struct sockaddr *)&from_addr, &from_addr_len)) < 0)
            {
                print_error_and_exit("recvfrom() failed");
            }

            // Check ACK
            if (buf[1] == OP_ACK)
            {
                unsigned short recv_block = (unsigned char)buf[2] << 8 | (unsigned char)buf[3];
                if (recv_block == block_num)
                {
                    block_num++;
                    if (data_len < 512)
                    {
                        printf("File transfer complete.\n");
                        break;
                    }
                }
            }
            else
            {
                fprintf(stderr, "Received unexpected packet.\n");
                break;
            }
        }

        fclose(file_ptr);
    }

    close(sock_fd);

    return 0;
}