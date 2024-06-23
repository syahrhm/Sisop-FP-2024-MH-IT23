#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <crypt.h>
#include <sys/stat.h>
#include <errno.h>

#define SERVER_PORT 12345
#define USERS_FILE "DiscorIT/users.csv"
#define CHANNELS_FILE "DiscorIT/channels.csv"
#define SALT "$6$rounds=5000$usesomesillystringforsalt$"  // Example salt for SHA-512
#define BUFFER_SIZE 1000  // Adjusted to ensure enough space

// Function to check if file exists
int file_exists(const char *filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

// Function to create necessary directories and files
void initialize_files() {
    if (!file_exists("DiscorIT")) {
        mkdir("DiscorIT", 0755);
    }

    if (!file_exists(USERS_FILE)) {
        FILE *file = fopen(USERS_FILE, "w");
        if (file == NULL) {
            perror("Failed to create users file");
            exit(EXIT_FAILURE);
        }
        fclose(file);
    }

    if (!file_exists(CHANNELS_FILE)) {
        FILE *file = fopen(CHANNELS_FILE, "w");
        if (file == NULL) {
            perror("Failed to create channels file");
            exit(EXIT_FAILURE);
        }
        fclose(file);
    }
}

void register_user(int client_sockfd, char *username, char *password) {
    FILE *file = fopen(USERS_FILE, "a+");
    if (!file) {
        perror("Failed to open users file");
        return;
    }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        char existing_username[50];
        sscanf(line, "%*d,%49[^,],%*s,%*s", existing_username);
        if (strcmp(existing_username, username) == 0) {
            send(client_sockfd, "Username already taken\n", 23, 0);
            fclose(file);
            return;
        }
    }

    char *hashed_password = crypt(password, SALT);
    if (hashed_password == NULL) {
        send(client_sockfd, "Error hashing password\n", 23, 0);
        fclose(file);
        return;
    }

    int id = 1;
    fseek(file, 0, SEEK_SET);
    while (fgets(line, sizeof(line), file)) {
        int existing_id;
        sscanf(line, "%d,%*s,%*s,%*s", &existing_id);
        if (existing_id >= id) {
            id = existing_id + 1;
        }
    }

    char *role = (id == 1) ? "ROOT" : "USER";
    fprintf(file, "%d,%s,%s,%s\n", id, username, hashed_password, role);
    fclose(file);

    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), "%s successfully registered\n", username);
    send(client_sockfd, response, strlen(response), 0);
}

void login_user(int client_sockfd, char *username, char *password) {
    FILE *file = fopen(USERS_FILE, "r");
    if (!file) {
        perror("Failed to open users file");
        return;
    }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        int id;
        char existing_username[50], existing_password[200], role[10];
        sscanf(line, "%d,%49[^,],%199[^,],%9s", &id, existing_username, existing_password, role);

        if (strcmp(existing_username, username) == 0) {
            char *hashed_password = crypt(password, existing_password);  // Use existing hash as salt
            if (hashed_password == NULL) {
                send(client_sockfd, "Error hashing password\n", 23, 0);
                fclose(file);
                return;
            }

            printf("Stored hash: %s\n", existing_password); // Debug log
            printf("Computed hash: %s\n", hashed_password); // Debug log

            if (strcmp(hashed_password, existing_password) == 0) {
                char response[BUFFER_SIZE];
                snprintf(response, sizeof(response), "%s successfully logged in\n", username);
                send(client_sockfd, response, strlen(response), 0);
            } else {
                send(client_sockfd, "Incorrect password\n", 19, 0);
            }
            fclose(file);
            return;
        }
    }
    send(client_sockfd, "Username not found\n", 19, 0);
    fclose(file);
}

