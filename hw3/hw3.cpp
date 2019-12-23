#include <bits/stdc++.h>
#include <pcap.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;

#define SNAP_LEN 1518     //設定一個封包最多捕獲1518 bytes
#define SIZE_ETHERNET 14  //header 必定14 bytes
#define ETHER_ADDR_LEN 6  //address 必定6 bytes
#define IP_RF 0x8000      //reserved fragment flag
#define IP_DF 0x4000      //dont fragment flag
#define IP_MF 0x2000      //more fragments flag
#define IP_OFFMASK 0x1fff //mask for fragmenting bits
#define IP_HL(ip) (((ip)->ip_vhl) & 0x0f)
#define IP_V(ip) (((ip)->ip_vhl) >> 4)
#define TH_OFF(th) (((th)->th_offx2 & 0xf0) >> 4)
#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PUSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
#define TH_ECE 0x40
#define TH_CWR 0x80
#define TH_FLAGS (TH_FIN | TH_SYN | TH_RST | TH_ACK | TH_URG | TH_ECE | TH_CWR)

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

//UDP header
typedef struct uheader
{
    uint16_t uh_sport; //source port
    uint16_t uh_dport; //destination port
    uint16_t uh_length;
    uint16_t uh_sum; //checksum
} UDP;
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
    static int count = 1; //封包計數
    const IP *ip;         //The IP header
    const TCP *tcp;       //The TCP header
    const UDP *udp;       //The UDP header
    struct tm *lt;        //時間函式
    char timestr[80];
    time_t local_tv_sec;
    int size_ip, size_tcp;

    local_tv_sec = header->ts.tv_sec;
    lt = localtime(&local_tv_sec);
    strftime(timestr, sizeof(timestr), "%b %d %Y, %X", lt);

    ip = (IP *)(packet + SIZE_ETHERNET);
    size_ip = IP_HL(ip) * 4; //IP header長度
    if (size_ip < 20)
        return;
    printf("\n=========================================\nPacket number %d\n\n", count);
    count++;
    int len = header->len;
    if (ip->ip_p == IPPROTO_TCP) // 計算TCP header offset
    {
        printf("\tProtocol Type: TCP\n");
        tcp = (TCP *)(packet + SIZE_ETHERNET + size_ip);
        printf("\tSource port: %d\n", ntohs(tcp->th_sport));
        printf("\tDestination port: %d\n", ntohs(tcp->th_dport));
    }
    else if (ip->ip_p == IPPROTO_UDP) //計算UDP header offset
    {
        printf("\tProtocol Type: UDP\n");
        udp = (UDP *)(packet + SIZE_ETHERNET + size_ip);
        printf("\tSource port: %d\n", ntohs(udp->uh_sport));
        printf("\tDestination port: %d\n", ntohs(udp->uh_dport));
        len = ntohs(udp->uh_length);
    }
    else if (ip->ip_p == IPPROTO_ICMP)
        printf("\tProtocol Type: ICMP\n");
    else if (ip->ip_p == IPPROTO_IP)
        printf("\tProtocol Type: IP\n");
    else
        printf("\tProtocol Type: Unknown\n");
    printf("\tSource IP: %s\n", inet_ntoa(ip->ip_src));
    printf("\tDestination IP: %s\n", inet_ntoa(ip->ip_dst));
    printf("\tTime: %s\n", timestr);
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
    const char *filter = " "; //過濾
    struct bpf_program fp;    //compiled filter program (expression)
    bpf_u_int32 mask, net;
    int num_packets = -1; //獲取封包數量

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
    printf("\nComplete...\n\n");
    map<string, int>::iterator ptr;
    ptr = Packet.begin();
    printf("Packet Counts List:\n");
    printf("===============================================================================\n");
    printf("Source IP\t\t|\tDestination IP\t\t|\tPacket Counts\n");
    printf("-------------------------------------------------------------------------------\n");
    for (ptr = Packet.begin(); ptr != Packet.end(); ptr++)
        printf("%s\t\t|\t%d\n", ptr->first.c_str(), ptr->second);
    return 0;
}