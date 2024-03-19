#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define MAXDATASIZE 1000
#define MAXDATAGRAMS 50
#define ROUNDS_PER_PAYLOAD 10

// Function to calculate time difference
double time_diff(struct timeval x, struct timeval y);

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <server IP> <server port> <TTL> <output file>\n", argv[0]);
        exit(1);
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int ttl = atoi(argv[3]);
    char *output_file = argv[4];

    // Initialize socket, bind, etc. (Similar to previous code)
    int sockfd;
    struct sockaddr_in server_addr;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error in socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_aton(server_ip, &server_addr.sin_addr);

    // Open output file
    FILE *output_file_ptr = fopen(output_file, "w");
    if (output_file_ptr == NULL) {
        perror("Error opening file");
        exit(1);
    }

    // Run for each payload size from 100 to 1000
    for (int payload = 100; payload <= 1000; payload += 100) {
        printf("Payload: %d\n", payload);
        double total_rtt = 0.0;

        // Run for each round
        for (int round = 0; round < ROUNDS_PER_PAYLOAD; round++) {
	    /* struct timeval start_time;
            gettimeofday(&start_time, NULL);
            // Send the first datagram to the server*/
            char first_datagram[MAXDATASIZE];
	    /*
            ssize_t num_bytes_sent = sendto(sockfd, first_datagram, strlen(first_datagram), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
            if (num_bytes_sent < 0) {
                perror("Error sending first datagram");
                exit(1);
            }*/

            // Receive the first datagram from the server
            char recv_buffer[MAXDATASIZE];
            /*ssize_t num_bytes_received = recvfrom(sockfd, recv_buffer, MAXDATASIZE, 0, NULL, NULL);
            if (num_bytes_received < 0) {
                perror("Error receiving first datagram");
                exit(1);
            }
 		*/
            int num_datagrams = 0; // Already sent and received one datagram
            // Receive and process datagrams
            while (num_datagrams < MAXDATAGRAMS) {
		 struct timeval start_time;
           	 gettimeofday(&start_time, NULL);
                // Send datagram to server
                ssize_t num_bytes_sent = sendto(sockfd, recv_buffer, strlen(first_datagram), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                if (num_bytes_sent < 0) {
                    perror("Error sending datagram");
                    exit(1);
                }

                // Receive datagram from server
                ssize_t num_bytes_received = recvfrom(sockfd, recv_buffer, MAXDATASIZE, 0, NULL, NULL);
                if (num_bytes_received < 0) {
                    perror("Error receiving datagram");
                    exit(1);
                }

                // Decrement TTL - this should be handled in the server side

                /*// Check if payload size matches the expected size
                if (num_bytes_received != payload) {
                    fprintf(stderr, "Error: Received payload size does not match expected size\n");
                    exit(1);
                }*/

                // Calculate Cumulative RTT
                struct timeval end_time;
                gettimeofday(&end_time, NULL);
                double rtt = time_diff(start_time, end_time);
                total_rtt += rtt;

                // Increment number of datagrams
                num_datagrams++;
            }
        }

        // Calculate average RTT for 10 rounds
        double avg_rtt = total_rtt / ROUNDS_PER_PAYLOAD;
        printf("Average RTT for payload %d: %lf\n", payload, avg_rtt);

        // Save the average RTT to the output file
        fprintf(output_file_ptr, "%d,%lf\n", payload, avg_rtt);
    }

    // Close output file
    fclose(output_file_ptr);

    // Close socket
    close(sockfd);

    return 0;
}

// Function to calculate time difference in milliseconds
double time_diff(struct timeval x, struct timeval y) {
    double x_ms, y_ms, diff;
    x_ms = (double)x.tv_sec * 1000000 + (double)x.tv_usec;
    y_ms = (double)y.tv_sec * 1000000 + (double)y.tv_usec;
    diff = (double)y_ms - (double)x_ms;
    return diff / 1000; // Convert to milliseconds
}