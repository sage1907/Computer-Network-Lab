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

        // Check if the server wants to end the chat
        if (strcmp(client_message, "Bye") == 0) {
            printf("Chat session ended by the server.\n");
            break;
        }

        // Print received message from server
	if (bytes_received > 0) {
        	printf("Server : %s\n", client_message);
		//printf("Client : ");
		fflush(stdout);
	}

    }

    // Close the client socket
    close(thread_data->client_socket);
    free(data);

    return NULL;
}


void send_messages(int client_socket) {
    char client_message[1024];

    while (1) {
        // Get the client's message
	//printf("Client : ");
        fgets(client_message, sizeof(client_message), stdin);

        send(client_socket, client_message, strlen(client_message), 0);

        // Check if the client wants to end the chat
        if (strcmp(client_message, "Bye\n") == 0) {
            printf("Chat session ended by the client.\n");
            exit(0);
        }
    }
}


void start_client(const char *server_ip, int port) {
    int client_socket;
    struct sockaddr_in server_address;

    
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error creating client socket");
        exit(EXIT_FAILURE);
    }

    
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(server_ip);
    server_address.sin_port = htons(port);

   
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Error connecting to the server");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    
    pthread_t receive_thread;
    struct ThreadData *thread_data = (struct ThreadData *)malloc(sizeof(struct ThreadData));
    if (thread_data == NULL) {
        perror("Error creating thread data");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    thread_data->client_socket = client_socket;
    if (pthread_create(&receive_thread, NULL, receive_messages, (void *)thread_data) != 0) {
        perror("Error creating receive thread");
        free(thread_data);
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    
    send_messages(client_socket);

    
    pthread_join(receive_thread, NULL);

    
    close(client_socket);
}

int main() {
    int server_port;
    char server_ip[16];
    printf("Enter the server IP address to connect to: ");
    scanf("%15s", server_ip);
    printf("Enter the server port to connect to: ");
    scanf("%d", &server_port);
    start_client(server_ip, server_port);

    return 0;
}