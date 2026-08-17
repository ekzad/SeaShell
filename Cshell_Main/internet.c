#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <signal.h>
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