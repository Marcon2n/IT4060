#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <endian.h>

#define BUFFER_SIZE 65536
#define MAX_PATH 1024

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

int main(int argc, char *argv[])
{
    char *server_ip = NULL;
    int port = -1;
    int opt;

    while ((opt = getopt(argc, argv, "a:p:")) != -1)
    {
        switch (opt)
        {
        case 'a':
            server_ip = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        default:
            break;
        }
    }
    if (!server_ip || port <= 0)
    {
        fprintf(stderr, "Su dung: %s -a <IP> -p <port>\n", argv[0]);
        return 1;
    }

    char filepath[MAX_PATH];
    printf("Nhap duong dan file can gui: ");
    if (!fgets(filepath, sizeof(filepath), stdin))
    {
        fprintf(stderr, "Loi doc duong dan file\n");
        return 1;
    }
    filepath[strcspn(filepath, "\n")] = '\0';

    FILE *fp = fopen(filepath, "rb");
    if (!fp)
    {
        perror("Khong mo duoc file");
        return 1;
    }

    if (fseek(fp, 0, SEEK_END) != 0)
    {
        fprintf(stderr, "Khong xac dinh duoc kich thuoc file\n");
        fclose(fp);
        return 1;
    }
    long filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (filesize < 0)
    {
        fprintf(stderr, "Khong xac dinh duoc kich thuoc file\n");
        fclose(fp);
        return 1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        fclose(fp);
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "Dia chi IP khong hop le: %s\n", server_ip);
        fclose(fp);
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        fclose(fp);
        close(sock);
        return 1;
    }

    char *base = strrchr(filepath, '/');
    base = base ? base + 1 : filepath;

    uint32_t name_len = (uint32_t)strlen(base);
    uint32_t name_len_net = htonl(name_len);

    if (send_all(sock, &name_len_net, sizeof(name_len_net)) < 0 ||
        send_all(sock, base, name_len) < 0)
    {
        fprintf(stderr, "Loi gui ten file toi server\n");
        fclose(fp);
        close(sock);
        return 1;
    }

    uint8_t status;
    if (recv_all(sock, &status, sizeof(status)) < 0)
    {
        fprintf(stderr, "Mat ket noi voi server\n");
        fclose(fp);
        close(sock);
        return 1;
    }

    if (status != 0)
    {
        printf("Loi: File '%s' da ton tai tren server. Vui long doi ten khac.\n", base);
        fclose(fp);
        close(sock);
        return 0;
    }

    uint64_t filesize_net = htobe64((uint64_t)filesize);
    if (send_all(sock, &filesize_net, sizeof(filesize_net)) < 0)
    {
        fprintf(stderr, "Loi gui kich thuoc file\n");
        fclose(fp);
        close(sock);
        return 1;
    }

    char *buffer = malloc(BUFFER_SIZE);
    if (!buffer)
    {
        fprintf(stderr, "Khong cap phat duoc bo nho\n");
        fclose(fp);
        close(sock);
        return 1;
    }

    long remaining = filesize;
    int transfer_ok = 1;
    while (remaining > 0)
    {
        size_t chunk = remaining < BUFFER_SIZE ? (size_t)remaining : BUFFER_SIZE;
        size_t n = fread(buffer, 1, chunk, fp);
        if (n != chunk || send_all(sock, buffer, chunk) < 0)
        {
            transfer_ok = 0;
            break;
        }
        remaining -= chunk;
    }

    free(buffer);
    fclose(fp);

    if (!transfer_ok)
    {
        printf("Gui file that bai (loi doc/gui du lieu).\n");
        close(sock);
        return 1;
    }

    uint8_t final_status;
    if (recv_all(sock, &final_status, sizeof(final_status)) < 0)
    {
        printf("Mat ket noi, khong nhan duoc xac nhan tu server.\n");
        close(sock);
        return 1;
    }

    if (final_status == 0)
    {
        printf("Gui file '%s' (%ld bytes) thanh cong!\n", base, filesize);
    }
    else
    {
        printf("Server bao loi khi luu file.\n");
    }

    close(sock);
    return 0;
}
