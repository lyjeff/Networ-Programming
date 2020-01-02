#include <bits/stdc++.h>
#include <pcap.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;

#define SNAP_LEN 1518    //設定一個封包最多捕獲1518 bytes
#define SIZE_ETHERNET 14 //header 必定14 bytes
#define ETHER_ADDR_LEN 6 //address 必定6 bytes
#define IP_HL(ip) (((ip)->ip_vhl) & 0x0f)

//TCP header
typedef u_int tcp_seq;

typedef struct theader
{
    u_short th_sport; //source port
    u_short th_dport; //destination port
    tcp_seq th_seq;   //sequence number
    tcp_seq th_ack;   //acknowledgement number
    u_char th_offx2;  //data offset, rsvd
    u_char th_flags;
    u_short th_win; //window
    u_short th_sum; //checksum
    u_short th_urp; //urgent pointer
} TCP;

//IP header
typedef struct iheader
{
    u_char ip_vhl;                 //version << 4 | header length >> 2
    u_char ip_tos;                 //type of service
    u_short ip_len;                //total length
    u_short ip_id;                 //identification
    u_short ip_off;                //fragment offset field
    u_char ip_ttl;                 //time to live
    u_char ip_p;                   //protocol
    u_short ip_sum;                //checksum
    struct in_addr ip_src, ip_dst; //source and dest address
} IP;

//Ethernet header
typedef struct eheader
{
    u_char ether_dhost[ETHER_ADDR_LEN]; //destination host address
    u_char ether_shost[ETHER_ADDR_LEN]; //source host address
    u_short ether_type;                 //IP? ARP? RARP? etc
} Ethernet;

map<string, int> Packet;

void callback(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
    static int count = 1;     //封包計數
    const IP *ip;             //The IP header
    const TCP *tcp;           //The TCP header
    const Ethernet *ethernet; // The Ethernet header
    struct tm *lt;            //時間函式
    char timestr[80];
    time_t local_tv_sec;
    int size_ip;
    printf("\n=========================================\nPacket number %d\n\n", count);
    count++;

    // get time
    local_tv_sec = header->ts.tv_sec;
    lt = localtime(&local_tv_sec);
    strftime(timestr, sizeof(timestr), "%b %d %Y, %X", lt);
    printf("\tTime: %s\n", timestr);

    ethernet = (Ethernet *)packet;
    ip = (IP *)(packet + SIZE_ETHERNET);
    size_ip = IP_HL(ip) * 4; // IP header長度
    tcp = (TCP *)(packet + SIZE_ETHERNET + size_ip);

    // get MAC source address
    u_char *mac_string = (unsigned char *)ethernet->ether_shost;
    printf("\tMac Source Address: %02x:%02x:%02x:%02x:%02x:%02x\n", *(mac_string + 0), *(mac_string + 1), *(mac_string + 2), *(mac_string + 3), *(mac_string + 4), *(mac_string + 5));

    // get MAC destination address
    mac_string = (unsigned char *)ethernet->ether_dhost;
    printf("\tMac Destination Address: %02x:%02x:%02x:%02x:%02x:%02x\n", *(mac_string + 0), *(mac_string + 1), *(mac_string + 2), *(mac_string + 3), *(mac_string + 4), *(mac_string + 5));

    if (ethernet->ether_type != 8)
    {
        printf("\tProtocol Type: Not IP\n\n");
        return;
    }

    int len = header->len;
    if (ip->ip_p == IPPROTO_TCP) // 計算TCP header offset
        printf("\tProtocol Type: TCP\n");
    else if (ip->ip_p == IPPROTO_UDP) //計算UDP header offset
        printf("\tProtocol Type: UDP\n");
    else if (ip->ip_p == IPPROTO_ICMP)
        printf("\tProtocol Type: ICMP\n");
    else if (ip->ip_p == IPPROTO_IP)
        printf("\tProtocol Type: IP\n");
    else if (ip->ip_p == 89)
        printf("\tProtocol Type: OSPF\n");
    else if (ip->ip_p == 91)
        printf("\tProtocol Type: LARP\n");
    else if (ip->ip_p == 54)
        printf("\tProtocol Type: NARP\n");
    else
        printf("\tProtocol Type: IP\n");
    printf("\tSource IP: %s:%d\n", inet_ntoa(ip->ip_src), ntohs(tcp->th_sport));
    printf("\tDestination IP: %s:%d\n", inet_ntoa(ip->ip_dst), ntohs(tcp->th_dport));

    printf("\tPacket Length: %d bytes\n", len);

    char tmp[100];
    memset(tmp, 0, 100);
    sprintf(tmp, "%s\t\t|\t%s", inet_ntoa(ip->ip_src), inet_ntoa(ip->ip_dst));
    std::string path(tmp);
    if (Packet.find(path) == Packet.end())
        Packet[path] = 1;
    else
        Packet[path]++;
}

int main(int argc, char *argv[])
{
    char *dev = NULL, errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle;
    struct bpf_program fp; //compiled filter program (expression)
    int num_packets = -1;  //獲取封包數量

    if (argc > 2)
    {
        if (strcmp(argv[1], "-r") == 0)
            handle = pcap_open_offline(argv[2], errbuf);
    }
    else
    {
        dev = pcap_lookupdev(errbuf);
        printf("Device: %s\n", dev);

        //打開網絡接口
        handle = pcap_open_live(dev, SNAP_LEN, 1, 1000, errbuf);
    }
    pcap_loop(handle, num_packets, callback, NULL);
    pcap_freecode(&fp);
    pcap_close(handle);
    printf("\n=============\nComplete...\n=============\n\n");
    map<string, int>::iterator ptr;
    ptr = Packet.begin();
    if (ptr == Packet.end())
    {
        printf("No IP Packet...\n");
        return 0;
    }
    printf("Packet Counts List:\n");
    printf("===============================================================================\n");
    printf("Source IP\t\t|\tDestination IP\t\t|\tPacket Counts\n");
    printf("-------------------------------------------------------------------------------\n");
    for (ptr = Packet.begin(); ptr != Packet.end(); ptr++)
        printf("%s\t\t|\t%d\n", ptr->first.c_str(), ptr->second);
    return 0;
}