#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>

struct client_info {
    int fd;
    int registered; // 0: Chưa đăng ký, 1: Đã đăng ký thành công
    char id[32];
    char name[64];
};

void remove_client(struct client_info clients[], int *num, int fd) {
    for (int i = 0; i < *num; i++) {
        if (clients[i].fd == fd) {
            for (int j = i; j < *num - 1; j++) {
                clients[j] = clients[j + 1];
            }
            (*num)--;
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Cu phap: %s <Cổng>\n", argv[0]);
        return 1;
    }
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(atoi(argv[1]));
    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5);
    printf("Chat Server dang lang nghe o cong %s...\n", argv[1]);
    fd_set fdread, fdtest;
    FD_ZERO(&fdread);
    FD_SET(listener, &fdread);
    struct client_info clients[64];
    int num_clients = 0;
    char buf[1024];
    while (1) {
        fdtest = fdread;
        int ret = select(FD_SETSIZE, &fdtest, NULL, NULL, NULL);
        if (ret < 0) break;
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &fdtest)) {
                if (i == listener) {
                    int new_client = accept(listener, NULL, NULL);
                    FD_SET(new_client, &fdread);
                    clients[num_clients].fd = new_client;
                    clients[num_clients].registered = 0;
                    num_clients++;
                    char *msg = "Vui long nhap theo cu phap: client_id: client_name\n";
                    send(new_client, msg, strlen(msg), 0);
                } 
                else {
                    int bytes = recv(i, buf, sizeof(buf) - 1, 0);
                    if (bytes <= 0) {
                        printf(">> Client %d da thoat\n", i);
                        close(i);
                        FD_CLR(i, &fdread);
                        remove_client(clients, &num_clients, i);
                    } else {
                        buf[bytes] = '\0';
                        buf[strcspn(buf, "\r\n")] = 0;
                        int c_idx = -1;
                        for (int j = 0; j < num_clients; j++) {
                            if (clients[j].fd == i) {
                                c_idx = j; break;
                            }
                        }
                        if (clients[c_idx].registered == 0) {
                            char id_tmp[32], name_tmp[64];
                            int parsed = sscanf(buf, "%[^:]: %[^\n]", id_tmp, name_tmp);
                            if (parsed == 2) {
                                strcpy(clients[c_idx].id, id_tmp);
                                strcpy(clients[c_idx].name, name_tmp);
                                clients[c_idx].registered = 1;
                                char *ok_msg = "Dang ky thanh cong! Ban da vao phong chat.\n";
                                send(i, ok_msg, strlen(ok_msg), 0);
                                printf(">> Client %d dang ky: ID=%s, Name=%s\n", i, id_tmp, name_tmp);
                            } else {
                                char *err_msg = "Sai cu phap! Vui long nhap lai: client_id: client_name\n";
                                send(i, err_msg, strlen(err_msg), 0);
                            }
                        } else {
                            char out_msg[2048];
                            time_t t = time(NULL);
                            struct tm *tm_info = localtime(&t);
                            char time_str[32];
                            strftime(time_str, sizeof(time_str), "%Y/%m/%d %I:%M:%S%p", tm_info);
                            sprintf(out_msg, "%s %s: %s\n", time_str, clients[c_idx].id, buf);
                            for (int j = 0; j < num_clients; j++) {
                                if (clients[j].registered == 1 && clients[j].fd != i) {
                                    send(clients[j].fd, out_msg, strlen(out_msg), 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    close(listener);
    return 0;
}