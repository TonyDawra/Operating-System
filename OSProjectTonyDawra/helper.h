#ifndef HELPER_H
#define HELPER_H

#include <stdio.h>
#include <stdlib.h>
#include "functions.h"

#define RIO_BUFSIZE 8192

// Rio buffer structure for robust I/O operations
typedef struct {
  int rio_fd;                // File descriptor
  ssize_t rio_cnt;           // Number of unread bytes in the buffer
  char *rio_bufptr;          // Next unread byte in the buffer
  char rio_buf[RIO_BUFSIZE]; // Buffer to store data
} rio_t;

// Function prototypes for robust I/O operations
ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_writen(int fd, const void *usrbuf, size_t n);
void rio_readinitb(rio_t *rp, int fd);
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n);
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

#endif
