#include "functions.h"
#include "helper.h"
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXLINE 1024 /* Maximum line size for messages */

char chatPrompt[] = "Chatroom> ";

/*
 * Display usage information for the script
 */
void displayUsage() {
  printf("-h  Print help\n");
  printf("-a  Server IP address [Required]\n");
  printf("-p  Server port number [Required]\n");
  printf("-u  Enter your username [Required]\n");
}

/*
 * Connects the client to the server.
 * The function traverses the list to find an appropriate socket connection (robust).
 *
 * @param serverAddress: IP address of the server
 * @param serverPort: Port number
 * @return: Connection file descriptor
 */
int connectToServer(char *serverAddress, char *serverPort) {
  int clientSocket, resultCode;
  struct addrinfo hints, *addressList, *addressNode;

  // Initialize the hints structure to zero and set relevant fields
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM; 
  hints.ai_flags |= AI_ADDRCONFIG;
  hints.ai_flags |= AI_NUMERICSERV; 

  // Get address information for the given server address and port
  if ((resultCode = getaddrinfo(serverAddress, serverPort, &hints, &addressList)) != 0) {
    fprintf(stderr, "Error: Invalid server address or port number\n");
    return -1;
  }

  // Iterate through the list of addresses and connect to the first successful one
  for (addressNode = addressList; addressNode; addressNode = addressNode->ai_next) {
    
    // Create a socket using the address information
    clientSocket = socket(addressNode->ai_family, addressNode->ai_socktype, addressNode->ai_protocol);
    if (clientSocket < 0) {
      continue; // Try the next address if socket creation fails
    }

    // Attempt to connect to the server using the created socket
    if (connect(clientSocket, addressNode->ai_addr, addressNode->ai_addrlen) != -1) {
      break; // Connection successful, exit the loop
    }

    // If connection fails, close the socket
    if (close(clientSocket) < 0) {
      perror("Error: Close failed");
      return -1;
    }
  }

  // Free the memory allocated for the address list
  freeaddrinfo(addressList);

  // Check if a valid address was found
  if (!addressNode) {
    return -1; // No valid address found
  } else {
    return clientSocket; // Return the connection socket
  }
}

/*
 * Function for a separate thread to read and display server responses
 */
void serverResponseReader(void *socketDescriptor) {
  char buffer[MAXLINE];
  rio_t rio;
  int readStatus;
  int connectionSocket = (int)socketDescriptor;

  // Initialize the Rio buffer for reading from the socket
  rio_readinitb(&rio, connectionSocket);

  while (1) {
    // Read lines from the server and print them
    while ((readStatus = rio_readlineb(&rio, buffer, MAXLINE)) > 0) {
      
      if (readStatus == -1)
        exit(1);

      // Break the loop when an empty line is received
      if (!strcmp(buffer, "\r\n")) {
        break;
      }

      // Exit the client if the server sends "exit"
      if (!strcmp(buffer, "exit")) {
        close(connectionSocket);
        exit(0);
      }

      // Display received messages, handling special case for "start"
      if (!strcmp(buffer, "start\n")) {
        printf("\n");
      } else {
        printf("%s", buffer);
      }
    }
    
    // Display the chat prompt after processing server responses
    printf("%s", chatPrompt);
    fflush(stdout);
  }
}

int main(int argc, char **argv) {
  char *defaultServerPort = "9000"; // Default server port if not provided

  char *serverAddress = NULL, *username = NULL;
  char userCommand[MAXLINE];
  char commandOption;
  pthread_t responseThread;

  // Parse command-line arguments using getopt
  while ((commandOption = getopt(argc, argv, "hu:a:p:")) != -1) {
    switch (commandOption) {
    
    case 'h':
      displayUsage();
      exit(1);
      break;

    case 'a':
      serverAddress = optarg;
      break;

    case 'p':
      defaultServerPort = optarg; // Set the server port
      break;

    case 'u':
      username = strdup(optarg); // Duplicate and store the username
      break;

    default:
      displayUsage();
      exit(1);
    }
  }

  // Check if required command-line arguments are provided
  if (optind == 1 || defaultServerPort == NULL || serverAddress == NULL || username == NULL) {
    fprintf(stderr, "Error: Invalid command-line arguments\n");
    displayUsage();
    exit(1);
  }

  // Connect to the server
  int connectionSocket = connectToServer(serverAddress, defaultServerPort);
  if (connectionSocket == -1) {
    fprintf(stderr, "Error: Couldn't connect to the server\n");
    exit(1);
  }

  // Append newline to the username and send it to the server
  strcat(username, "\n");
  if (rio_writen(connectionSocket, username, strlen(username)) == -1) {
    perror("Error: Unable to send the data");
    close(connectionSocket);
    exit(1);
  }

  // Create a separate thread to read and display server responses
  pthread_create(&responseThread, NULL, serverResponseReader, (void *)(intptr_t)connectionSocket);

  // Display the chat prompt for user input
  printf("%s", chatPrompt);

  while (1) {
    // Read user input and send it to the server
    if ((fgets(userCommand, MAXLINE, stdin) == NULL) && ferror(stdin)) {
      perror("Error: fgets error");
      close(connectionSocket);
      exit(1);
    }

    // Send the user command to the server
    if (rio_writen(connectionSocket, userCommand, strlen(userCommand)) == -1) {
      perror("Error: Unable to send the data");
      close(connectionSocket);
      exit(1);
    }
  }

  // Free allocated memory for the username
  free(username);

  return 0;
}
