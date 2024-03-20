#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t locker = PTHREAD_MUTEX_INITIALIZER;

char iface[30];
char sender_ip[30];
char receiver_ip[30];
struct timespec timer;

unsigned char* buffer = NULL;

struct ethernet_header {
    unsigned char dest_mac[ETH_ALEN];
    unsigned char source_mac[ETH_ALEN];
    __be16 protocol;
};

void print_ethernet_header(void *buffer);
void* timer_function(void* ptr);
void* arp_packet_processor(void* ptr);
bool validate_arp_packet(unsigned char* packet);
void* wait_for_arp_response(void* ptr);
void fill_arp_packet(unsigned char* buffer, struct ifreq* ifreq_c);


int main(int argc, char* argv[]) {
    strcpy(iface, argv[1]);
    printf("%s\n", iface);
    strcpy(sender_ip, argv[2]);
    strcpy(receiver_ip, argv[3]);
    timer.tv_sec = atoll(argv[4]) / 1000000000;
    timer.tv_nsec = atoll(argv[4]) % 1000000000;
    int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd < 0) {
        perror("Socket failed to open :");
        exit(1);
    }

    unsigned char packet[64];
    memset(packet, 64, 0);
    struct sockaddr sock_address;
    socklen_t size = sizeof(sock_address);

    /* Index of the iface */
    struct ifreq ifreq_i;
    memset(&ifreq_i, 0, sizeof(ifreq_i));
    strncpy(ifreq_i.ifr_name, iface, IFNAMSIZ - 1);
    if ((ioctl(fd, SIOCGIFINDEX, &ifreq_i)) < 0) {
        perror("Error in index \n");
        exit(1);
    }

    /* Mac Address of the iface */
    struct ifreq ifreq_c;
    memset(&ifreq_c, 0, sizeof(ifreq_c));
    strncpy(ifreq_c.ifr_name, iface, IFNAMSIZ - 1);
    if ((ioctl(fd, SIOCGIFHWADDR, &ifreq_c)) < 0) {
        perror("Error in mac \n");
        exit(1);
    }

    struct ethernet_header *eth = (struct ethernet_header *)(packet);

    /* Filling Source mac address of the specified iface */
    memcpy(eth->source_mac, ifreq_c.ifr_hwaddr.sa_data, ETH_ALEN);

    /* Filling destination mac address as Broadcast address 255.255.255.255.255.255 */
    memset(eth->dest_mac, 255, ETH_ALEN);

    /* Filling the protocol Type... ARP */
    unsigned short protocol_id = 2054;
    eth->protocol = htons(protocol_id); // 0X0806 for ARP
    print_ethernet_header((void*)packet);

    /* Filling ARP request packet */
    fill_arp_packet(packet, &ifreq_c);

    /* Sending ARP Packet to the Physical Layer */
    struct sockaddr_ll sadr_ll;
    sadr_ll.sll_ifindex = ifreq_i.ifr_ifindex;  // index of iface
    sadr_ll.sll_halen = ETH_ALEN;  // length of destination mac address
    memset(sadr_ll.sll_addr, 255, ETH_ALEN);
    int send_len = sendto(fd, packet, 64, 0, (const struct sockaddr*)&sadr_ll, sizeof(sadr_ll));
    if (send_len < 0) {
        perror("Sending Error ");
        exit(1);
    }

    pthread_t timer, receiver, processor;
    pthread_create(&processor, NULL, &arp_packet_processor, NULL);
    pthread_create(&receiver, NULL, &wait_for_arp_response, (void*)&fd);
    pthread_create(&timer, NULL, &timer_function, NULL);
    pthread_join(timer, NULL);
    pthread_cancel(receiver);
    close(fd);
    return 0;
}

void print_ethernet_header(void *buffer) {
    struct ethernet_header* eth = (struct ethernet_header*)buffer;
    printf("Source MAC Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",
           (unsigned)eth->source_mac[0], (unsigned)eth->source_mac[1],
           (unsigned)eth->source_mac[2], (unsigned)eth->source_mac[3],
           (unsigned)eth->source_mac[4], (unsigned)eth->source_mac[5]);
    printf("\n");
}

