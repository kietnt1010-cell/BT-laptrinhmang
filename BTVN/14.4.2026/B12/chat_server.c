#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>

#define MAX_CLIENT 100
#define BUFFER_SIZE 1024

struct client {
    int fd;
    int logged_in;
    char id[50];
    char name[50];
};

struct client clients[MAX_CLIENT];

void init_clients() {
    for (int i = 0; i < MAX_CLIENT; i++) {
        clients[i].fd = -1;
        clients[i].logged_in = 0;
    }
}

void add_client(int fd) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (clients[i].fd == -1) {
            clients[i].fd = fd;
            clients[i].logged_in = 0;
            return;
        }
    }
}

void remove_client(int fd) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (clients[i].fd == fd) {
            close(fd);
            clients[i].fd = -1;
            clients[i].logged_in = 0;
            return;
        }
    }
}

char* get_time_str() {
    static char buf[32];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    // format gọn: HH:MM:SS
    strftime(buf, sizeof(buf), "%H:%M:%S", t);
    return buf;
}

// parse "id: name"
int parse_login(char *msg, char *id, char *name) {
    char *p = strchr(msg, ':');
    if (!p) return 0;

    *p = '\0';
    strcpy(id, msg);

    char *n = p + 1;
    while (*n == ' ') n++;

    if (strlen(id) == 0 || strlen(n) == 0) return 0;

    strcpy(name, n);
    name[strcspn(name, "\r\n")] = 0;

    return 1;
}

void broadcast(int sender_fd, char *msg) {
    char buffer[BUFFER_SIZE];
    char sender_name[50] = "Unknown";

    // tìm tên người gửi
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (clients[i].fd == sender_fd) {
            strcpy(sender_name, clients[i].name);
            break;
        }
    }

    char *time_str = get_time_str();

    for (int i = 0; i < MAX_CLIENT; i++) {
        if (clients[i].fd != -1 &&
            clients[i].fd != sender_fd &&
            clients[i].logged_in) {

            snprintf(buffer, sizeof(buffer),
                     "[%s] %s: %s\r\n",
                     time_str,
                     sender_name,
                     msg);

            send(clients[i].fd, buffer, strlen(buffer), 0);
        }
    }
}

int main() {
    int server_fd, new_fd, max_fd;
    struct sockaddr_in server_addr;
    fd_set readfds;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5000);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, 5);

    init_clients();

    printf("Server started...\n");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;

        for (int i = 0; i < MAX_CLIENT; i++) {
            if (clients[i].fd != -1) {
                FD_SET(clients[i].fd, &readfds);
                if (clients[i].fd > max_fd)
                    max_fd = clients[i].fd;
            }
        }

        select(max_fd + 1, &readfds, NULL, NULL, NULL);

        // client mới
        if (FD_ISSET(server_fd, &readfds)) {
            new_fd = accept(server_fd, NULL, NULL);
            add_client(new_fd);

            char *msg = "Nhap ten dang nhap (format: id:name):\r\n> ";
            send(new_fd, msg, strlen(msg), 0);
        }

        // xử lý client
        for (int i = 0; i < MAX_CLIENT; i++) {
            int fd = clients[i].fd;

            if (fd != -1 && FD_ISSET(fd, &readfds)) {
                char buffer[BUFFER_SIZE];
                int len = recv(fd, buffer, sizeof(buffer) - 1, 0);

                if (len <= 0) {
                    remove_client(fd);
                } else {
                    buffer[len] = 0;

                    if (!clients[i].logged_in) {
                        char id[50], name[50];

                        if (parse_login(buffer, id, name)) {
                            strcpy(clients[i].id, id);
                            strcpy(clients[i].name, name);
                            clients[i].logged_in = 1;

                            char *ok = "Dang nhap thanh cong!\r\n-----------------------------\r\n";
                            send(fd, ok, strlen(ok), 0);
                        } else {
                            char *err =
                                "Sai cu phap!\r\n"
                                "Nhap lai (id:name):\r\n> ";
                            send(fd, err, strlen(err), 0);
                        }
                    } else {
                        broadcast(fd, buffer);
                    }
                }
            }
        }
    }

    return 0;
}