// Fungsi untuk membuat channel baru
void create_channel(int client_sockfd, char *username, char *channel_name, char *key) {
    // Membuat direktori channel jika belum ada
    char channel_path[BUFFER_SIZE];
    snprintf(channel_path, sizeof(channel_path), "DiscorIT/%s", channel_name);
    if (mkdir(channel_path, 0755) == -1) {
        if (errno != EEXIST) {
            perror("Failed to create channel directory");
            send(client_sockfd, "Failed to create channel\n", 25, 0);
            return;
        }
    }

    // Membuat direktori admin di dalam channel
    char admin_path[BUFFER_SIZE];
    snprintf(admin_path, sizeof(admin_path), "%s/admin", channel_path);
    if (mkdir(admin_path, 0755) == -1) {
        if (errno != EEXIST) {
            perror("Failed to create admin directory");
            send(client_sockfd, "Failed to create admin directory\n", 33, 0);
            return;
        }
    }

    // Membuat file auth.csv untuk channel
    char auth_file[BUFFER_SIZE];
    snprintf(auth_file, sizeof(auth_file), "%s/auth.csv", admin_path);
    FILE *auth_fp = fopen(auth_file, "w");
    if (!auth_fp) {
        perror("Failed to create auth.csv file");
        send(client_sockfd, "Failed to create auth.csv file\n", 31, 0);
        return;
    }
    // Menambahkan admin ke file auth.csv
    fprintf(auth_fp, "1,%s,ADMIN\n", username);
    fclose(auth_fp);

    // Menambahkan channel ke channels.csv
    FILE *channels_fp = fopen(CHANNELS_FILE, "a+");
    if (!channels_fp) {
        perror("Failed to open channels.csv file");
        send(client_sockfd, "Failed to open channels.csv file\n", 33, 0);
        return;
    }

    char line[BUFFER_SIZE];
    int id = 1;
    while (fgets(line, sizeof(line), channels_fp)) {
        int existing_id;
        sscanf(line, "%d,%*s,%*s", &existing_id);
        if (existing_id >= id) {
            id = existing_id + 1;
        }
    }

    char *hashed_key = crypt(key, SALT);
    if (hashed_key == NULL) {
        send(client_sockfd, "Error hashing key\n", 18, 0);
        fclose(channels_fp);
        return;
    }

    fprintf(channels_fp, "%d,%s,%s\n", id, channel_name, hashed_key);
    fclose(channels_fp);

    send(client_sockfd, "Channel created successfully\n", 29, 0);
}

void *handle_client(void *client_sockfd_ptr) {
    int client_sockfd = *(int *)client_sockfd_ptr;
    char buffer[BUFFER_SIZE];
    while (1) {
        int len = recv(client_sockfd, buffer, sizeof(buffer), 0);
        if (len <= 0) {
            break;
        }
        buffer[len] = '\0';

        char command[20], username[50], password[50], channel_name[50], key[50];
        if (sscanf(buffer, "%s %s -p %s", command, username, password) == 3) {
            if (strcmp(command, "REGISTER") == 0) {
                register_user(client_sockfd, username, password);
            } else if (strcmp(command, "LOGIN") == 0) {
                login_user(client_sockfd, username, password);
            } else {
                send(client_sockfd, "Invalid command\n", 16, 0);
            }
        } else if (sscanf(buffer, "CREATE CHANNEL %s -k %s", channel_name, key) == 2) {
            create_channel(client_sockfd, username, channel_name, key);
        } else {
            send(client_sockfd, "Invalid command format\n", 23, 0);
        }
    }
    close(client_sockfd);
    return NULL;
}

void start_server() {
    int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sockfd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sockfd, 5) < 0) {
        perror("Listen failed");
        close(server_sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", SERVER_PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (client_sockfd < 0) {
            perror("Accept failed");
            continue;
        }

        pthread_t thread;
        int *client_sockfd_ptr = malloc(sizeof(int));
        *client_sockfd_ptr = client_sockfd;
        pthread_create(&thread, NULL, handle_client, client_sockfd_ptr);
        pthread_detach(thread);
    }

    close(server_sockfd);
}

int main() {
    initialize_files();
    start_server();
    return 0;
}
