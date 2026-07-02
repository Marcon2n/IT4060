// Yêu cầu nộp bài:
//     • Tạo Makefile để biên dịch
//     • Đóng gói tất cả file mã nguồn và Makefile vào một file nén có tên theo định dạnh HotenSV_MSSV_HW4.zip. Ví dụ: TranNguyenNgoc_20161234_HW2.zip
//     • Nộp bài theo quy định

// Bài tập 1. Viết ứng dụng cung cấp chức năng tìm kiếm thông tin tên miền cho người dùng. Tên miền hoặc địa chỉ IP được truyền dưới dạng tham số dòng lệnh theo cú pháp sau
// ./resolver parameter
// Trong đó:
//     • Nếu parameter là một địa chỉ IP hợp lệ, chương trình hiển thị tên miền chính thức và danh sách các tên miền phụ có thể phân giải thành địa chỉ này
//     • Ngược lại, coi parameter là tên miền thì hiển thị địa chỉ IP chính thức và danh sách địa chỉ IP phụ tìm kiếm được.
//     • Trong các trường hợp không tìm thấy thông tin, hiển thị thông báo tương ứng cho người dùng.
//     • Trường hợp nhập địa chỉ không đúng định dạng, đưa ra thông báo lỗi
//     • Chương trình có khả năng xử lý các địa chỉ đặc biệt như địa chỉ nội bộ, loopback, multicast hay link-local.
// Lưu ý: Tạo Makefile với tên file thực thi sau khi biên dịch là resolver
// INPUT
// OUTPUT
// ./resolver google.com
// Official IP: 216.58.197.110
// Alias IP:
// 216.58.197.123
// 126.58.99.199
// ./resolver 126.58.99.199
// Official name: hkg07s22-in-f3.1e100.net
// Alias name:
// hkg07s22-in-f99.1e100.net
// ./resolver aznsc.test.com
// Not found information~
// ./resolver 255.12.34.12
// Not found information
// ./resolver 1.2.3
// Invalid address

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

#define TYPE_INVALID 0
#define TYPE_IP 1
#define TYPE_DOMAIN 2

// Hàm phân loại đầu vào
int classifyParam(const char *str)
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

// Hàm phân rã ip
void resolve_ip(const char *ip_addr)
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
        printf("Not found information\n");
        return;
    }

    printf("Official name: %s\n", host);
    printf("Alias name:\n");
}

// Hàm phân rã tên miền
void resolve_domain(const char *domain)
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
        printf("Not found information\n");
        return;
    }

    // Địa chỉ đầu tiên tìm thấy là Official IP
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
    inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, sizeof(ip_str));
    printf("Official IP: %s\n", ip_str);

    // Các địa chỉ còn lại trong danh sách liên kết là Alias IP
    printf("Alias IP:\n");
    for (p = res->ai_next; p != NULL; p = p->ai_next)
    {
        struct sockaddr_in *alias_ipv4 = (struct sockaddr_in *)p->ai_addr;
        char alias_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(alias_ipv4->sin_addr), alias_ip, sizeof(alias_ip));

        // Tránh in trùng lặp lại địa chỉ IP chính thức
        if (strcmp(ip_str, alias_ip) != 0)
        {
            printf("%s\n", alias_ip);
        }
    }

    // Giải phóng bộ nhớ động
    freeaddrinfo(res);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: ./resolver parameter\n");
        return 1;
    }

    char *param = argv[1];

    // Phân loại tham số đầu vào
    int input_type = classifyParam(param);

    // Chia case xử lý
    switch (input_type)
    {
    case TYPE_IP:
        resolve_ip(param);
        break;
    case TYPE_DOMAIN:
        resolve_domain(param);
        break;
    default:
        printf("Invalid address\n");
    }

    return 0;
}
