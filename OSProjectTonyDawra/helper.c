#include "helper.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "functions.h"

/*
 * Read n bytes from a file descriptor into a buffer.
 *
 * @param fd: File descriptor
 * @param usrbuf: Buffer to store the read data
 * @param n: Number of bytes to read
 * @return: Number of bytes read, or -1 on error
 */
ssize_t rio_readn(int fd, void *usrbuf, size_t n) {
  size_t nleft = n;
  ssize_t nread;
  char *bufp = usrbuf;

  while (nleft > 0) {
    if ((nread = read(fd, bufp, nleft)) < 0) {
      if (errno != EINTR) {
        return -1; // Error other than interruption, return -1
      }

      nread = 0; // Retry if read was interrupted
    } else if (nread == 0) {
      break; // End of file
    }
    nleft -= (size_t)nread;
    bufp += nread;
  }
  return (ssize_t)(n - nleft); // Return the total number of bytes read
}

/*
 * Write n bytes from a buffer to a file descriptor.
 *
 * @param fd: File descriptor
 * @param usrbuf: Buffer containing the data to write
 * @param n: Number of bytes to write
 * @return: Number of bytes written, or -1 on error
 */
ssize_t rio_writen(int fd, const void *usrbuf, size_t n) {
  size_t nleft = n;
  ssize_t nwritten;
  const char *bufp = usrbuf;

  while (nleft > 0) {
    if ((nwritten = write(fd, bufp, nleft)) <= 0) {
      if (errno != EINTR) {
        return -1; // Error other than interruption, return -1
      }

      nwritten = 0; // Retry if write was interrupted
    }
    nleft -= (size_t)nwritten;
    bufp += nwritten;
  }
  return (ssize_t)n; // Return the total number of bytes written
}

/*
 * Read data from an internal buffer (robust version).
 *
 * @param rp: Pointer to the Rio buffer structure
 * @param usrbuf: Buffer to store the read data
 * @param n: Number of bytes to read
 * @return: Number of bytes read, or -1 on error
 */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n) {
  size_t cnt;

  while (rp->rio_cnt <= 0) { // Refill buffer if empty
    rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
    if (rp->rio_cnt < 0) {
      if (errno != EINTR) {
        return -1; // Error other than interruption, return -1
      }

      // Retry if read was interrupted
    } else if (rp->rio_cnt == 0) {
      return 0; // End of file
    } else {
      rp->rio_bufptr = rp->rio_buf; // Reset buffer pointer
    }
  }

  cnt = n;
  if ((size_t)rp->rio_cnt < n) {
    cnt = (size_t)rp->rio_cnt;
  }
  memcpy(usrbuf, rp->rio_bufptr, cnt);
  rp->rio_bufptr += cnt;
  rp->rio_cnt -= cnt;
  return (ssize_t)cnt; // Return the number of bytes read
}

/*
 * Initialize a Rio buffer structure.
 *
 * @param rp: Pointer to the Rio buffer structure
 * @param fd: File descriptor to associate with the buffer
 */
void rio_readinitb(rio_t *rp, int fd) {
  rp->rio_fd = fd;
  rp->rio_cnt = 0;
  rp->rio_bufptr = rp->rio_buf;
}

/*
 * Read n bytes from a Rio buffer (robust version).
 *
 * @param rp: Pointer to the Rio buffer structure
 * @param usrbuf: Buffer to store the read data
 * @param n: Number of bytes to read
 * @return: Number of bytes read, or -1 on error
 */
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n) {
  size_t nleft = n;
  ssize_t nread;
  char *bufp = usrbuf;

  while (nleft > 0) {
    if ((nread = rio_read(rp, bufp, nleft)) < 0) {
      return -1; // Error during reading
    } else if (nread == 0) {
      break; // End of file
    }
    nleft -= (size_t)nread;
    bufp += nread;
  }
  return (ssize_t)(n - nleft); // Return the total number of bytes read
}

/*
 * Read a line from a Rio buffer, storing at most maxlen bytes.
 *
 * @param rp: Pointer to the Rio buffer structure
 * @param usrbuf: Buffer to store the read line
 * @param maxlen: Maximum number of bytes to read
 * @return: Number of bytes read, or -1 on error
 */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) {
  size_t n;
  ssize_t rc;
  char c, *bufp = usrbuf;

  for (n = 1; n < maxlen; n++) {
    if ((rc = rio_read(rp, &c, 1)) == 1) {
      *bufp++ = c;
      if (c == '\n') {
        n++;
        break; // Newline character encountered
      }
    } else if (rc == 0) {
      if (n == 1) {
        return 0; // Empty line
      } else {
        break; // End of file
      }
    } else {
      return -1; // Error during reading
    }
  }
  *bufp = 0; // Null-terminate the string
  return (ssize_t)(n - 1); // Return the total number of bytes read
}
