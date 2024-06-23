#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "crypt_blowfish/bcrypt.h"

#define SERVER_PORT 12345
#define SERVER_IP "127.0.0.1"
#define BUF_SIZE 1024
#define HASH_SIZE 61 // bcrypt hash size including null terminator

void save_user_to_csv(int id, const char *username, const char *hashed_password, const char *role) {
    FILE *file = fopen("users.csv", "a");
    if (file == NULL) {
        perror("Error opening users.csv");
        return;
    }
    fprintf(file, "%d,%s,%s,%s\n", id, username, hashed_password, role);
    fclose(file);
}

void save_channel_to_csv(int id, const char *channel, const unsigned char *hashed_key) {
    FILE *file = fopen("channels.csv", "a");
    if (file == NULL) {
        perror("Error opening channels.csv");
        return;
    }
    fprintf(file, "%d,%s,", id, channel);
    for (int i = 0; i < HASH_SIZE; ++i) {
        fprintf(file, "%02x", hashed_key[i]);
    }
    fprintf(file, "\n");
    fclose(file);
}

void compute_hash(const char *password, char *hash) {
    char salt[BCRYPT_HASHSIZE];

    // Generate salt
    if (bcrypt_gensalt(12, salt) != 0) {
        perror("Error generating salt");
        return;
    }

    // Hash password
    if (bcrypt_hashpw(password, salt, hash) != 0) {
        perror("Error hashing password");
        return;
    }
}


void register_user(int sock, const char *username, const char *password) {
    char buffer[BUF_SIZE];
    char hashed_password[HASH_SIZE];

    // Compute hash of password
    compute_hash(password, hashed_password);

    snprintf(buffer, sizeof(buffer), "REGISTER %s\n", username);
    write(sock, buffer, strlen(buffer));
    read(sock, buffer, BUF_SIZE);
    printf("%s\n", buffer);

    // Save to users.csv if registration successful
    if (strstr(buffer, "berhasil register") != NULL) {
        FILE *file = fopen("users.csv", "r");
        int id = 1;
        if (file != NULL) {
            char line[BUF_SIZE];
            while (fgets(line, sizeof(line), file)) {
                id++;
            }
            fclose(file);
        }
        const char *role = (id == 1) ? "ROOT" : "USER";
        save_user_to_csv(id, username, hashed_password, role);
    }
}

void login_user(int sock, const char *username, const char *password) {
    char buffer[BUF_SIZE];
    snprintf(buffer, sizeof(buffer), "LOGIN %s %s\n", username, password);
    write(sock, buffer, strlen(buffer));
    read(sock, buffer, BUF_SIZE);
    printf("%s\n", buffer);
}

void list_channels(int sock) {
    char buffer[BUF_SIZE];
    snprintf(buffer, sizeof(buffer), "LIST Channels\n");
    write(sock, buffer, strlen(buffer));
    read(sock, buffer, BUF_SIZE);
    printf("%s\n", buffer);
}

void join_channel(int sock, const char *channel, const char *key) {
    char buffer[BUF_SIZE];
    if (key != NULL) {
        snprintf(buffer, sizeof(buffer), "JOIN %s\nKey: %s\n", channel, key);
    } else {
        snprintf(buffer, sizeof(buffer), "JOIN %s\n", channel);
    }
    write(sock, buffer, strlen(buffer));
    read(sock, buffer, BUF_SIZE);
    printf("%s\n", buffer);
}

void join_room(int sock, const char *room) {
    char buffer[BUF_SIZE];
    snprintf(buffer, sizeof(buffer), "JOIN %s\n", room);
    write(sock, buffer, strlen(buffer));
    read(sock, buffer, BUF_SIZE);
    printf("%s\n", buffer);
}

void chat(int sock, const char *text) {
    char buffer[BUF_SIZE];
    snprintf(buffer, sizeof(buffer), "CHAT %s\n", text);
    write(sock, buffer, strlen(buffer));
    read(sock, buffer, BUF_SIZE);
    printf("%s\n", buffer);
}

void see_chat(int sock) {
    char buffer[BUF_SIZE];
    snprintf(buffer, sizeof(buffer), "SEE CHAT\n");
    write(sock, buffer, strlen(buffer));
    read(sock, buffer, BUF_SIZE);
    printf("%s\n", buffer);
}

void edit_chat(int sock, int id, const char *text) {
    char buffer[BUF_SIZE];
    snprintf(buffer, sizeof(buffer), "EDIT CHAT %d %s\n", id, text);
    write(sock, buffer, strlen(buffer));
    read(sock, buffer, BUF_SIZE);
    printf("%s\n", buffer);
}

void del_chat(int sock, int id) {
    char buffer[BUF_SIZE];
    snprintf(buffer, sizeof(buffer), "DEL CHAT %d\n", id);
    write(sock, buffer, strlen(buffer));
    read(sock, buffer, BUF_SIZE);
    printf("%s\n", buffer);
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUF_SIZE];

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Handle different commands
    if (strcmp(argv[1], "REGISTER") == 0 && argc == 5 && strcmp(argv[3], "-p") == 0) {
        register_user(sock, argv[2], argv[4]);
    } else if (strcmp(argv[1], "LOGIN") == 0 && argc == 5 && strcmp(argv[3], "-p") == 0) {
        login_user(sock, argv[2], argv[4]);
    } else if (strcmp(argv[1], "LIST") == 0 && argc == 3 && strcmp(argv[2], "Channels") == 0) {
        list_channels(sock);
    } else if (strcmp(argv[1], "JOIN") == 0 && argc >= 3) {
        const char *key = NULL;
        if (argc == 5 && strcmp(argv[3], "Key:") == 0) {
            key = argv[4];
        }
        join_channel(sock, argv[2], key);
    } else if (strcmp(argv[1], "JOIN") == 0 && argc == 4) {
        join_room(sock, argv[3]);
    } else if (strcmp(argv[1], "CHAT") == 0 && argc == 3) {
        chat(sock, argv[2]);
    } else if (strcmp(argv[1], "SEE") == 0 && argc == 3 && strcmp(argv[2], "CHAT") == 0) {
        see_chat(sock);
    } else if (strcmp(argv[1], "EDIT") == 0 && argc == 5 && strcmp(argv[2], "CHAT") == 0) {
        edit_chat(sock, atoi(argv[3]), argv[4]);
    } else if (strcmp(argv[1], "DEL") == 0 && argc == 4 && strcmp(argv[2], "CHAT") == 0) {
        del_chat(sock, atoi(argv[3]));
    } else {
        fprintf(stderr, "Invalid command\n");
    }

    close(sock);
    return 0;
}