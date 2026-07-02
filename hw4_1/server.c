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
        // Xâu chứa ký tự khác chữ/số -> trả về thông báo lỗi cho client
        strcpy(output, "Error");
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

    // Khai báo các biến cần sử dụng
    int listen_sk, conn_sk;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE * 2];

    // Tạo socket lắng nghe, khác với UDP tạo luôn socket để kết nối thằng TCP tạo 1 socket để lắng nghe và thực hiện bắt tay để valid client rồi mới bước vào phase trao đổi dữ liệu
    listen_sk = socket(AF_INET, SOCK_STREAM, 0);

    // Tái sử dụng cổng ngay khi restart server, nguyên nhân là do thường TCP connect sẽ keep 1-2 phút để đảm bảo dữ liệu đã được truyền hết, thực hiện ép sử dụng cổng ngay khi ứng dụng được restart lại để tránh lỗi ứng dụng
    int opt = 1;
    setsockopt(listen_sk, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Cấu hình và bind cổng
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;                                        // Phương thức IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;                                // Tương tự chạy trên mọi card mạng
    server_addr.sin_port = htons(port);                                      // Dùng cổng được khai báo tham số
    bind(listen_sk, (struct sockaddr *)&server_addr, sizeof(server_addr)); // Gán cổng nghe vào port

    // Chuyển sang chế độ lắng nghe
    listen(listen_sk, 5); // Giới hạn số lượng client yêu cầu kết nối để tránh bottleneck
    printf("TCP Server dang lang nghe tren cong %d\n", port);

    // Vòng lặp vô tận để lắng nghe và xử lý kết nối với client
    while (1)
    {
        // Tạo kết nối riêng với client khi handshake thành công
        conn_sk = accept(listen_sk, (struct sockaddr *)&client_addr, &client_len);
        printf("Chuyen client vao kenh rieng de trao doi du lieu\n");

        // Vòng lặp socket trao đổi data riêng với client handshake thành công đến khi nào ngắt kết nối
        while (1)
        {
            memset(buffer, 0, BUFFER_SIZE);
            memset(response, 0, sizeof(response));

            // Nhận dữ liệu stream, khác với thằng UDP TCP dùng hàm recv chứ không dùng recvfrom bởi nó đã là kênh riêng và xử lý riêng biết người nhận
            int n = recv(conn_sk, buffer, BUFFER_SIZE - 1, 0);

            // Xử lý kết thúc hội thoại
            if (n <= 0)
            {
                printf("Client muon dong ket noi\n");
                break;
            }

            buffer[n] = '\0';
            printf("[Client gui]: %s\n", buffer);

            // Xử lý xâu nhận được và gửi kết quả trả về client
            process_client_string(buffer, response);
            send(conn_sk, response, strlen(response), 0);
        }

        // Giải phòng socket trao đổi dữ liệu với client cũ
        close(conn_sk);
    }

    close(listen_sk);
    return 0;
}