// Bài 2. Viết ứng dụng phân giải tên miền dùng UDP socket:
// Server:
//     • Chạy ở số hiệu cổng bất kỳ dùng tham số dòng lệnh theo cú pháp sau:
// ./server PortNumber (Ví dụ ./server 5500)
//     • Nhận một xâu chứa tên miền hoặc địa chỉ IP do client gửi lên
//     • Trả lại xâu chứa tên miền hoặc địa chỉ IP cho client
// Client:
//     • Kết nối tới server. Sử dụng tham số dòng lệnh cho địa chỉ IP và số hiệu cổng của server kết nối tới. Cú pháp:
// ./client IPAddress PortNumber (Ví dụ: ./client 127.0.0.1 5500)
//     • Cho phép người dùng nhập vào từ bàn phím tên miền hoặc địa chỉ IP nào đó
//     • Nhận kết quả từ server và hiển thị
//     • Chức năng lặp lại cho tới khi người dùng nhập vào một xâu rỗng
// Lưu ý: Tạo Makefile với tên file thực thi sau khi biên dịch là server và client
// INPUT
// OUTPUT
// google.com
// Official IP: 216.58.197.110
// Alias IP:
// 216.58.197.123
// 126.58.99.199
// 126.58.99.199
// Official name: hkg07s22-in-f3.1e100.net
// Alias name:
// hkg07s22-in-f99.1e100.net
// aznsc.test.com
// Not found information
// 259.12.34.12
// IP Address is invalid

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <ctype.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define TYPE_INVALID 0
#define TYPE_IP 1
#define TYPE_DOMAIN 2

// Hàm phân loại input từ người dùng
int classifyInput(const char *str)
{
    int has_letter = 0;

    // Kiểm tra xem có chữ cái không nếu có thì không phải là ip
    for (int i = 0; str[i] != '\0'; i++)
    {
        // Nếu có chữ cái thì dừng vòng lặp và cập nhật has_letter
        if (isalpha(str[i]))
        {
            has_letter = 1;
            break;
        }
    }

    // Hàm kiểm tra chính
    if (!has_letter)
    {
        // Dùng pton kiểm tra ip có hợp lệ không
        struct in_addr dst;
        if (inet_pton(AF_INET, str, &dst) == 1)
        {
            return TYPE_IP;
        }
        else
        {
            return TYPE_INVALID;
        }
    }
    else
    {
        // Kiểm tra nếu có text và dấu chấm thì tạm coi là domain để  tiến hành phân rã
        if (strchr(str, '.') != NULL)
        {
            return TYPE_DOMAIN;
        }
        return TYPE_INVALID;
    }
}

// Hàm phân rã ip, ghi kết quả vào buffer output để gửi trả về client
void resolve_ip(const char *ip_addr, char *output)
{
    struct sockaddr_in sa;
    char host[NI_MAXHOST];

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;

    // Dịch chuỗi IP sang số nhị phân cấu trúc mạng
    inet_pton(AF_INET, ip_addr, &sa.sin_addr);

    // getnameinfo với cờ NI_NAMEREQD để bắt trường hợp không lấy được tên miền tương ứng
    int status = getnameinfo((struct sockaddr *)&sa, sizeof(sa), host, sizeof(host), NULL, 0, NI_NAMEREQD);

    if (status != 0)
    {
        strcpy(output, "Not found information\n");
        return;
    }

    sprintf(output, "Official name: %s\nAlias name:\n", host);
}

// Hàm phân rã tên miền, ghi kết quả vào buffer output để gửi trả về client
void resolve_domain(const char *domain, char *output)
{
    struct addrinfo hints, *res, *p;
    char ip_str[INET_ADDRSTRLEN];
    int status;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // getaddrinfo tự động liên hệ DNS để check xem tên miền có tồn tại thật hay không
    if ((status = getaddrinfo(domain, NULL, &hints, &res)) != 0)
    {
        strcpy(output, "Not found information\n");
        return;
    }

    // Địa chỉ đầu tiên tìm thấy là Official IP
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
    inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, sizeof(ip_str));
    sprintf(output, "Official IP: %s\n", ip_str);

    // Các địa chỉ còn lại trong danh sách liên kết là Alias IP
    strcat(output, "Alias IP:\n");
    for (p = res->ai_next; p != NULL; p = p->ai_next)
    {
        struct sockaddr_in *alias_ipv4 = (struct sockaddr_in *)p->ai_addr;
        char alias_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(alias_ipv4->sin_addr), alias_ip, sizeof(alias_ip));

        // Tránh in trùng lặp lại địa chỉ IP chính thức
        if (strcmp(ip_str, alias_ip) != 0)
        {
            strcat(output, alias_ip);
            strcat(output, "\n");
        }
    }

    // Giải phóng bộ nhớ động
    freeaddrinfo(res);
}

// Hàm xử lý chính phân rã tên miền theo loại và gửi phản hồi cho người dùng
void process_client_string(const char *input, char *output)
{
    output[0] = '\0';

    int type = classifyInput(input);

    switch (type)
    {
    case TYPE_IP:
        resolve_ip(input, output);
        break;
    case TYPE_DOMAIN:
        resolve_domain(input, output);
        break;
    default:
        strcpy(output, "IP Address is invalid\n");
        break;
    }
}

// Hàm chính của chương trình sử dụng code của bài 1 HW_3
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