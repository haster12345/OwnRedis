#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>


static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static void do_something(int connfd) {
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        printf("read() error");
        return;
    }
    printf("cleint says: %s\n", rbuf);

    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf));
 }

int main(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)); // socket options

    // bind
    struct sockaddr_in addr = {};   // initalising the sockaddr_in struct and setting all members of the structs to zero
    addr.sin_family = AF_INET;  // IPv4
    addr.sin_port = ntohs(1234);    // convert numbers to big endian
    addr.sin_addr.s_addr = ntohl(0);    // wildcard address 0.0.0.0
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));   // configure listening address of a socket
    if (rv) {
        die("bind()");
    }

    // Make the socket a listening socket
    rv = listen(fd, SOMAXCONN); // automaticcally handle TCP handshakes and place established connection in a queue
    if (rv) {
        die("listen()");
    }

    while (true){
        // accept
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
        if (connfd < 0) {
            continue; //error
        }

        do_something(connfd);
        close(connfd);
    }

    return 0;
}
