#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

struct client_info {
    int fd;
    int logged_in;
};

void remove_client(struct pollfd fds[], struct client_info clients[], int *nfds, int index) {
    close(fds[index].fd);
    for (int i = index; i < *nfds - 1; i++) {
        fds[i] = fds[i + 1];
        clients[i] = clients[i + 1];
    }
    (*nfds)--;
}

int check_login(char *user, char *pass) {
    FILE *f = fopen("database.txt", "r");
    if (!f) return 0; 
    
    char f_user[64], f_pass[64];
    while (fscanf(f, "%s %s", f_user, f_pass) == 2) {
        if (strcmp(user, f_user) == 0 && strcmp(pass, f_pass) == 0) {
            fclose(f);
            return 1; 
        }
    }
    fclose(f);
    return 0;
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
    printf("Poll Telnet Server dang lang nghe o cong %s...\n", argv[1]);

    struct pollfd fds[64];
    int nfds = 1;
    fds[0].fd = listener;
    fds[0].events = POLLIN;

    struct client_info clients[64];
    char buf[1024];

    while (1) {
        int ret = poll(fds, nfds, -1);
        if (ret < 0) break;
        if (fds[0].revents & POLLIN) {
            int new_client = accept(listener, NULL, NULL);
            fds[nfds].fd = new_client;
            fds[nfds].events = POLLIN;
            clients[nfds].fd = new_client;
            clients[nfds].logged_in = 0;
            nfds++;
            char *msg = "Vui long nhap tai khoan (Cu phap: user pass):\n";
            send(new_client, msg, strlen(msg), 0);
        }
        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & (POLLIN | POLLERR)) {
                int bytes = recv(fds[i].fd, buf, sizeof(buf) - 1, 0);
                if (bytes <= 0) { 
                    remove_client(fds, clients, &nfds, i);
                    i--; 
                } else {
                    buf[bytes] = '\0';
                    buf[strcspn(buf, "\r\n")] = 0;
                    if (clients[i].logged_in == 0) {
                        char user[64], pass[64];
                        if (sscanf(buf, "%s %s", user, pass) == 2 && check_login(user, pass)) {
                            clients[i].logged_in = 1;
                            char *ok_msg = "Dang nhap thanh cong! Ban co th nhap lenh:\n";
                            send(fds[i].fd, ok_msg, strlen(ok_msg), 0);
                        } else {
                            char *err_msg = "Loi dang nhap! Tai khoan hoac mat khau khong dung.\n";
                            send(fds[i].fd, err_msg, strlen(err_msg), 0);
                        }
                    } else {
                        char cmd[1100];
                        sprintf(cmd, "%s > out.txt", buf);
                        system(cmd);
                        FILE *f = fopen("out.txt", "r");
                        if (f) {
                            char out_buf[1024];
                            int has_output = 0;
                            while (fgets(out_buf, sizeof(out_buf), f)) {
                                send(fds[i].fd, out_buf, strlen(out_buf), 0);
                                has_output = 1;
                            }
                            if (!has_output) {
                                send(fds[i].fd, "(Khong co ket qua tra ve)\n", 26, 0);
                            }
                            fclose(f);
                        }
                    }
                }
            }
        }
    }
    close(listener);
    return 0;
}