#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

void start_server(int port) {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len = sizeof(client_address);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }

    // Prepare the server address struct
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    // Bind the socket
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Error binding server socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_socket, 1) == -1) {
        perror("Error listening for connections");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);

    // Accept a connection
    client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_len);
    if (client_socket == -1) {
        perror("Error accepting connection");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Connection established from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

    char client_message[1024];
    char server_message[1024];

    while (1) {
        // Receive message from the client
        ssize_t bytes_received = recv(client_socket, client_message, sizeof(client_message), 0);
        if (bytes_received <= 0) {
            perror("Error receiving data");
            break;
        }

        // Null-terminate the received data
        client_message[bytes_received] = '\0';

        printf("Received message from client: %s\n", client_message);

        // Check if the client wants to end the chat
        if (strcmp(client_message, "Bye") == 0) {
            printf("Chat session ended by the client.\n");
            break;
        }

        // Get the server's response
        printf("Enter your response: ");
        fgets(server_message, sizeof(server_message), stdin);

        // Send the response to the client
        send(client_socket, server_message, strlen(server_message), 0);

        // Check if the server wants to end the chat
        if (strcmp(server_message, "Bye\n") == 0) {
            printf("Chat session ended by the server.\n");
            break;
        }
    }

    // Close the connection
    close(client_socket);

    // Close the server socket
    close(server_socket);
}

int main() {
    int server_port;
    printf("Enter the server port: ");
    scanf("%d", &server_port);
    start_server(server_port);

    return 0;
}
