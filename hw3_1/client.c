// Yêu cầu nộp bài:
//     • Tạo Makefile để biên dịch
//     • Đóng gói tất cả file mã nguồn và Makefile vào một file nén có tên theo định dạnh HotenSV_MSSV_HW3.zip. Ví dụ: TranNguyenNgoc_20161234_HW3.zip
//     • Nộp bài theo quy định

// Bài 1. Viết ứng dụng mạng sử dụng UDP socket như sau
// Server:
//     • Chạy ở số hiệu cổng bất kỳ theo tham số dòng lệnh theo cú pháp sau:
// ./server PortNumber (Ví dụ ./server 5500)
//     • Nhận một xâu do client gửi lên
//     • Trả lại hai xâu, một xâu chỉ chứa chứa các ký tự chữ cái, một xâu chỉ chứa các ký tự chữ số. Nếu xâu nhận được chứa ký tự không phải là chữ cái hoặc chữ số, gửi lại thông báo lỗi
// Client:
//     • Kết nối tới server. Sử dụng tham số dòng lệnh cho địa chỉ IP và số hiệu cổng của server kết nối tới. Cú pháp:
// ./client IPAddress PortNumber (Ví dụ: ./client 127.0.0.1 5500)
//     • Cho phép người dùng nhập xâu bất kỳ từ bàn phím và gửi cho server
//     • Nhận kết quả từ server và hiển thị
//     • Chức năng lặp lại cho tới khi người dùng nhập xâu rỗng
// Lưu ý: Tạo Makefile với tên file thực thi sau khi biên dịch là server và client
// INPUT
// OUTPUT
// 1a2b3cd
// 123
// abcd
// 123
// 123
// abcd
// abcd
// Ab15CD$
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
    // Kiểm tra tham số truyền vào và hướng dẫn người dùng cần nhập ip và port của server
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
    socklen_t server_len = sizeof(server_addr);

    // 1. Tạo socket bằng UDP bằng hàm socket()
    client_sk = socket(AF_INET, SOCK_DGRAM, 0);
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
    printf("San sang gui du lieu toi [%s:%d]\n", server_ip, server_port);

    // Vòng lặp vô hạn để client tiếp tục có thể giao tiếp với Server
    char input[BUFFER_SIZE];
    while (1)
    {
        printf("> ");
        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        // Xóa ký tự xuống dòng ở cuối chuỗi nhập, cần thực hiện xóa do behavior của hàm fgets đọc được input enter khi hoàn thành thao tác nhập liệu -> đẩy lên Server gây lỗi do thuộc ký tự lạ cần xóa trước khi gửi
        input[strcspn(input, "\r\n")] = '\0';

        // Nhập xâu rỗng thì dừng chương trình
        if (strlen(input) == 0)
        {
            printf("Dang dung chuong trinh...\n");
            break;
        }

        // Gửi nội dung lên Server
        int bytes_sent = sendto(client_sk, input, strlen(input), 0,
                                (struct sockaddr *)&server_addr, server_len);
        if (bytes_sent < 0)
        {
            perror("Loi khi gui du lieu");
            continue;
        }

        // Nhận kết quả phản hồi từ Server
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recvfrom(client_sk, buffer, BUFFER_SIZE - 1, 0,
                                      (struct sockaddr *)&server_addr, &server_len);
        if (bytes_received < 0)
        {
            perror("Loi khi nhan du lieu");
            continue;
        }
        buffer[bytes_received] = '\0';

        // Hiển thị kết quả nhận được từ server
        printf("%s\n", buffer);
    }

    // Đóng socket trước khi kết thúc chương trình
    close(client_sk);
    return 0;
}
