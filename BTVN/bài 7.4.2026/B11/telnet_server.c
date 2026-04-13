#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define MAX_CLIENT 64
#define BUFFER_SIZE 1024

int clients[MAX_CLIENT];
int logged[MAX_CLIENT];

int find_client(int fd, int total) {
    for (int i = 0; i < total; i++) {
        if (clients[i] == fd) return i;
    }
    return -1;
}

void remove_client(int idx, int *total, fd_set *set) {
    close(clients[idx]);
    FD_CLR(clients[idx], set);

    for (int i = idx; i < *total - 1; i++) {
        clients[i] = clients[i + 1];
        logged[i] = logged[i + 1];
    }
    (*total)--;
}

int check_login(char *user, char *pass) {
    FILE *f = fopen("database.txt", "r");
    if (!f) return 0;

    char u[64], p[64];
    while (fscanf(f, "%s %s", u, p) == 2) {
        if (strcmp(user, u) == 0 && strcmp(pass, p) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

void handle_login(int fd, int idx, char *buf) {
    char user[64], pass[64];

    if (sscanf(buf, "%s %s", user, pass) == 2 &&
        check_login(user, pass)) {

        logged[idx] = 1;
        send(fd, "Dang nhap thanh cong!\n", 22, 0);
    } else {
        send(fd, "Sai tai khoan/mat khau!\n", 25, 0);
    }
}

void handle_command(int fd, char *cmd_input) {
    char cmd[1200];
    snprintf(cmd, sizeof(cmd), "%s > out.txt", cmd_input);

    system(cmd);

    FILE *f = fopen("out.txt", "r");
    if (!f) {
        send(fd, "Khong mo duoc file!\n", 21, 0);
        return;
    }

    char buf[BUFFER_SIZE];
    int has_output = 0;

    while (fgets(buf, sizeof(buf), f)) {
        send(fd, buf, strlen(buf), 0);
        has_output = 1;
    }

    if (!has_output) {
        send(fd, "(Khong co output)\n", 18, 0);
    }

    fclose(f);
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Dung: %s <port>\n", argv[0]);
        return 1;
    }

    int server = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server, (struct sockaddr*)&addr, sizeof(addr));
    listen(server, 5);

    printf("Server dang chay o port %s...\n", argv[1]);

    fd_set master, readfds;
    FD_ZERO(&master);
    FD_SET(server, &master);

    int maxfd = server;
    int total = 0;

    char buffer[BUFFER_SIZE];

    while (1) {
        readfds = master;

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0)
            break;

        for (int i = 0; i <= maxfd; i++) {
            if (!FD_ISSET(i, &readfds)) continue;

            // client mới
            if (i == server) {
                int client = accept(server, NULL, NULL);

                FD_SET(client, &master);
                if (client > maxfd) maxfd = client;

                clients[total] = client;
                logged[total] = 0;
                total++;

                send(client,
                    "Nhap user pass (vd: admin admin):\n",
                    39, 0);
            }

            // client gửi data
            else {
                int bytes = recv(i, buffer, sizeof(buffer) - 1, 0);

                if (bytes <= 0) {
                    int idx = find_client(i, total);
                    if (idx != -1)
                        remove_client(idx, &total, &master);
                } else {
                    buffer[bytes] = 0;
                    buffer[strcspn(buffer, "\r\n")] = 0;

                    int idx = find_client(i, total);
                    if (idx == -1) continue;

                    if (!logged[idx]) {
                        handle_login(i, idx, buffer);
                    } else {
                        handle_command(i, buffer);
                    }
                }
            }
        }
    }

    close(server);
    return 0;
}