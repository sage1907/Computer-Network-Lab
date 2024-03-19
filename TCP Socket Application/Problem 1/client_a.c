#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

void start_client(int port)
{
    int client_socket;
    struct sockaddr_in server_address;

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        perror("Error creating client socket");
        exit(EXIT_FAILURE);
    }

    // Prepare the server address struct
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("192.168.29.68");
    server_address.sin_port = htons(port);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("Error connecting to the server");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    char message[] = "Hello World!!";
    // Send the message to the server
    send(client_socket, message, strlen(message), 0);
    printf("Sent message to server: %s\n", message);

    char buffer[1024];
    // Receive the message from the server
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received == -1)
    {
        perror("Error receiving data");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Null-terminate the received data
    buffer[bytes_received] = '\0';
    printf("Received message from server: %s\n", buffer);

    // Close the connection
    close(client_socket);
}

int main()
{
    int server_port;
    printf("Enter the server port to connect to: ");
    scanf("%d", &server_port);
    start_client(server_port);

    return 0;
}
