#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

int connect_inet(char *host, char *service)
{
    struct addrinfo hints, *info_list, *info;
    int sock, error;

    // look up remote host
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;  // in practice, this means give us IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // indicate we want a streaming socket

    error = getaddrinfo(host, service, &hints, &info_list);
    if (error) {
        fprintf(stderr, "error looking up %s:%s: %s\n", host, service, gai_strerror(error));
        return -1;
    }

    for (info = info_list; info != NULL; info = info->ai_next) {
        sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
        if (sock < 0) continue;

        error = connect(sock, info->ai_addr, info->ai_addrlen);
        if (error) {
            close(sock);
            continue;
        }

        break;
    }
    freeaddrinfo(info_list);

    if (info == NULL) {
        fprintf(stderr, "Unable to connect to %s:%s\n", host, service);
        return -1;
    }

    return sock;
}

#define BUFSIZE 1024

int main(int argc, char **argv)
{
    int sock;
    char buf[BUFSIZE];

    if (argc != 3) {
        printf("Specify host and service\n");
        exit(EXIT_FAILURE);
    }

    sock = connect_inet(argv[1], argv[2]);
    if (sock < 0) exit(EXIT_FAILURE);

    int started = 0;
    //int waiting  =0;

    while (1) {
        printf("Enter a string (or 'q' to quit): ");

        fgets(buf, BUFSIZE, stdin);
        buf[strcspn(buf, "\n")] = '\0';

        if (strcmp(buf, "q") == 0) {
            break;
        } else if(strlen(buf) == 0) {
            printf("EMPTY\n");
        } else {
            int send_size = send(sock, buf, strlen(buf), 0);
            if (send_size == -1) {
                perror("Error sending data");
                exit(1);
            }

            started = 1;
        }

        if(started == 1) {
            int recv_size = recv(sock, buf, BUFSIZE, 0);
            if (recv_size <= 0) {
                perror("Error receiving da");
                exit(1);
            }
            buf[recv_size] = '\0';

            printf("Response from server: %s\n", buf);
            /*if(strncmp(buf, "WAIT") == 0) {
                //waiting = 1;
            }*/
        }
    }

    close(sock);

    return EXIT_SUCCESS;
}
