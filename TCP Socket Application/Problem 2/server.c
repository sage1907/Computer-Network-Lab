#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

struct ThreadData {
    int client_socket;
};


void *receive_messages(void *data) {
    struct ThreadData *thread_data = (struct ThreadData *)data;
    char client_message[1024];

    while (1) {
        ssize_t bytes_received = recv(thread_data->client_socket, client_message, sizeof(client_message), 0);

        if (bytes_received <= 0) {
            //perror("Error receiving data");
            break;
        }

        client_message[bytes_received] = '\0';

        if (strcmp(client_message, "Bye") == 0) {
            printf("Chat session ended by the client.\n");
            break;
        }

	if (bytes_received > 0) {
		printf("Client : %s\n", client_message);
		//printf("Server : ");
		fflush(stdout);
	}

    }

    close(thread_data->client_socket);
    free(data);

    return NULL;
}

void send_messages(int client_socket) {
    char server_message[1024];

    while (1) {
        //printf("Server : ");
        fgets(server_message, sizeof(server_message), stdin);

        send(client_socket, server_message, strlen(server_message), 0);

        if (strcmp(server_message, "Bye\n") == 0) {
            printf("Chat session ended by the server.\n");
            exit(0);
        }
    }
}

void start_server(int port) {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len = sizeof(client_address);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Error binding server socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 1) == -1) {
        perror("Error listening for connections");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);

    client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
    if (client_socket == -1) {
        perror("Error accepting connection");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Connection established from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

    pthread_t receive_thread;
    struct ThreadData *thread_data = (struct ThreadData *)malloc(sizeof(struct ThreadData));
    if (thread_data == NULL) {
        perror("Error creating thread data");
        close(client_socket);
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    thread_data->client_socket = client_socket;
    if (pthread_create(&receive_thread, NULL, receive_messages, (void *)thread_data) != 0) {
        perror("Error creating receive thread");
        free(thread_data);
        close(client_socket);
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    send_messages(client_socket);

    pthread_join(receive_thread, NULL);

    close(server_socket);
}

int main() {
    int server_port;
    printf("Enter the server port: ");
    scanf("%d", &server_port);
    start_server(server_port);

    return 0;
}