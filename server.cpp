#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <assert.h>

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static void msg(const char *msg) {
    printf("%s\n",msg);
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

 static int32_t read_full(int fd, char *buf, size_t n) {
     while (n > 0) {
         ssize_t rv = read(fd, buf, n);
         if (rv <= 0) {
             return -1;  // error, or unexpected EOF
         }
         assert((size_t)rv <= n);
         n -= (size_t)rv;
         buf += rv;
     }
     return 0;
 }

 static int32_t write_all(int fd, const char *buf, size_t n) {
     while (n > 0) {
         ssize_t rv = write(fd, buf, n);
         if (rv <= 0) {
             return -1;  // error
         }
         assert((size_t)rv <= n);
         n -= (size_t)rv;
         buf += rv;
     }
     return 0;
 }



 const size_t k_max_msg = 4096;

 static int32_t one_request(int connfd) {
    // 4 bytes header
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4); // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // request body
    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    // do something
    rbuf[4 + len] = '\0';
    printf("client says: %s\n", &rbuf[4]);

    // reply using the same protocol
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, 4 + len);
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
    rv = listen(fd, SOMAXCONN); // automatically handle TCP handshakes and place established connection in a queue
    if (rv) {
        die("listen()");
    }

    while (true){
        // accept
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen); // accept a connection on a listing socket
        if (connfd < 0) {
            continue; //error
        }

        while (true) {
            int32_t err = one_request(connfd);
            if (err) {
                break;
            }
        }
        // do_something(connfd);
        close(connfd);
    }

    return 0;
}
