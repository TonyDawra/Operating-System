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

#define BUFFER_SIZE 1000

pthread_mutex_t mutex;
struct Client {
  char *username;
  int connection_fd;
  struct Client *next;
};

struct Client *userList = NULL;

// Function to add a user to the linked list of users
void addUser(struct Client *user) {
  if (userList == NULL) {
    // If the user list is empty, set the user as the first element
    userList = user;
    user->next = NULL;
  } else {
    // If the user list is not empty, add the user to the beginning
    user->next = userList;
    userList = user;
  }
}

// Function to delete a user from the linked list of users
void deleteUser(int connection_fd) {
  struct Client *user = userList;
  struct Client *previous = NULL;

  // Traverse the list to find the user with the specified connection_fd
  while (user != NULL && user->connection_fd != connection_fd) {
    previous = user;
    user = user->next;
  }

  if (user == NULL)
    return; // User not found

  // Remove the user from the linked list
  if (previous == NULL)
    userList = user->next; // If the user is the first in the list
  else
    previous->next = user->next; // If the user is not the first

  free(user->username);
  free(user);
}

// Function to create a listening socket
int createListeningSocket(char *port) {
  struct addrinfo *p, *listp, hints;
  int rc, listen_fd, optval = 1;

  // Initialize hints for getaddrinfo
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_ADDRCONFIG | AI_PASSIVE;  // AI_PASSIVE for server

  // Get a list of potential address structures using getaddrinfo
  if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0) {
    fprintf(stderr, "getaddrinfo failed: Port number %s is invalid\n", port);
    return -1;
  }

  // Iterate over the list of potential address structures to find a suitable one
  for (p = listp; p; p = p->ai_next) {
    // Create a socket
    listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (listen_fd < 0) {
      continue; // Socket creation failed, try the next one
    }

    // Set socket option to reuse the address
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

    // Bind the socket to a specific address and port
    if (bind(listen_fd, p->ai_addr, p->ai_addrlen) == 0) {
      break; // Binding successful, use this address
    }

    // Close the socket if binding failed
    if (close(listen_fd) < 0) {
      fprintf(stderr, "open_listenfd close failed: %s\n", strerror(errno));
      return -1;
    }
  }

  // Free the address structures obtained from getaddrinfo
  freeaddrinfo(listp);

  // If p is NULL, no suitable address was found
  if (!p) {
    return -1;
  }

  // Make the socket a listening socket with a backlog of 1024 pending connections
  if (listen(listen_fd, 1024) < 0) {
    close(listen_fd);
    return -1;
  }

  return listen_fd;
}

// Function to send a message to all clients except the sender
void sendMessageToAll(int connection_fd, char *message, char *sender) {
  char response[BUFFER_SIZE];
  struct Client *user = userList;

  while (user != NULL) {
    if (user->connection_fd != connection_fd) {
      // Prepare the message format and send it to each client
      sprintf(response, "start\n%s:%s\n\r\n", sender, message);
      rio_writen(user->connection_fd, response, strlen(response));
    }
    user = user->next;
  }

  // Notify the sender that the message was sent to all
  strcpy(response, "Message sent to all\n\r\n");
  rio_writen(connection_fd, response, strlen(response));
}

// Function to send a message to a specific client
void sendMessage(int connection_fd, char *message, char *receiver, char *sender) {
  char response[BUFFER_SIZE];
  struct Client *user = userList;

  // If no specific receiver is specified, send the message to all clients
  if (receiver == NULL || receiver[0] == '\0') {
    sendMessageToAll(connection_fd, message, sender);
  } else {
    while (user != NULL) {
      // Find the user with the specified username and send the message
      if (!strcmp(user->username, receiver)) {
        sprintf(response, "start\n%s:%s\n\r\n", sender, message);
        rio_writen(user->connection_fd, response, strlen(response));
        // Notify the sender that the message was sent
        strcpy(response, "Message sent\n\r\n");
        rio_writen(connection_fd, response, strlen(response));
        return;
      }
      user = user->next;
    }
    // Notify the sender that the specified user was not found
    strcpy(response, "User not found\n\r\n");
    rio_writen(connection_fd, response, strlen(response));
  }
}

