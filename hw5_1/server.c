// Viết chương trình sử dụng đa luồng (thread) để giải quyết các yêu cầu sau
// Bài 1.  Xây dựng một ứng dụng mạng sử dụng TCP socket như sau:
//     • Server:
//     • Sử dụng địa chỉ IP: 127.0.0.1
//     • Chờ yêu cầu kết nối từ client trên cổng 5500
//     • Nhận thông điệp từ client, chuyển thông điệp sang dạng viết hoa và gửi lại cho client. Nếu nhận được xâu “q” hoặc “Q” thì đóng kết nối.
//     • Client:
//     • Kết nối tới server. Lưu ý: thông báo nếu không kết nối được
//     • Nhận thông điệp người dùng nhập từ bàn phím và gửi cho server.
//     • Hiển thị thông điệp nhận được từ server.
//     • Ngừng gửi và đóng kết nối nếu người dùng nhập xâu “q” hoặc “Q”. Hiển thị số byte đã gửi tới server trước khi kết thúc chương trình

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5500
#define BUFFER_SIZE 1024

// Hàm chạy trên thread riêng, phục vụ trao đổi dữ liệu với 1 client
void *handle_client(void *arg)
{
    int conn_sk = *(int *)arg;
    free(arg); // Đã lấy được giá trị socket, giải phóng vùng nhớ cấp phát tạm

    char buffer[BUFFER_SIZE];

    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int n = recv(conn_sk, buffer, BUFFER_SIZE - 1, 0);

        if (n <= 0)
        {
            printf("[Thread %lu] Client da dong ket noi\n", pthread_self());
            break;
        }

        buffer[n] = '\0';
        // Xóa ký tự xuống dòng dư thừa nếu client gửi kèm \n
        buffer[strcspn(buffer, "\r\n")] = '\0';

        printf("[Thread %lu] Nhan tu client: %s\n", pthread_self(), buffer);

        // Client yêu cầu kết thúc phiên làm việc
        if (strcmp(buffer, "q") == 0 || strcmp(buffer, "Q") == 0)
        {
            printf("[Thread %lu] Client yeu cau dong ket noi\n", pthread_self());
            break;
        }

        // Chuyển thông điệp sang chữ hoa
        for (int i = 0; buffer[i] != '\0'; i++)
        {
            buffer[i] = toupper((unsigned char)buffer[i]);
        }

        send(conn_sk, buffer, strlen(buffer), 0);
    }

    close(conn_sk);
    return NULL;
}

int main()
{
    int listen_sk;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Tạo socket lắng nghe TCP
    listen_sk = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sk < 0)
    {
        perror("Tao socket that bai");
        return 1;
    }

    // Cho phép tái sử dụng cổng ngay khi restart server
    int opt = 1;
    setsockopt(listen_sk, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Cấu hình địa chỉ: bind cố định vào 127.0.0.1:5500 theo yêu cầu đề bài
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "Dia chi IP khong hop le: %s\n", SERVER_IP);
        close(listen_sk);
        return 1;
    }

    if (bind(listen_sk, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind cong that bai");
        close(listen_sk);
        return 1;
    }

    if (listen(listen_sk, 5) < 0)
    {
        perror("Mo cong listen that bai");
        close(listen_sk);
        return 1;
    }

    printf("TCP Server dang lang nghe tai %s:%d\n", SERVER_IP, SERVER_PORT);

    // Vòng lặp vô tận: mỗi client kết nối thành công được giao cho 1 thread riêng xử lý
    while (1)
    {
        int *conn_sk = malloc(sizeof(int));
        *conn_sk = accept(listen_sk, (struct sockaddr *)&client_addr, &client_len);

        if (*conn_sk < 0)
        {
            perror("Accept ket noi that bai");
            free(conn_sk);
            continue;
        }

        printf("Client %s:%d da ket noi\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pthread_t tid;
        // Tạo thread mới để phục vụ client này, không chặn vòng lặp accept
        if (pthread_create(&tid, NULL, handle_client, conn_sk) != 0)
        {
            perror("Tao thread that bai");
            close(*conn_sk);
            free(conn_sk);
            continue;
        }

        // Detach để thread tự giải phóng tài nguyên khi kết thúc, không cần join
        pthread_detach(tid);
    }

    close(listen_sk);
    return 0;
}
