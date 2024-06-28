# Sisop-FP-2024-MH-IT23

## Laporan Resmi Sistem Operasi Final Project 2024
## Anggota Kelompok

| NRP        | Nama                            |
|:----------:|:-------------------------------:|
| 5027231020 | Acintya Edria Sudarsono         |
| 5027231044 | Dionisius Marcell Putra Indranto|
| 5027231072 | Aisyah Rahmasari                |

### Struktur Repository 

```sh
-fp/
    ---discorit.c 
    ---monitor.c 
    ---server.c 
```
## REVISI

### discorit.c


```sh
void register_user(const char *username, const char *password) {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    char message[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return;
    }

    snprintf(message, sizeof(message), "DISCORIT REGISTER %s %s", username, password);
    send(sock, message, strlen(message), 0);
    valread = read(sock, buffer, 1024);
    printf("%s\n", buffer);
}
```

Pertama, fungsi register_user(username, password) dan login_user(username, password) bertanggung jawab untuk inisialisasi socket, menghubungkan ke server yang berjalan pada alamat IP loopback 127.0.0.1 dengan port 8081, serta mengirim pesan registrasi atau login ke server dengan format yang sesuai. Setelah mengirim pesan, kedua fungsi ini menunggu balasan dari server menggunakan read() dan mencetak balasan tersebut ke terminal.

```sh
void login_user(const char *username, const char *password) {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    char message[1024] = {0};
    char current_username[256];

    strncpy(current_username, username, sizeof(current_username) - 1); // 
    current_username[sizeof(current_username) - 1] = '\0';

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return;
    }

    snprintf(message, sizeof(message), "DISCORIT LOGIN %s %s", username, password);
    send(sock, message, strlen(message), 0);
    valread = read(sock, buffer, 1024);
    buffer[valread] = '\0';
    printf("%s\n", buffer);

    if (strstr(buffer, "berhasil login")) {
        char current_channel[256] = {0};
        char current_room[256] = {0};
        while (1) {
            if(strstr(buffer, "Profil diupdate menjadi")){
                char *new_username = strstr(buffer, "Profil diupdate menjadi ") + strlen("Profil diupdate menjadi ");
                strcpy(current_username, new_username);
            } 
            
            // format path
            if (strlen(current_channel) == 0 && strlen(current_room) == 0) {
                printf("[%s]> ", current_username);
            } else if (strlen(current_channel) != 0 && strlen(current_room) == 0) {
                printf("[%s/%s]> ", current_username, current_channel);
            } else {
                printf("[%s/%s/%s]> ", current_username, current_channel, current_room);
            }

            char input[1024];
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = 0; // Remove newline character


            if(strstr(input, "-channel") && strstr(input, "-room")){
                // jangan di send
                printf("Invalid command. Try on monitor\n");
                continue;
            }            

            // Send the input to the server
            send(sock, input, strlen(input), 0);

            // Receive response from server
            valread = read(sock, buffer, 1024);
            buffer[valread] = '\0';

            char temp_channel[256];
            if (strncmp(input, "JOIN", 4) == 0) {
                
                sscanf(input, "JOIN %255s", temp_channel);
                char temp_room[256];
                sscanf(input, "JOIN %255s", temp_room);
                if (strstr(buffer, "Key:")) {
                    // Server requests key
                    printf("%s", buffer);
                    fgets(input, sizeof(input), stdin);
                    input[strcspn(input, "\n")] = 0; // Remove newline character
                    send(sock, input, strlen(input), 0);
                    valread = read(sock, buffer, 1024);
                    buffer[valread] = '\0';
                    if (strstr(buffer, "Invalid key")) {
                        // If key is incorrect, don't change current_channel
                        printf("%s", buffer);
                        continue;
                    } else {
                        strcpy(current_channel, temp_channel);
                        continue;
                    }
                } else if(strstr(buffer, "Anda dibanned")){
                    printf("%s", buffer);
                    continue;
                } else if (strstr(buffer, "Channel tidak tersedia")) {
                    // If channel is not available, don't change current_channel
                    printf("%s", buffer);
                    continue;
                } else if(strstr(buffer, "Room tidak tersedia")){
                    printf("%s", buffer);
                    continue;
                } else if (strstr(buffer, "masuk room")){
                    strcpy(current_room, temp_room);
                    continue;
                } else {
                    // If admin/root/already joined
                    strcpy(current_channel, temp_channel);
                    continue;
                } 
            } else if((strncmp(buffer, "Channel", 7) == 0 && strstr(buffer, "diubah menjadi")) && strcmp(current_channel,"")){
                    printf("%s", buffer);
                    char *new_channel = strstr(buffer, "diubah menjadi ") + strlen("diubah menjadi ");
                    new_channel[strcspn(new_channel, "\n")] = 0; // Remove newline
                    strcpy(current_channel, new_channel);
                    continue;
            } else if((strncmp(buffer, "Room", 4) == 0 && strstr(buffer, "diubah menjadi")) && strcmp(current_room,"")){
                    printf("%s", buffer);
                    char *new_room = strstr(buffer, "diubah menjadi ") + strlen("diubah menjadi ");
                    new_room[strcspn(new_room, "\n")] = 0; // Remove newline
                    strcpy(current_room, new_room);
                    continue;
            } else if (strncmp(input, "CREATE ROOM", 11) == 0){
                char temp_room[256];
                sscanf(input, "CREATE ROOM %255s", temp_room);
                if (strstr(buffer, "Room sudah ada")) { 
                    printf("%s", buffer); //print to terminal
                    continue;
                } else {
                    if (strstr(buffer, "dibuat\n")){
                        printf("%s", buffer);
                        continue;
                    } else {
                        printf("%s", buffer);
                        continue;
                    }
                }
            } else if(strncmp(input, "EXIT", 4) == 0){
                if (strstr(buffer, "exit room")){
                    strcpy(current_room, "");
                    continue;
                } else if (strstr(buffer, "exit channel")){
                    strcpy(current_channel, "");
                    continue;
                } else {
                    break;
                }
            }

            printf("%s\n", buffer);
        }
        return;
    }
}
```

