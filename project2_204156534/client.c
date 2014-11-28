#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define BUF_SIZE 2048

int main(int argc, char *argv[]) {
    struct sockaddr_in server;
    int len = sizeof(struct sockaddr_in);
    char buffer[BUF_SIZE];
    struct hostent *host;
    int message_length, sock, port;

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <host> <port> <filename>\n", argv[0]);
        return 1;
    }

    host = gethostbyname(argv[1]);
    if (host == NULL) {
        perror("gethostbyname");
        return 1;
    }

    port = atoi(argv[2]);

    /* initialize socket */
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("socket");
        return 1;
    }

    /* initialize server addr */
    memset((char *) &server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr = *((struct in_addr*) host->h_addr);

    char* filename = argv[3];

    /* send filename */
    if (sendto(sock, filename, strlen(filename), 0, (struct sockaddr *) &server, len) == -1) {
        perror("sendto()");
        return 1;
    }
    printf("requesting file: '%s'\n", filename);

    /* receive echo.
    ** for single message, "while" is not necessary. But it allows the client 
    ** to stay in listen mode and thus function as a "server" - allowing it to 
    ** receive message sent from any endpoint.
    */
    while ((message_length = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr *) &server, &len)) != -1) {
        printf("Received from %s:%d: \n",  inet_ntoa(server.sin_addr), ntohs(server.sin_port)); 
        fflush(stdout);
        write(1, buffer, message_length);
        write(1, "\n", 1);
    }

    close(sock);
    return 0;
}
