// Bài 1. Xử lý xâu ký tự:
//     • Server:
//     • Khởi động với số hiệu cổng là giá trị truyền qua tham số dòng lệnh:
// $./server Port_Number	(Ví dụ: $./server 5500)
//     • Nhận xâu do client gửi tới. Tách xâu này thành 2 xâu, một xâu chỉ chứa các ký tự chữ số, một xâu chỉ chứa các ký tự chữ cái.
//     • Gửi 2 xâu kết quả cho client
//     • Nếu xâu nhận được chứa loại ký tự khác, gửi thông báo lỗi cho client
//     •  Client:
//     • Khởi động với địa chỉ server là các giá trị truyền qua tham số dòng lệnh:
// $./client IP_Addr Port_Number (Ví dụ: $./client 10.0.0.1 5500)
//     • Nhận xâu do người dùng nhập từ bàn phím và gửi cho server
//     • Nhận kết quả trả về từ server và hiển thị.
//     • Chức năng lặp lại cho tới khi người dùng nhập vào xâu rỗng
//     • Ứng dụng cần giải quyết vấn đề truyền theo dòng byte của giao thức TCP
// Lưu ý: Tạo Makefile với tên file thực thi sau khi biên dịch là server và client
// INPUT
// OUTPUT
// 1ab23c
// 123
// abc
// 123abc#
// Error

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    // Kiểm tra tham số dòng lệnh: ./client IPAddress PortNumber
    if (argc != 3)
    {
        fprintf(stderr, "Cach dung: %s IPAddress PortNumber\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    int client_sk;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char input[BUFFER_SIZE];

    // 1. Tạo socket bằng TCP bằng hàm socket()
    client_sk = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sk < 0)
    {
        perror("Tao socket khong thanh cong");
        return 1;
    }
    printf("Tao socket thanh cong\n");

    // 2. Cấu hình cấu trúc địa chỉ sockaddr_in của server cần kết nối tới
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "Dia chi IP khong hop le: %s\n", server_ip);
        close(client_sk);
        return 1;
    }

    // 3. Thực hiện bắt tay 3 bước để thiết lập kết nối TCP với server
    if (connect(client_sk, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Ket noi toi server that bai");
        close(client_sk);
        return 1;
    }
    printf("Ket noi thanh cong toi [%s:%d]\n", server_ip, server_port);

    // Vòng lặp cho tới khi người dùng nhập vào xâu rỗng thì ngắt kết nối
    while (1)
    {
        printf("> ");
        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        // Xóa ký tự xuống dòng ở cuối chuỗi nhập do fgets giữ lại \n khi nhấn Enter
        input[strcspn(input, "\r\n")] = '\0';

        // Nhập xâu rỗng thì ngắt kết nối và dừng chương trình
        if (strlen(input) == 0)
        {
            printf("Dang ngat ket noi...\n");
            break;
        }

        // Gửi nội dung lên Server qua kênh TCP đã kết nối
        int bytes_sent = send(client_sk, input, strlen(input), 0);
        if (bytes_sent < 0)
        {
            perror("Loi khi gui du lieu");
            continue;
        }

        // Nhận kết quả phản hồi từ Server
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_sk, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0)
        {
            printf("Server da dong ket noi\n");
            break;
        }
        buffer[bytes_received] = '\0';

        // Hiển thị kết quả nhận được từ server
        printf("%s\n", buffer);
    }

    // Đóng socket trước khi kết thúc chương trình, hoàn tất bắt tay 4 bước đóng kết nối TCP
    close(client_sk);
    return 0;
}
