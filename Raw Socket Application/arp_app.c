#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>

#define ETHERTYPE_ARP 0x0806
#define ARP_REQUEST 1
#define ARP_REPLY 2
#define ARP_HW_TYPE_ETH 1
#define ARP_PROTOCOL_TYPE_IP 0x0800
#define ARP_HW_SIZE_ETH 6
#define ARP_PROTOCOL_SIZE_IP 4

struct arp_packet {
    unsigned short hardware_type;
    unsigned short protocol_type;
    unsigned char hardware_size;
    unsigned char protocol_size;
    unsigned short opcode;
    unsigned char sender_mac[6];
    unsigned char sender_ip[4];
    unsigned char target_mac[6];
    unsigned char target_ip[4];
};

void send_arp_request(int sock, struct sockaddr_ll *sockaddr, unsigned char *src_mac, unsigned char *src_ip, unsigned char *dst_ip) {
    struct arp_packet packet;

    // Filling ARP packet fields
    packet.hardware_type = htons(ARP_HW_TYPE_ETH);
    packet.protocol_type = htons(ARP_PROTOCOL_TYPE_IP);
    packet.hardware_size = ARP_HW_SIZE_ETH;
    packet.protocol_size = ARP_PROTOCOL_SIZE_IP;
    packet.opcode = htons(ARP_REQUEST);
    memcpy(packet.sender_mac, src_mac, 6);
    memcpy(packet.sender_ip, src_ip, 4);
    memset(packet.target_mac, 0, 6); // Not known yet
    memcpy(packet.target_ip, dst_ip, 4);

    // Filling Ethernet header
    struct ether_header ether_header;
    memcpy(ether_header.ether_shost, src_mac, 6);
    memset(ether_header.ether_dhost, 0xff, 6); // Broadcast MAC address
    ether_header.ether_type = htons(ETHERTYPE_ARP);

    // Constructing packet buffer
    unsigned char packet_buffer[sizeof(struct ether_header) + sizeof(struct arp_packet)];
    memcpy(packet_buffer, &ether_header, sizeof(struct ether_header));
    memcpy(packet_buffer + sizeof(struct ether_header), &packet, sizeof(struct arp_packet));

    // Sending the ARP request
    sendto(sock, packet_buffer, sizeof(packet_buffer), 0, (struct sockaddr *)sockaddr, sizeof(struct sockaddr_ll));
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <interface> <target IP>\n", argv[0]);
        return 1;
    }

    // Create a raw socket
    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock == -1) {
        perror("Socket creation failed");
        return 1;
    }

    // Get the interface index
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, argv[1], IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) == -1) {
        perror("Failed to get interface index");
        close(sock);
        return 1;
    }

    // Prepare sockaddr_ll structure
    struct sockaddr_ll sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sll_family = AF_PACKET;
    sockaddr.sll_protocol = htons(ETH_P_ALL);
    sockaddr.sll_ifindex = ifr.ifr_ifindex;

    // Get source MAC address
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) == -1) {
        perror("Failed to get MAC address");
        close(sock);
        return 1;
    }
    unsigned char src_mac[6];
    memcpy(src_mac, ifr.ifr_hwaddr.sa_data, 6);

    // Get source IP address
    if (ioctl(sock, SIOCGIFADDR, &ifr) == -1) {
        perror("Failed to get IP address");
        close(sock);
        return 1;
    }
    unsigned char src_ip[4];
    struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
    memcpy(src_ip, &sin->sin_addr, 4);

    // Convert target IP address from string to binary
    unsigned char dst_ip[4];
    if (inet_pton(AF_INET, argv[2], dst_ip) != 1) {
        printf("Invalid target IP address\n");
        close(sock);
        return 1;
    }

    // Send ARP request
    send_arp_request(sock, &sockaddr, src_mac, src_ip, dst_ip);
    printf("ARP request sent for %s\n", argv[2]);

    // Receive ARP response (assuming this is a blocking call)
    struct arp_packet response_packet;
    recv(sock, &response_packet, sizeof(struct arp_packet), 0);

    // Check if it's an ARP reply
    if (ntohs(response_packet.opcode) == ARP_REPLY) {
        printf("Received ARP response\n");
        printf("MAC address of %s: %02x:%02x:%02x:%02x:%02x:%02x\n",
               argv[2],
               response_packet.sender_mac[0], response_packet.sender_mac[1], response_packet.sender_mac[2],
               response_packet.sender_mac[3], response_packet.sender_mac[4], response_packet.sender_mac[5]);
    } else {
        printf("Received packet is not an ARP response\n");
    }

    close(sock);
    return 0;
}
