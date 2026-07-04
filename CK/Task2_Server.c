#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define OPCODE_ENCRYPT 0
#define OPCODE_DECRYPT 1
#define OPCODE_DATA 2
#define OPCODE_ERROR 3

#define CHUNK_SIZE 32768
#define TMP_DIR "caesar_tmp"

typedef struct
{
    uint8_t opcode;
    uint16_t length;
    uint8_t *payload; /* cap phat dong bang malloc, NULL neu length == 0 */
} Message;

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

static int send_message(int sock, uint8_t opcode, uint16_t length, const void *payload)
{
    uint8_t header[3];
    uint16_t length_net = htons(length);
    header[0] = opcode;
    memcpy(&header[1], &length_net, 2);

    if (send_all(sock, header, sizeof(header)) < 0)
        return -1;
    if (length > 0 && send_all(sock, payload, length) < 0)
        return -1;
    return 0;
}

/* Cap phat dong msg->payload (goi free() sau khi dung xong). */
static int recv_message(int sock, Message *msg)
{
    uint8_t header[3];
    if (recv_all(sock, header, sizeof(header)) < 0)
        return -1;

    msg->opcode = header[0];
    uint16_t length_net;
    memcpy(&length_net, &header[1], 2);
    msg->length = ntohs(length_net);
    msg->payload = NULL;

    if (msg->length > 0)
    {
        msg->payload = malloc(msg->length);
        if (!msg->payload)
            return -1;
        if (recv_all(sock, msg->payload, msg->length) < 0)
        {
            free(msg->payload);
            msg->payload = NULL;
            return -1;
        }
    }
    return 0;
}

static uint8_t caesar_byte(uint8_t b, uint8_t key, int decrypt)
{
    return decrypt ? (uint8_t)(b - key) : (uint8_t)(b + key);
}

/* Doc tung khoi tu in_path, ma hoa/giai ma, ghi sang out_path. */
static int transform_file(const char *in_path, const char *out_path, uint32_t key, int decrypt)
{
    FILE *in = fopen(in_path, "rb");
    if (!in)
        return -1;

    FILE *out = fopen(out_path, "wb");
    if (!out)
    {
        fclose(in);
        return -1;
    }

    uint8_t k = (uint8_t)(key % 256);
    unsigned char *buffer = malloc(CHUNK_SIZE);
    if (!buffer)
    {
        fclose(in);
        fclose(out);
        return -1;
    }

    int ok = 1;
    size_t n;
    while ((n = fread(buffer, 1, CHUNK_SIZE, in)) > 0)
    {
        for (size_t i = 0; i < n; i++)
        {
            buffer[i] = caesar_byte(buffer[i], k, decrypt);
        }
        if (fwrite(buffer, 1, n, out) != n)
        {
            ok = 0;
            break;
        }
    }
    if (ferror(in))
        ok = 0;

    free(buffer);
    fclose(in);
    fclose(out);
    return ok ? 0 : -1;
}

static void handle_client(int client_fd)
{
    Message msg;
    if (recv_message(client_fd, &msg) < 0)
    {
        close(client_fd);
        return;
    }
    if ((msg.opcode != OPCODE_ENCRYPT && msg.opcode != OPCODE_DECRYPT) || msg.length != 4)
    {
        free(msg.payload);
        send_message(client_fd, OPCODE_ERROR, 0, NULL);
        close(client_fd);
        return;
    }

    int decrypt = (msg.opcode == OPCODE_DECRYPT);
    uint32_t key_net;
    memcpy(&key_net, msg.payload, 4);
    uint32_t key = ntohl(key_net);
    free(msg.payload);

    char tmp_path[64], res_path[64];
    snprintf(tmp_path, sizeof(tmp_path), "%s/tmp_XXXXXX", TMP_DIR);
    snprintf(res_path, sizeof(res_path), "%s/res_XXXXXX", TMP_DIR);

    int tmp_fd = mkstemp(tmp_path);
    if (tmp_fd < 0)
    {
        send_message(client_fd, OPCODE_ERROR, 0, NULL);
        close(client_fd);
        return;
    }

    int recv_ok = 1;
    while (1)
    {
        Message data_msg;
        if (recv_message(client_fd, &data_msg) < 0)
        {
            recv_ok = 0;
            break;
        }
        if (data_msg.opcode != OPCODE_DATA)
        {
            free(data_msg.payload);
            recv_ok = 0;
            break;
        }
        if (data_msg.length == 0)
        {
            free(data_msg.payload);
            break;
        }
        ssize_t written = write(tmp_fd, data_msg.payload, data_msg.length);
        free(data_msg.payload);
        if (written < 0 || (size_t)written != data_msg.length)
        {
            recv_ok = 0;
            break;
        }
    }
    close(tmp_fd);

    if (!recv_ok)
    {
        remove(tmp_path);
        close(client_fd);
        return;
    }

    if (transform_file(tmp_path, res_path, key, decrypt) < 0)
    {
        send_message(client_fd, OPCODE_ERROR, 0, NULL);
        remove(tmp_path);
        remove(res_path);
        close(client_fd);
        return;
    }

    FILE *res = fopen(res_path, "rb");
    if (!res)
    {
        send_message(client_fd, OPCODE_ERROR, 0, NULL);
        remove(tmp_path);
        remove(res_path);
        close(client_fd);
        return;
    }

    unsigned char *buffer = malloc(CHUNK_SIZE);
    int send_ok = (buffer != NULL);
    if (buffer)
    {
        size_t n;
        while ((n = fread(buffer, 1, CHUNK_SIZE, res)) > 0)
        {
            if (send_message(client_fd, OPCODE_DATA, (uint16_t)n, buffer) < 0)
            {
                send_ok = 0;
                break;
            }
        }
        if (ferror(res))
            send_ok = 0;
    }
    free(buffer);
    fclose(res);

    if (send_ok)
    {
        send_message(client_fd, OPCODE_DATA, 0, NULL);
        printf("[pid %d] %s thanh cong (key=%u).\n", getpid(), decrypt ? "Giai ma" : "Ma hoa", key);
    }
    else
    {
        printf("[pid %d] Loi khi gui ket qua ve client.\n", getpid());
    }

    remove(tmp_path);
    remove(res_path);
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
    if (stat(TMP_DIR, &st) == -1)
    {
        if (mkdir(TMP_DIR, 0755) == -1)
        {
            perror("mkdir");
            return 1;
        }
    }

    signal(SIGPIPE, SIG_IGN);

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

    if (listen(server_fd, 16) < 0)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Server dang lang nghe tren cong %d\n", port);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
        {
            if (errno == EINTR)
                continue;
            perror("accept");
            continue;
        }

        printf("Client ket noi tu %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        handle_client(client_fd);
        close(client_fd);
    }

    close(server_fd);
    return 0;
}