void print_ethernet_header_received_packet(void *buffer) {
    struct ethernet_header* eth = (struct ethernet_header*)buffer;
    printf("Destination MAC Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",
           (unsigned)eth->source_mac[0], (unsigned)eth->source_mac[1],
           (unsigned)eth->source_mac[2], (unsigned)eth->source_mac[3],
           (unsigned)eth->source_mac[4], (unsigned)eth->source_mac[5]);
    printf("\n");
}


void* timer_function(void* ptr) {
    nanosleep(&timer, NULL);
    pthread_cond_signal(&cond);
}

void* arp_packet_processor(void* ptr) {
    pthread_mutex_lock(&locker);
    pthread_cond_wait(&cond, &locker);
    if (buffer == NULL) {
        printf("No ARP response received.\nTIMED OUT.\n");
        return NULL;
    }
    print_ethernet_header_received_packet(buffer);
    free(buffer);
    return NULL;
}

bool validate_arp_packet(unsigned char* packet) {
    unsigned int arp_protocol = 2054;
    struct ethernet_header* eth = (struct ethernet_header*)packet;
    if (ntohs(eth->protocol) != arp_protocol) {
        memset(packet, 0, 1024);
        return false;
    }

    int pos = sizeof(struct ethernet_header) + 6;
    int operation = 0;
    int y = packet[pos];
    operation = y;
    operation = operation << 8;
    pos++;
    y = packet[pos];
    operation = operation | y;
    if (operation != 2) {
        return false;
    }

    pos = sizeof(struct ethernet_header) + 14;
    char received_ip[16] = {0};
    sprintf(received_ip, "%d.%d.%d.%d", packet[pos], packet[pos + 1], packet[pos + 2], packet[pos + 3]);
    if (strcmp(received_ip, receiver_ip) != 0) {
        return false;
    }
    return true;
}

void* wait_for_arp_response(void* ptr) {
    int fd = *((int*)ptr);
    unsigned char packet[1024];
    memset(packet, 0, 1024);
    unsigned int arp_protocol = 2054;
    struct sockaddr sock_address;
    socklen_t size = sizeof(sock_address);

    while (1) {
        int recv_size = recvfrom(fd, packet, sizeof(packet), 0, &sock_address, &size);
        if (recv_size < 0) {
            perror("Receive failed");
            exit(1);
        }
        if (validate_arp_packet(packet) == false) continue;
        buffer = (unsigned char*)malloc(1024);
        memcpy(buffer, packet, 1024);
        break;
    }
    return NULL;
}

void fill_arp_packet(unsigned char* buffer, struct ifreq* ifreq_c) {
    int pos = sizeof(struct ethernet_header);
    buffer[pos++] = 0;
    buffer[pos++] = 1;
    unsigned short opcode = 2048;
    buffer[pos++] = (opcode >> 8);
    buffer[pos++] = opcode;
    buffer[pos++] = 6;
    buffer[pos++] = 4;
    buffer[pos++] = 0;
    buffer[pos++] = 1;
    buffer[pos++] = (ifreq_c->ifr_hwaddr).sa_data[0];
    buffer[pos++] = (ifreq_c->ifr_hwaddr).sa_data[1];
    buffer[pos++] = (ifreq_c->ifr_hwaddr).sa_data[2];
    buffer[pos++] = (ifreq_c->ifr_hwaddr).sa_data[3];
    buffer[pos++] = (ifreq_c->ifr_hwaddr).sa_data[4];
    buffer[pos++] = (ifreq_c->ifr_hwaddr).sa_data[5];
    in_addr_t ip = inet_addr(sender_ip);
    buffer[pos++] = ip;
    buffer[pos++] = (ip >> 8);
    buffer[pos++] = (ip >> 16);
    buffer[pos++] = (ip >> 24);
    buffer[pos++] = 0;
    buffer[pos++] = 0;
    buffer[pos++] = 0;
    buffer[pos++] = 0;
    buffer[pos++] = 0;
    buffer[pos++] = 0;
    ip = inet_addr(receiver_ip);
    buffer[pos++] = ip;
    buffer[pos++] = (ip >> 8);
    buffer[pos++] = (ip >> 16);
    buffer[pos++] = (ip >> 24);
}