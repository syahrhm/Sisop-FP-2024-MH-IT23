#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netinet/in.h>

#define SERVER_PORT 12345
#define BUF_SIZE 1024

void *receive_handler(void *socket_desc) {
    int sock = *(int*)socket_desc;
    char buffer[BUF_SIZE];
    int read_size;

    while ((read_size = recv(sock, buffer, BUF_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';
        printf("%s", buffer);
    }

    if (read_size == 0) {
        printf("Server disconnected\n");
    } else if (read_size == -1) {
        perror("recv failed");
    }

    return 0;
}

void send_command(int sock, const char *command) {
    send(sock, command, strlen(command), 0);
}

void login_to_server(int sock, const char *username, const char *password) {
    char login_command[BUF_SIZE];
    snprintf(login_command, sizeof(login_command), "LOGIN %s %s\n", username, password);
    send_command(sock, login_command);
}

void join_channel(int sock, const char *channel) {
    char join_command[BUF_SIZE];
    snprintf(join_command, sizeof(join_command), "JOIN %s\n", channel);
    send_command(sock, join_command);
}

void join_room(int sock, const char *room) {
    char join_command[BUF_SIZE];
    snprintf(join_command, sizeof(join_command), "JOIN %s\n", room);
    send_command(sock, join_command);
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in server;
    pthread_t recv_thread;

    if (argc != 5) {
        fprintf(stderr, "Usage: %s <username> <password> <channel> <room>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *username = argv[1];
    const char *password = argv[2];
    const char *channel = argv[3];
    const char *room = argv[4];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create socket");
        exit(EXIT_FAILURE);
    }

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("connect failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server\n");

    login_to_server(sock, username, password);
    join_channel(sock, channel);
    join_room(sock, room);

    if (pthread_create(&recv_thread, NULL, receive_handler, (void*)&sock) < 0) {
        perror("Could not create thread");
        close(sock);
        exit(EXIT_FAILURE);
    }

    pthread_join(recv_thread, NULL);

    close(sock);
    return 0;
}