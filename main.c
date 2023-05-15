#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define MAX_CLIENTS 10
#define MAX_USERNAME_LENGTH 100
#define MAX_MESSAGE_LENGTH 2000

int clients[MAX_CLIENTS];
int server_sock;
int n_clients = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast(char* message, int sender_sock, char* username) {
    for (int i = 0; i < n_clients; i++) {
        if (clients[i] != sender_sock) {
            char broadcast_msg[MAX_MESSAGE_LENGTH + MAX_USERNAME_LENGTH + 3]; // Increase size to allow for \r character
            memset(broadcast_msg, 0, sizeof(broadcast_msg));
            strcat(broadcast_msg, username);
            strcat(broadcast_msg, ": ");
            strcat(broadcast_msg, message);
            strcat(broadcast_msg, "\r"); // Add \r character
            send(clients[i], broadcast_msg, strlen(broadcast_msg), 0);
        }
    }
}


void *client_handler(void *arg) {
    int client_sock = *(int *)arg;
    char message[MAX_MESSAGE_LENGTH];
    char username[MAX_USERNAME_LENGTH];

    // Prompt the client for the username
    send(client_sock, "Enter username: ", strlen("Enter username: "), 0);
    int bytes_received = (int)recv(client_sock, username, sizeof(username), 0);
    if (bytes_received <= 0) {
        printf("Failed to receive username from client\n");
        close(client_sock);
        return NULL;
    }
    username[bytes_received] = '\0';
    printf("New client connected: %s\n", username);

    while (1) {
        memset(message, 0, sizeof(message));
        int other_bytes_received = (int)recv(client_sock, message, sizeof(message), 0);
        if (other_bytes_received <= 0) {
            printf("Client %s disconnected\n", username);
            break;
        }
        printf("%s: %s\n", username, message);
        broadcast(message, client_sock, username);
    }

    // Remove the client from the list of connected clients
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < n_clients; i++) {
        if (clients[i] == client_sock) {
            for (int j = i; j < n_clients - 1; j++) {
                clients[j] = clients[j + 1];
            }
            n_clients--;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    close(client_sock);
    return NULL;
}

void sigint_handler(int sig) {
    printf("Server shutting down...\n");
    char* msg = "Server has disconnected.\n";
    broadcast(msg, -1, "");
    close(server_sock);
    exit(0);
}

int main() {
    int client_sock;
    struct sockaddr_in server_addr, client_addr;
    pthread_t thread;

    // Initialize the signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Failed to register signal handler");
        exit(1);
    }

    // Create the server socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Failed to create socket");
        exit(1);
    }

    // Set socket options to reuse the address and port
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Failed to set socket options");
        exit(1);
    }

    // Bind the socket to a specific IP address and port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8888);
    if (bind(server_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind");
        exit(1);
    }

    // Listen for incoming connections
    if (listen(server_sock, 3) < 0) {
        perror("Failed to listen");
        exit(1);
    }

    printf("Server started on port 8888\n");

    // Handle incoming connections
    while (1) {
        socklen_t addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr *) &client_addr, &addr_size);

        // Reject connection if the maximum number of clients is reached
        if (n_clients == MAX_CLIENTS) {
            printf("Client limit reached. Connection rejected\n");
            close(client_sock);
            continue;
        }

        // Add the new client to the list of connected clients
        clients[n_clients] = client_sock;
        n_clients++;

        // Create a new thread to handle the client
        pthread_create(&thread, NULL, client_handler, (void *) &client_sock);

        // Check if all clients have disconnected
        int i;
        for (i = 0; i < n_clients; i++) {
            if (clients[i] != -1) {
                // At least one client is still connected, continue accepting connections
                break;
            }
        }

        if (i == n_clients) {
            // All clients have disconnected, exit the while loop and shutdown the server
            break;
        }
    }

    // Shutdown the server
    printf("Shutting down server\n");
    close(server_sock);

    return 0;
}
