#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

void start_client(int port) {
    int client_socket;
    struct sockaddr_in server_address;

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error creating client socket");
        exit(EXIT_FAILURE);
    }

    // Prepare the server address struct
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("192.168.29.68");
    server_address.sin_port = htons(port);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Error connecting to the server");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    char client_message[1024];
    char server_message[1024];

    while (1) {
        // Get the client's message
        printf("Enter your message: ");
        fgets(client_message, sizeof(client_message), stdin);

        // Send the message to the server
        send(client_socket, client_message, strlen(client_message), 0);

        // Check if the client wants to end the chat
        if (strcmp(client_message, "Bye\n") == 0) {
            printf("Chat session ended by the client.\n");
            break;
        }

        // Receive the response from the server
        ssize_t bytes_received = recv(client_socket, server_message, sizeof(server_message), 0);
        if (bytes_received <= 0) {
            perror("Error receiving data");
            break;
        }

        // Null-terminate the received data
        server_message[bytes_received] = '\0';

        printf("Received message from server: %s\n", server_message);

        // Check if the server wants to end the chat
        if (strcmp(server_message, "Bye") == 0) {
            printf("Chat session ended by the server.\n");
            break;
        }
    }

    // Close the connection
    close(client_socket);
}

int main() {
    int server_port;
    printf("Enter the server port to connect to: ");
    scanf("%d", &server_port);
    start_client(server_port);

    return 0;
}
