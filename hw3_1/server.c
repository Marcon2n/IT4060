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
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

// Hàm xử lý văn bản gửi từ client
void process_client_string(const char *input, char *output)
{
    char letters[BUFFER_SIZE] = {0};
    char digits[BUFFER_SIZE] = {0};
    int l_idx = 0, d_idx = 0;
    int is_error = 0;

    for (int i = 0; input[i] != '\0'; i++)
    {
        if (isalpha(input[i]))
        {
            letters[l_idx++] = input[i];
        }
        else if (isdigit(input[i]))
        {
            digits[d_idx++] = input[i];
        }
        else
        {
            // Thấy ký tự đặc biệt hoặc khoảng trắng -> Đánh dấu lỗi và dừng quét
            is_error = 1;
            break;
        }
    }

    if (is_error)
    {
        strcpy(output, "Van ban co ky tu khong phai la chu hoac so. Vui long chi nhap chu va so");
    }
    else
    {
        // Gộp chuỗi số và chuỗi chữ, phân tách bằng dấu xuống dòng \n
        sprintf(output, "%s\n%s", digits, letters);
    }
}

int main(int argc, char *argv[])
{
    // Kiểm tra tham số dòng lệnh: ./server PortNumber
    if (argc != 2)
    {
        fprintf(stderr, "Cach dung: %s PortNumber\n", argv[0]);
        return 1;
    }
    int port = atoi(argv[1]);

    int server_sk;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE * 2];
    socklen_t client_len = sizeof(client_addr);

    // 1. Tạo socket bằng UDP bằng hàm socket()
    // AF_INET: IPv4 | SOCK_DGRAM: Kết nối giao thức socket bằng UDP
    server_sk = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sk < 0)
    {
        perror("Tao socket khong thanh cong");
        return 1;
    }
    printf("Tao socket thanh cong\n");

    // 2. Cấu hình cấu trúc địa chỉ sockaddr_in cho server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;         // Thiếp lập giao thức IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; // Lắng nghe ở tất cả card mạng
    server_addr.sin_port = htons(port);       // Chuyển đổi Port sang định dạng mạng (Big Endian)
    printf("Cau hinh socket thanh cong o port %d\n", port);

    // 3. Gán socket vào cổng khai báo;
    if (bind(server_sk, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Gan cong that bai");
        close(server_sk);
        return 1;
    }
    printf("Gan ket socket vao cong %d thanh cong.\n", port);
    printf("Cho client gui tin nhan...\n");

    // VÒNG LẶP VÔ HẠN: Duy trì Server luôn chạy để nhận tin từ Client
    while (1)
    {
        // Xóa sạch bộ đệm trước khi nhận dữ liệu mới
        memset(buffer, 0, BUFFER_SIZE);
        memset(response, 0, sizeof(response));

        // Chờ nhận dữ liệu từ client sử dụng hàm recvfrom
        int bytes_received = recvfrom(server_sk, buffer, BUFFER_SIZE - 1, 0,
                                      (struct sockaddr *)&client_addr, &client_len);
        if (bytes_received < 0)
        {
            perror("Loi khi nhan du lieu");
            continue; // Bỏ qua nếu lỗi để tiếp tục nhận gói tin
        }

        // Chuyển đổi IP của Client vừa gửi sang dạng chuỗi chữ để in ra màn hình giám sát
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_addr.sin_port);

        // Đảm bảo chuỗi nhận được kết thúc bằng ký tự null
        buffer[bytes_received] = '\0';
        printf("[Log] Nhan tu [%s:%d]: %s\n", client_ip, client_port, buffer);

        // Biến đổi chuổi response theo yêu cầu của đề bài
        process_client_string(buffer, response);

        // Sử dụng chính biến client_addr vừa hứng được ở trên để trả dữ liệu response về
        int bytes_sent = sendto(server_sk, response, strlen(response), 0,
                                (struct sockaddr *)&client_addr, client_len);
        if (bytes_sent < 0)
        {
            perror("Loi khi gui du lieu phan hoi");
        }
        else
        {
            printf("Da gui tra ket qua ve [%s:%d]\n\n", client_ip, client_port);
        }
    }

    // Đóng socket trước khi kết thúc chương trình
    close(server_sk);
    return 0;
}