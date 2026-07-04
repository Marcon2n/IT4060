#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <endian.h>

#define BUFFER_SIZE 65536
#define UPLOAD_DIR "uploaded_files"
#define MAX_FILENAME 256

static int recv_all(int sock, void *buf, size_t len)
{
    size_t total = 0;
    char *p = (char *)buf;
    while (total < len)
    {
        ssize_t n = recv(sock, p + total, len - total, 0);
        if (n <= 0)
            return -1;
        total += (size_t)n;
    }
    return 0;
}

static int send_all(int sock, const void *buf, size_t len)
{
    size_t total = 0;
    const char *p = (const char *)buf;
    while (total < len)
    {
        ssize_t n = send(sock, p + total, len - total, 0);
        if (n <= 0)
            return -1;
        total += (size_t)n;
    }
    return 0;
}

static void handle_client(int client_fd)
{
    uint32_t name_len_net;
    if (recv_all(client_fd, &name_len_net, sizeof(name_len_net)) < 0)
    {
        close(client_fd);
        return;
    }
    uint32_t name_len = ntohl(name_len_net);
    if (name_len == 0 || name_len > MAX_FILENAME)
    {
        close(client_fd);
        return;
    }

    char *filename = malloc(name_len + 1);
    if (!filename)
    {
        close(client_fd);
        return;
    }
    if (recv_all(client_fd, filename, name_len) < 0)
    {
        free(filename);
        close(client_fd);
        return;
    }
    filename[name_len] = '\0';

    char filepath[MAX_FILENAME + sizeof(UPLOAD_DIR) + 2];
    snprintf(filepath, sizeof(filepath), "%s/%s", UPLOAD_DIR, filename);

    uint8_t status;

    if (access(filepath, F_OK) == 0)
    {
        status = 1;
        send_all(client_fd, &status, sizeof(status));
        printf("[%s] Da ton tai tren server, tu choi.\n", filename);
        free(filename);
        close(client_fd);
        return;
    }

    FILE *fp = fopen(filepath, "wb");
    if (!fp)
    {
        status = 1;
        send_all(client_fd, &status, sizeof(status));
        printf("[%s] Khong tao duoc file tren server.\n", filename);
        free(filename);
        close(client_fd);
        return;
    }

    status = 0;
    if (send_all(client_fd, &status, sizeof(status)) < 0)
    {
        fclose(fp);
        remove(filepath);
        free(filename);
        close(client_fd);
        return;
    }

    uint64_t filesize_net;
    if (recv_all(client_fd, &filesize_net, sizeof(filesize_net)) < 0)
    {
        fclose(fp);
        remove(filepath);
        free(filename);
        close(client_fd);
        return;
    }
    uint64_t filesize = be64toh(filesize_net);

    char *buffer = malloc(BUFFER_SIZE);
    if (!buffer)
    {
        fclose(fp);
        remove(filepath);
        free(filename);
        close(client_fd);
        return;
    }

    uint64_t remaining = filesize;
    int transfer_ok = 1;
    while (remaining > 0)
    {
        size_t chunk = remaining < BUFFER_SIZE ? (size_t)remaining : BUFFER_SIZE;
        if (recv_all(client_fd, buffer, chunk) < 0)
        {
            transfer_ok = 0;
            break;
        }
        if (fwrite(buffer, 1, chunk, fp) != chunk)
        {
            transfer_ok = 0;
            break;
        }
        remaining -= chunk;
    }

    free(buffer);
    fclose(fp);

    uint8_t final_status = transfer_ok ? 0 : 1;
    send_all(client_fd, &final_status, sizeof(final_status));

    if (transfer_ok)
    {
        printf("[%s] Nhan thanh cong (%llu bytes).\n", filename, (unsigned long long)filesize);
    }
    else
    {
        printf("[%s] Nhan that bai, xoa file loi.\n", filename);
        remove(filepath);
    }

    free(filename);
    close(client_fd);
}

int main(int argc, char *argv[])
{
    int port = -1;
    int opt;
    while ((opt = getopt(argc, argv, "p:")) != -1)
    {
        if (opt == 'p')
            port = atoi(optarg);
    }
    if (port <= 0)
    {
        fprintf(stderr, "Su dung: %s -p <port>\n", argv[0]);
        return 1;
    }

    struct stat st;
    if (stat(UPLOAD_DIR, &st) == -1)
    {
        if (mkdir(UPLOAD_DIR, 0755) == -1)
        {
            perror("mkdir");
            return 1;
        }
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket");
        return 1;
    }

    int reuse = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons((uint16_t)port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Server dang lang nghe tren cong %d, thu muc luu file: %s/\n", port, UPLOAD_DIR);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }
        printf("Client ket noi tu %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        handle_client(client_fd);
    }

    close(server_fd);
    return 0;
}
