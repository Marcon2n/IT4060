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
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5500
#define BUFFER_SIZE 1024

int main()
{
    int client_sk;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char input[BUFFER_SIZE];
    long total_bytes_sent = 0; // Tổng số byte đã gửi tới server

    // Tạo socket TCP
    client_sk = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sk < 0)
    {
        perror("Tao socket khong thanh cong");
        return 1;
    }

    // Cấu hình địa chỉ server cần kết nối tới
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "Dia chi IP khong hop le: %s\n", SERVER_IP);
        close(client_sk);
        return 1;
    }

    // Thực hiện bắt tay 3 bước để kết nối tới server, thông báo nếu thất bại
    if (connect(client_sk, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Khong the ket noi toi server");
        close(client_sk);
        return 1;
    }
    printf("Ket noi thanh cong toi server [%s:%d]\n", SERVER_IP, SERVER_PORT);

    while (1)
    {
        printf("Nhap thong diep (q/Q de thoat): ");
        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        // Xóa ký tự xuống dòng do fgets giữ lại khi nhấn Enter
        input[strcspn(input, "\r\n")] = '\0';

        // Gửi thông điệp lên server
        int bytes_sent = send(client_sk, input, strlen(input), 0);
        if (bytes_sent < 0)
        {
            perror("Loi khi gui du lieu");
            break;
        }
        total_bytes_sent += bytes_sent;

        // Người dùng nhập q/Q: ngừng gửi và đóng kết nối
        if (strcmp(input, "q") == 0 || strcmp(input, "Q") == 0)
        {
            printf("Dang ngat ket noi...\n");
            break;
        }

        // Nhận và hiển thị kết quả phản hồi từ server
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_sk, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0)
        {
            printf("Server da dong ket noi\n");
            break;
        }
        buffer[bytes_received] = '\0';

        printf("Phan hoi tu server: %s\n", buffer);
    }

    // Đóng socket trước khi kết thúc chương trình
    close(client_sk);

    printf("Tong so byte da gui toi server: %ld\n", total_bytes_sent);
    return 0;
}
