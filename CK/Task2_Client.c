/*
 * Bai 2: Client ma hoa/giai ma file (Caesar) qua server.
 * Cu phap: ./client -a <IP> -p <port>
 *
 * Khuon dang thong diep: Opcode(1B) | Length(2B, network order) | Payload(Length byte)
 *   Opcode: 0 = ma hoa, 1 = giai ma, 2 = truyen du lieu file, 3 = bao loi
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define OPCODE_ENCRYPT 0
#define OPCODE_DECRYPT 1
#define OPCODE_DATA    2
#define OPCODE_ERROR   3

#define CHUNK_SIZE 32768
#define MAX_PATH_LEN 1024

typedef struct {
    uint8_t opcode;
    uint16_t length;
    uint8_t *payload; /* cap phat dong bang malloc, NULL neu length == 0 */
} Message;

static int send_all(int sock, const void *buf, size_t len) {
    size_t total = 0;
    const char *p = (const char *)buf;
    while (total < len) {
        ssize_t n = send(sock, p + total, len - total, 0);
        if (n <= 0) return -1;
        total += (size_t)n;
    }
    return 0;
}

static int recv_all(int sock, void *buf, size_t len) {
    size_t total = 0;
    char *p = (char *)buf;
    while (total < len) {
        ssize_t n = recv(sock, p + total, len - total, 0);
        if (n <= 0) return -1;
        total += (size_t)n;
    }
    return 0;
}

static int send_message(int sock, uint8_t opcode, uint16_t length, const void *payload) {
    uint8_t header[3];
    uint16_t length_net = htons(length);
    header[0] = opcode;
    memcpy(&header[1], &length_net, 2);

    if (send_all(sock, header, sizeof(header)) < 0) return -1;
    if (length > 0 && send_all(sock, payload, length) < 0) return -1;
    return 0;
}

/* Cap phat dong msg->payload (goi free() sau khi dung xong). */
static int recv_message(int sock, Message *msg) {
    uint8_t header[3];
    if (recv_all(sock, header, sizeof(header)) < 0) return -1;

    msg->opcode = header[0];
    uint16_t length_net;
    memcpy(&length_net, &header[1], 2);
    msg->length = ntohs(length_net);
    msg->payload = NULL;

    if (msg->length > 0) {
        msg->payload = malloc(msg->length);
        if (!msg->payload) return -1;
        if (recv_all(sock, msg->payload, msg->length) < 0) {
            free(msg->payload);
            msg->payload = NULL;
            return -1;
        }
    }
    return 0;
}

static void flush_stdin_line(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

int main(int argc, char *argv[]) {
    char *server_ip = NULL;
    int port = -1;
    int opt;

    while ((opt = getopt(argc, argv, "a:p:")) != -1) {
        switch (opt) {
            case 'a': server_ip = optarg; break;
            case 'p': port = atoi(optarg); break;
            default: break;
        }
    }
    if (!server_ip || port <= 0) {
        fprintf(stderr, "Su dung: %s -a <IP> -p <port>\n", argv[0]);
        return 1;
    }

    int choice;
    printf("Chon chuc nang (0: Ma hoa, 1: Giai ma): ");
    if (scanf("%d", &choice) != 1 || (choice != 0 && choice != 1)) {
        fprintf(stderr, "Lua chon khong hop le\n");
        return 1;
    }
    flush_stdin_line();

    long key_input;
    printf("Nhap gia tri khoa (so nguyen khong am): ");
    if (scanf("%ld", &key_input) != 1 || key_input < 0) {
        fprintf(stderr, "Khoa khong hop le\n");
        return 1;
    }
    flush_stdin_line();
    uint32_t key = (uint32_t)key_input;

    char filepath[MAX_PATH_LEN];
    printf("Nhap duong dan file can %s: ", choice == 0 ? "ma hoa" : "giai ma");
    if (!fgets(filepath, sizeof(filepath), stdin)) {
        fprintf(stderr, "Loi doc duong dan file\n");
        return 1;
    }
    filepath[strcspn(filepath, "\n")] = '\0';

    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        perror("Khong mo duoc file");
        return 1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        fclose(fp);
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Dia chi IP khong hop le: %s\n", server_ip);
        fclose(fp);
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        fclose(fp);
        close(sock);
        return 1;
    }

    uint32_t key_net = htonl(key);
    uint8_t request_opcode = (choice == 0) ? OPCODE_ENCRYPT : OPCODE_DECRYPT;
    if (send_message(sock, request_opcode, 4, &key_net) < 0) {
        fprintf(stderr, "Loi gui yeu cau toi server\n");
        fclose(fp);
        close(sock);
        return 1;
    }

    unsigned char *buffer = malloc(CHUNK_SIZE);
    if (!buffer) {
        fprintf(stderr, "Khong cap phat duoc bo nho\n");
        fclose(fp);
        close(sock);
        return 1;
    }

    int send_ok = 1;
    size_t n;
    while ((n = fread(buffer, 1, CHUNK_SIZE, fp)) > 0) {
        if (send_message(sock, OPCODE_DATA, (uint16_t)n, buffer) < 0) {
            send_ok = 0;
            break;
        }
    }
    if (ferror(fp)) send_ok = 0;
    free(buffer);
    fclose(fp);

    if (!send_ok || send_message(sock, OPCODE_DATA, 0, NULL) < 0) {
        fprintf(stderr, "Gui file len server that bai\n");
        close(sock);
        return 1;
    }

    char *base = strrchr(filepath, '/');
    base = base ? base + 1 : filepath;
    char outpath[MAX_PATH_LEN + 32];
    snprintf(outpath, sizeof(outpath), "%s_%s", choice == 0 ? "encrypted" : "decrypted", base);

    FILE *out = fopen(outpath, "wb");
    if (!out) {
        perror("Khong tao duoc file ket qua");
        close(sock);
        return 1;
    }

    int recv_ok = 1;
    int server_error = 0;
    while (1) {
        Message msg;
        if (recv_message(sock, &msg) < 0) {
            recv_ok = 0;
            break;
        }
        if (msg.opcode == OPCODE_ERROR) {
            free(msg.payload);
            server_error = 1;
            break;
        }
        if (msg.opcode != OPCODE_DATA) {
            free(msg.payload);
            recv_ok = 0;
            break;
        }
        if (msg.length == 0) {
            free(msg.payload);
            break;
        }
        size_t written = fwrite(msg.payload, 1, msg.length, out);
        free(msg.payload);
        if (written != msg.length) {
            recv_ok = 0;
            break;
        }
    }
    fclose(out);
    close(sock);

    if (server_error) {
        printf("Server bao loi trong qua trinh xu ly file.\n");
        remove(outpath);
        return 1;
    }
    if (!recv_ok) {
        printf("Loi khi nhan ket qua tu server.\n");
        remove(outpath);
        return 1;
    }

    printf("%s file thanh cong! Ket qua luu tai: %s\n", choice == 0 ? "Ma hoa" : "Giai ma", outpath);
    return 0;
}
