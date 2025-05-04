#ifndef FUNCTIONS_H
#define FUNCTIONS_H

/*
 * Function to establish a connection to the server.
 *
 * @param port: Port number
 * @return: Connection file descriptor or -1 on error
 */
int connection(char *port);

#endif
