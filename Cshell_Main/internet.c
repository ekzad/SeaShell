#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#include "colors.h"
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <ifaddrs.h>
    #include <netdb.h>
    #include <arpa/inet.h>
#endif

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} ICMPHeader;

static uint16_t checksum(void *data, int len) {
    uint16_t *buf = data;
    unsigned int sum = 0;
    uint16_t result;

    for (sum = 0; len > 1; len -= 2) {
        sum += *buf++;
    }
    if (len == 1) {
        sum += *(uint8_t *)buf;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}
extern sig_atomic_t interrupt_requested;
void ping(const char *arg) {
    if (arg == NULL || arg[0] == '\0') {
        printf(COLOR_BRIGHT_RED "[!] Invalid Argument\n" COLOR_RESET);
        printf("Usage: ping <host>\n");
        return;
    }

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf(COLOR_BRIGHT_RED "[!] WSAStartup failed\n" COLOR_RESET);
        return;
    }

    SOCKET sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock == INVALID_SOCKET) {
        printf(COLOR_BRIGHT_RED "[!] Raw socket failed (are you running as Administrator?)\n" COLOR_RESET);
        WSACleanup();
        return;
    }

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;

    // resolve hostname (or IP) to an actual address
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    if (getaddrinfo(arg, NULL, &hints, &res) != 0) {
        printf(COLOR_BRIGHT_RED "[!] Could not resolve host: %s\n" COLOR_RESET, arg);
        closesocket(sock);
        WSACleanup();
        return;
    }
    dest.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
    freeaddrinfo(res);

    // build the ICMP packet
    char packet[64];
    memset(packet, 0, sizeof(packet));
    ICMPHeader *icmp = (ICMPHeader *)packet;
    icmp->type = 8; // echo request
    icmp->code = 0;
    icmp->id = (uint16_t)GetCurrentProcessId();
    icmp->seq = 1;
    icmp->checksum = 0;
    icmp->checksum = checksum(packet, sizeof(packet));

    printf("Pinging %s...\n", arg);

    for (int i = 0; i < 4; i++) { // 4
        if (interrupt_requested) {
            printf("\nPing was interrupted.\n");
            interrupt_requested = 0;
            break;
        }
        icmp->seq = i + 1;
        icmp->checksum = 0;
        icmp->checksum = checksum(packet, sizeof(packet));

        clock_t start = clock();

        int sent = sendto(sock, packet, sizeof(packet), 0,
                           (struct sockaddr *)&dest, sizeof(dest));
        if (sent == SOCKET_ERROR) {
            printf(COLOR_BRIGHT_RED "[!] Send failed\n" COLOR_RESET);
            continue;
        }

        char recv_buf[128];
        struct sockaddr_in from;
        int from_len = sizeof(from);
        int received = recvfrom(sock, recv_buf, sizeof(recv_buf), 0,
                                 (struct sockaddr *)&from, &from_len);

        clock_t end = clock();
        double ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;

        if (received > 0) {
            printf("Reply from %s: time=%.2fms\n", arg, ms);
        } else {
            printf(COLOR_YELLOW "Request timed out\n" COLOR_RESET);
        }
    }

    closesocket(sock);
    WSACleanup();
#else
    printf(COLOR_BRIGHT_RED "[!] ping is currently only implemented for Windows\n" COLOR_RESET);
#endif
}

#ifdef _WIN32
void netfo(const char *arg) {
    (void)arg;
    // get mac
    // get ip
    // i havent really figured this out yet. its complex as fuck (i dont wanna resort to ips and etc)
    // for now let's go with the ipconfig in windows CMD
    system("ipconfig");
}
#else
void netfo(const char *arg) {
    (void)arg;
    printf(COLOR_BRIGHT_RED "Unable to get device info." COLOR_RESET);
}
#endif
#include <curl/curl.h>
void download(const char *arg) {
    if (arg==NULL || arg[0] == ' ' || arg[0] == '\0') {
        printf(COLOR_BRIGHT_RED "[!] Invalid Argument\n" COLOR_RESET);
        printf("Usage: grab <github link>\n");
        return;
    }
    char buf[1024];
    strncpy(buf, arg, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *url = strtok(buf, " ");
    char *filename = strtok(NULL, " ");
    if (url==NULL || filename==NULL) {
        printf(COLOR_BRIGHT_RED "Destination invalid. Try a different link\n" COLOR_RESET);
        return;
    }
    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        printf(COLOR_BRIGHT_RED "[!] Failed to init curl\n" COLOR_RESET);
        return;
    }             // the file
    FILE *fp = fopen(filename, "wb");
    if (fp ==NULL){
        printf(COLOR_BRIGHT_RED "[!] Couldn't create an output\n" COLOR_RESET);
        curl_easy_cleanup(curl);
        return;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // follow redirects
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        printf(COLOR_BRIGHT_RED "[!] Couldn't download: %s\n" COLOR_RESET, filename);
    }
    else {
        printf(COLOR_BRIGHT_GREEN "[>] Downloaded to %s\n" COLOR_RESET, filename);
    }
    fclose(fp); // close the file
    curl_easy_cleanup(curl); //clean up :)

}