// Function to evaluate and execute client commands
void evaluateCommand(char *command, int connection_fd, char *username) {
  char response[BUFFER_SIZE];
  char message[BUFFER_SIZE];
  char receiver[BUFFER_SIZE];
  char keyword[BUFFER_SIZE];

  message[0] = '\0';
  receiver[0] = '\0';
  keyword[0] = '\0';

  struct Client *user = userList;

  // Handle the "help" command
  if (!strcmp(command, "help")) {
    sprintf(response,
            "msg \"text\": Send a message to all clients online\n");
    sprintf(response,
            "%smsg \"text\" user: Send a message to a specific client\n",
            response);
    sprintf(response, "%sonline: Get the username of all clients online\n",
            response);
    sprintf(response, "%squit: Exit the chatroom\n\r\n", response);
    rio_writen(connection_fd, response, strlen(response));
    return;
  }

  // Handle the "online" command
  if (!strcmp(command, "online")) {
    char online_users[BUFFER_SIZE];
    online_users[0] = '\0'; 

    pthread_mutex_lock(&mutex);

    // Concatenate the usernames of online users
    struct Client *current_user = userList;
    while (current_user != NULL) {
      strcat(online_users, current_user->username); 
      strcat(online_users, "\n"); 
      current_user = current_user->next;
    }

    pthread_mutex_unlock(&mutex);

    // Add the terminating characters and send the list to the client
    strcat(online_users, "\r\n");
    rio_writen(connection_fd, online_users, strlen(online_users));
    return;
  }

  // Handle the "quit" command
  if (!strcmp(command, "quit")) {
    pthread_mutex_lock(&mutex);
    // Delete the user from the list and notify the client to exit
    deleteUser(connection_fd);
    pthread_mutex_unlock(&mutex);
    strcpy(response, "exit");
    rio_writen(connection_fd, response, strlen(response));
    close(connection_fd);
    return;
  }

  // Parse the command to extract the keyword, message, and receiver
  sscanf(command, "%s \" %[^\"] \"%s", keyword, message, receiver);

  // Handle the "msg" command
  if (!strcmp(keyword, "msg")) {
    pthread_mutex_lock(&mutex);
    // If no specific receiver is specified, send the message to all clients
    if (receiver[0] == '\0') {
      sendMessageToAll(connection_fd, message, username);
    } else {
      // Send the message to the specified receiver
      sendMessage(connection_fd, message, receiver, username);
    }
    pthread_mutex_unlock(&mutex);
  } else {
    // Notify the client of an invalid command
    strcpy(response, "Invalid command\n\r\n");
    rio_writen(connection_fd, response, strlen(response));
  }
}

// Function to handle communication with a client
void *handleClient(void *vargp) {
  char username[BUFFER_SIZE];
  rio_t rio;
  struct Client *user;
  long byte_size;
  char command[BUFFER_SIZE];

  // Detach the thread
  pthread_detach(pthread_self());

  int connection_fd = *((int *)vargp);
  rio_readinitb(&rio, connection_fd);

  // Read the username from the client
  if ((byte_size = rio_readlineb(&rio, username, BUFFER_SIZE)) == -1) {
    close(connection_fd);
    free(vargp);
    return NULL;
  }

  username[byte_size - 1] = '\0';

  // Create a new client structure
  user = malloc(sizeof(struct Client));

  if (user == NULL) {
    perror("Memory allocation error");
    close(connection_fd);
    free(vargp);
    return NULL;
  }

  // Allocate memory for the username and copy it
  user->username = malloc(sizeof(username));
  memcpy(user->username, username, strlen(username) + 1);
  user->connection_fd = connection_fd;

  // Lock the mutex before modifying the user list
  pthread_mutex_lock(&mutex);
  // Add the user to the list
  addUser(user);
  // Unlock the mutex after modifying the user list
  pthread_mutex_unlock(&mutex);

  // Continuously read commands from the client and evaluate them
  while ((byte_size = rio_readlineb(&rio, command, BUFFER_SIZE)) > 0) {
    command[byte_size - 1] = '\0';
    evaluateCommand(command, connection_fd, username);
  }

  return NULL;
}

// Main function for the server
int main(int argc, char **argv) {
  struct sockaddr_storage client_address;
  socklen_t client_len;
  int listen_fd = -1;
  char *port = "80";

  // If a port is provided as a command-line argument, use it
  if (argc > 1)
    port = argv[1];

  pthread_mutex_init(&mutex, NULL);

  // Create a listening socket
  listen_fd = createListeningSocket(port);

  // Exit if the socket creation fails
  if (listen_fd == -1) {
    printf("Connection failed\n");
    exit(EXIT_FAILURE);
  }

  printf("Waiting at localhost and port '%s'\n", port);

  // Continuously accept incoming client connections
  while (1) {
    int *connection_fd = malloc(sizeof(int));
    if (connection_fd == NULL) {
      perror("Memory allocation error");
      exit(EXIT_FAILURE);
    }

    client_len = sizeof(struct sockaddr_storage);
    // Accept a new client connection
    *connection_fd = accept(listen_fd, (struct sockaddr *)&client_address, &client_len);
    if (*connection_fd == -1) {
      perror("Accept failed");
      free(connection_fd);
      continue;
    }

    printf("A new client is online\n");

    // Create a new thread to handle the client
    pthread_t tid;
    if (pthread_create(&tid, NULL, handleClient, connection_fd) != 0) {
      perror("Thread creation failed");
      free(connection_fd);
      close(*connection_fd);
      continue;
    }
  }

  // Cleanup and close the listening socket
  pthread_mutex_destroy(&mutex);
  close(listen_fd);
  return 0;
}