Fungsi login_user(username, password) memiliki kompleksitas tambahan setelah login berhasil, yaitu memasuki loop untuk berinteraksi dengan server. Di dalam loop ini, program menanggapi input pengguna, mengirim perintah ke server, dan menerima balasan dari server. Program juga mengelola kondisi khusus seperti permintaan key untuk bergabung ke channel tertentu, perubahan channel atau room, serta pesan error jika terjadi masalah seperti room tidak tersedia atau pengguna dibanned. Loop berlanjut sampai pengguna memilih untuk keluar dengan perintah "EXIT".

```sh
int main(int argc, char const *argv[]) {
    if (argc != 5 || (strcmp(argv[1], "REGISTER") != 0 && strcmp(argv[1], "LOGIN") != 0) || strcmp(argv[3], "-p") != 0) {
        printf("Usage:\n");
        printf("Register: %s REGISTER username -p password\n", argv[0]);
        printf("Login: %s LOGIN username -p password\n", argv[0]);
        return 1;
    }

    const char *command = argv[1];
    const char *username = argv[2];
    const char *password = argv[4];

    if (strcmp(command, "REGISTER") == 0) {
        register_user(username, password);
    } else if (strcmp(command, "LOGIN") == 0) {
        login_user(username, password);
    }

    return 0;
}
```
Fungsi main(argc, argv) bertanggung jawab untuk memvalidasi argumen yang diberikan pada command line, memastikan formatnya sesuai untuk operasi REGISTER atau LOGIN, dan memanggil fungsi register_user() atau login_user() sesuai dengan argumen yang diberikan. Jika format argumen tidak sesuai, program memberikan petunjuk penggunaan yang benar.

#### Secara keseluruhan, kode ini menyediakan antarmuka untuk registrasi dan login pengguna ke server DISCORIT, serta interaksi lanjutan setelah login berhasil, termasuk pengelolaan channel, room, dan pesan antara pengguna dan server.