void larp(const char *arg) {
    (void)arg;

    ULONG size = 0;
    GetIpNetTable(NULL, &size, FALSE); // first call just gets required size

    PMIB_IPNETTABLE table = (PMIB_IPNETTABLE)malloc(size);
    if (GetIpNetTable(table, &size, FALSE) != NO_ERROR) {
        printf("[!] Couldn't read ARP table\n");
        free(table);
        return;
    }

    printf("Devices seen on your network:\n");
    for (DWORD i = 0; i < table->dwNumEntries; i++) {
        MIB_IPNETROW row = table->table[i];

        struct in_addr addr;
        addr.S_un.S_addr = row.dwAddr;

        printf("  IP: %-15s  MAC: %02X-%02X-%02X-%02X-%02X-%02X\n",
            inet_ntoa(addr),
            row.bPhysAddr[0], row.bPhysAddr[1], row.bPhysAddr[2],
            row.bPhysAddr[3], row.bPhysAddr[4], row.bPhysAddr[5]);
    }

    free(table);
}

int tcp_response(const char *ip, unsigned short port, char *response, int response_size)
{
    if (ip == NULL || response == NULL || response_size <= 0) {
        return -1;
    }

    response[0] = '\0';  // init

    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (InetPtonA(AF_INET, ip, &server.sin_addr) != 1) {
        printf("ERR: Invalid IP address.\n");
        return -1;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        printf("ERR: WSAStartup Failed...\n");
        return -1;  // <-- ending up here means socket wasnt overwritten basically. WSA startup failed
    }

    // timeout time is 3s so that we arent left hanging forever with no termination
    DWORD timeout = 3000; // 3s
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
        closesocket(sock);
        return 0;
    }

    // connect succeeded => port is open. For RDP, no banner needed
    closesocket(sock);
    return 1; // 1=OPEN
}
// Port scanner

void checkports(const char *arg) {
    // arg IS the ip
    if (arg == NULL || arg[0] == '\0') {
        printf("[!] Invalid argument\n");
        printf("Usage: scan <ip_address>\n");
        return;
    }

    unsigned short common_ports[] = {
    7, 19, 20, 21, 22, 23, 25, 42, 43, 49, 53, 67, 68, 69, 70, 79, 80, 88,
    102, 110, 113, 119, 123, 135, 137, 138, 139, 143, 161, 162, 177, 179,
    194, 201, 264, 318, 381, 383, 389, 411, 412, 427, 443, 445, 464, 465,
    497, 500, 512, 513, 514, 515, 520, 521, 540, 546, 547, 548, 554, 560,
    563, 587, 591, 593, 596, 631, 636, 639, 646, 691, 860, 873, 902, 989,
    990, 993, 995, 3389, 8080
};

    const char *port_names[] = {
    "Echo", "CHARGEN", "FTP-data", "FTP", "SSH", "Telnet", "SMTP",
    "WINS-Repl", "WHOIS", "TACACS", "DNS", "DHCP-srv", "DHCP-cli", "TFTP",
    "Gopher", "Finger", "HTTP", "Kerberos", "MS-Exch-ISO", "POP3", "Ident",
    "NNTP", "NTP", "MS-RPC-EPMAP", "NetBIOS-ns", "NetBIOS-dgm",
    "NetBIOS-ssn", "IMAP", "SNMP", "SNMP-trap", "XDMCP", "BGP", "IRC",
    "AppleTalk", "BGMP", "TSP", "HP-OV-collect", "HP-OV-alarm", "LDAP",
    "DC-Hub", "DC-C2C", "SLP", "HTTPS", "MS-DS-SMB", "Kerberos-pw",
    "SMTPS", "Retrospect", "IPSec-IKE", "rexec", "rlogin", "syslog",
    "LPD", "RIP", "RIPng", "UUCP", "DHCPv6-cli", "DHCPv6-srv", "AFP",
    "RTSP", "rmonitor", "NNTPS", "SMTP-sub", "FileMaker", "MS-DCOM",
    "SMSD", "IPP", "LDAPS", "MSDP", "LDP-MPLS", "MS-Exch-Route", "iSCSI",
    "rsync", "VMware-Srv", "FTPS-data", "FTPS-ctrl", "IMAPS", "POP3S",
    "RDP", "HTTP-Alt"
};
    const int len_ports = sizeof(common_ports) / sizeof(common_ports[0]);

    printf("Scanning %s\n", arg);
    for (int i=0; i < len_ports; i++) {
        char junk[64];
        int result = tcp_response(arg, common_ports[i], junk, sizeof(junk));

        const char *status;
        if (result == 0) status="CLOSED";
        if (result > 0) status="OPEN";
        if (result <0)  status="ERROR"; // this might break ? keep an eye on this

        printf("  Port %-5d (%-8s): %s\n", common_ports[i], port_names[i], status);
    }

}

// PORT 3389 Check
void rdp(const char *arg) {
    if (arg == NULL || arg[0] == '\0') {
        printf("[!] Invalid input\n");
        printf("Usage: rdp <ip_address>\n");
        return;
    }
    int port = 3389;
    char response[1024] = {0};
    int result = tcp_response(arg, port, response, sizeof(response));
    // printf("Debug line: Response: %s, Result: %d", response, result); they return None and -1. idk why 8.21.2026, After midnight // 8.21.2026 1PM: fixed it:
    /* TCP response function was broken. We never called WSAStartup. now though, with the new tcp response, 1: Connected, 0: Not connected*/
    if (result > 0) {
        printf("TCP:3389 Responded with:\n");
        printf("Response: %s, Result: %d\n", response, result );
        printf("Destination has an open 3389 port.\n");
        return;
        // add coverage for other ports also
    }
    else if (result==0){
        printf("[!] Could not connect to RDP on TCP:3389, on destination IP address\n");
        return;
    }
    else {
        printf("[!] An error occured.\n");
    }
}