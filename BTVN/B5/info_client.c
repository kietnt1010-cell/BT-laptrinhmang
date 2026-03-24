
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

struct sinh_vien{
    int MSSV;
    char ten[100];
    char ngay_sinh[40];
    float diem_trung_binh;
};

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("Usage: %s <dia_chi_IP> <cong>\n", argv[0]);
        printf("Example: %s 127.0.0.1 8080\n", argv[0]);
        return 1;
    }

    char* sv_IP = argv[1];
    int sv_port = atoi(argv[2]);

    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(sv_IP);
    addr.sin_port = htons(sv_port);
    
    if (inet_pton(AF_INET, sv_IP, &addr.sin_addr) <= 0) {
        printf("Dia chi IP khong hop le: %s\n", sv_IP);
        close(client);
        return 1;
    }

    printf("Dang ket noi den %s:%d...\n", sv_IP, sv_port);
    if (connect(client, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect() failed");
        close(client);
        return 1;
    }
    printf("Ket noi thanh cong den %s:%d\n", sv_IP, sv_port);   

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        send(client, cwd, sizeof(cwd), 0);
    } else {
        perror("getcwd");
        return 1;
    }

    DIR *dir = opendir(".");
    if (dir == NULL) {
        perror("opendir");
        return EXIT_FAILURE;
    }
    char *files[100];  // Mảng lưu tên file
    char buffer[1024];
    int file_count = 0;
    struct dirent *entry;
    // Đọc từng phần tử trong thư mục
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && 
            strcmp(entry->d_name, "..") != 0) {
            struct stat file_stat;
            if (stat(entry->d_name, &file_stat) < 0) {
                perror("stat");
                continue;
            }
            snprintf(buffer, sizeof(buffer), "%s|%ld bytes\n", entry->d_name, file_stat.st_size);
            files[file_count] = strdup(buffer);  // Copy tên file
            file_count++;
        }
    }
    closedir(dir);

    for (int i = 0; i < file_count; i++) {
        
        send(client, files[i], strlen(files[i]), 0);
        send(client, "\n", 1, 0);
        free(files[i]);  // Giải phóng bộ nhớ đã cấp phát
    }
    close(client);
    return 0;
}