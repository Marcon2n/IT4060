// Bài tập 2. Sửa đổi chương trình ở Bài tập 1 như sau:
//     • Chương trình có thể chạy mà không cần tham số dòng lệnh.
//     • Người dùng sẽ nhập địa chỉ IP hoặc tên miền.
//     • Nếu chuỗi nhập vào là địa chỉ IP hợp lệ, chương trình sẽ hiển thị tên miền chính thức và danh sách tên miền bí danh (nếu có).
//     • Ngược lại, nếu chuỗi được coi là tên miền, chương trình sẽ hiển thị địa chỉ IP chính thức và danh sách địa chỉ IP bí danh.
//     • Trong trường hợp không tìm thấy thông tin, chương trình sẽ hiển thị thông báo giống như ở Bài tập 1.
//     • Người dùng có thể nhập nhiều chuỗi liên tiếp cho đến khi nhập chuỗi rỗng thì chương trình dừng.
// Thêm một số tính năng nâng cao:
//     • Nếu địa chỉ IP đầu vào thuộc loại đặc biệt (loopback, private,…), hiển thị cảnh báo: “special IP address — may not have DNS record”
//     • Hiển thị thêm các thông tin bổ sung như thời gian truy vấn, tên chuẩn (canonical name — CNAME).
//     • Cho phép người dùng nhập nhiều địa chỉ IP / tên miền trên cùng một dòng, sau đó tra cứu từng địa chỉ/ tên miền.
//     • Lưu lại tất cả các truy vấn và kết quả vào một tệp nhật ký (log file).
//     • Cho phép chương trình chạy ở chế độ batch: nếu dòng lệnh có tham số là tên tệp văn bản (ví dụ ./resolver list.txt), chương trình sẽ đọc từng dòng, tra cứu và in kết quả.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <time.h>

#define TYPE_INVALID 0
#define TYPE_IP 1
#define TYPE_DOMAIN 2

#ifndef NI_MAXHOST
#endif
#ifndef NI_NAMEREQD
#endif

char g_input_line[1024];

// Hàm ghi nhật ký vào log.txt
void write_log(const char *message)
{
    FILE *log_file = fopen("log.txt", "a");
    if (log_file != NULL)
    {
        fprintf(log_file, "%s", message);
        fclose(log_file);
    }
}

// Kiểm tra IP đặc biệt (Private, Loopback,...)
int check_special_ip(const char *ip_addr)
{
    int n1, n2, n3, n4;
    if (sscanf(ip_addr, "%d.%d.%d.%d", &n1, &n2, &n3, &n4) == 4)
    {
        if (n1 == 127)
            return 1;
        if (n1 == 10 || (n1 == 192 && n2 == 168) || (n1 == 172 && n2 >= 16 && n2 <= 31))
            return 1;
        if (n1 == 169 && n2 == 254)
            return 1;
        if (n1 >= 224 && n1 <= 239)
            return 1;
    }
    return 0;
}

// Hàm phân loại đầu vào
int classifyInput(const char *str)
{
    int has_letter = 0;
    for (int i = 0; str[i] != '\0'; i++)
    {
        if (isalpha(str[i]))
        {
            has_letter = 1;
            break;
        }
    }
    if (!has_letter)
    {
        struct in_addr dst;
        if (inet_pton(AF_INET, str, &dst) == 1)
            return TYPE_IP;
        return TYPE_INVALID;
    }
    else
    {
        if (strchr(str, '.') != NULL)
            return TYPE_DOMAIN;
        return TYPE_INVALID;
    }
}

// Phân giải tên miền
void resolve_domain(const char *domain)
{
    struct addrinfo hints, *res, *p;
    char ip_str[INET_ADDRSTRLEN];
    char log_buf[2048] = {0};
    char temp[512] = {0};

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    sprintf(temp, "--- Ket qua tra cuu: %s ---\n", domain);
    strcat(log_buf, temp);

    clock_t start = clock();
    int status = getaddrinfo(domain, NULL, &hints, &res);
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;

    if (status != 0)
    {
        printf("Not found information\n\n");
        strcat(log_buf, "Not found information\n\n");
        write_log(log_buf);
        return;
    }

    printf("Query time: %.2f ms\n", time_taken);
    sprintf(temp, "Query time: %.2f ms\n", time_taken);
    strcat(log_buf, temp);

    if (res->ai_canonname != NULL)
    {
        printf("Canonical name (CNAME): %s\n", res->ai_canonname);
        sprintf(temp, "Canonical name (CNAME): %s\n", res->ai_canonname);
        strcat(log_buf, temp);
    }

    struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
    inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, sizeof(ip_str));
    printf("Official IP: %s\n", ip_str);
    sprintf(temp, "Official IP: %s\n", ip_str);
    strcat(log_buf, temp);

    printf("Alias IP:\n");
    strcat(log_buf, "Alias IP:\n");
    for (p = res->ai_next; p != NULL; p = p->ai_next)
    {
        struct sockaddr_in *alias_ipv4 = (struct sockaddr_in *)p->ai_addr;
        char alias_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(alias_ipv4->sin_addr), alias_ip, sizeof(alias_ip));
        if (strcmp(ip_str, alias_ip) != 0)
        {
            printf("%s\n", alias_ip);
            sprintf(temp, "%s\n", alias_ip);
            strcat(log_buf, temp);
        }
    }
    printf("\n");
    strcat(log_buf, "\n");
    write_log(log_buf);

    freeaddrinfo(res);
}

// Phân giải IP
void resolve_ip(const char *ip_addr)
{
    struct sockaddr_in sa;
    char host[NI_MAXHOST];
    char log_buf[2048] = {0};
    char temp[NI_MAXHOST + 128] = {0};

    sprintf(temp, "--- Ket qua tra cuu: %s ---\n", ip_addr);
    strcat(log_buf, temp);

    if (check_special_ip(ip_addr))
    {
        printf("special IP address — may not have DNS record\n");
        strcat(log_buf, "special IP address — may not have DNS record\n");
    }

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    inet_pton(AF_INET, ip_addr, &sa.sin_addr);

    clock_t start = clock();
    int status = getnameinfo((struct sockaddr *)&sa, sizeof(sa), host, sizeof(host), NULL, 0, NI_NAMEREQD);
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;

    if (status != 0)
    {
        printf("Not found information\n\n");
        strcat(log_buf, "Not found information\n\n");
        write_log(log_buf);
        return;
    }

    printf("Query time: %.2f ms\n", time_taken);
    printf("Official name: %s\n", host);
    printf("Alias name:\n\n");

    snprintf(temp, sizeof(temp), "Query time: %.2f ms\nOfficial name: %s\nAlias name:\n\n", time_taken, host);
    strcat(log_buf, temp);
    write_log(log_buf);
}

// Hàm băm nhỏ chuỗi theo khoảng trắng
void process_line(char *line)
{
    line[strcspn(line, "\r\n")] = 0;
    if (strlen(line) == 0)
        return;

    char *token = strtok(line, " \t");
    while (token != NULL)
    {
        int type = classifyInput(token);
        if (type == TYPE_IP)
        {
            resolve_ip(token);
        }
        else if (type == TYPE_DOMAIN)
        {
            resolve_domain(token);
        }
        else
        {
            printf("Result for '%s': Invalid address\n\n", token);
            char log_err[256];
            sprintf(log_err, "Result for '%s': Invalid address\n\n", token);
            write_log(log_err);
        }
        token = strtok(NULL, " \t");
    }
}

int main()
{
    int choice = 0;
    char file_name[256];

    printf("=========================================\n");
    printf("          DNS RESOLVER INTERACTIVE       \n");
    printf("=========================================\n");
    printf("1. Nhap du lieu truc tiep tu ban phim\n");
    printf("2. Doc du lieu tu file van ban (Batch mode)\n");
    printf("Chon che do (1 hoac 2): ");

    if (scanf("%d", &choice) != 1)
    {
        printf("Lua chon khong hop le.\n");
        return 1;
    }

    while (getchar() != '\n')
        ;

    if (choice == 2)
    {
        printf("Nhap ten file van ban (Vi du: list.txt): ");
        if (fgets(file_name, sizeof(file_name), stdin) == NULL)
            return 1;
        file_name[strcspn(file_name, "\r\n")] = 0;

        FILE *batch_file = fopen(file_name, "r");
        if (batch_file == NULL)
        {
            printf("Loi: Khong the mo file '%s'\n", file_name);
            return 1;
        }

        char file_line[1024];
        while (fgets(file_line, sizeof(file_line), batch_file) != NULL)
        {
            if (file_line[0] == '\n' || file_line[0] == '\r')
                continue;
            process_line(file_line);
        }
        fclose(batch_file);
        printf("Da xu ly xong file batch.\n");
    }
    else if (choice == 1)
    {
        printf("Nhap IP/Ten mien (cac thanh phan cach boi space):\n");
        while (1)
        {
            printf("> ");
            if (fgets(g_input_line, sizeof(g_input_line), stdin) == NULL)
                break;

            g_input_line[strcspn(g_input_line, "\r\n")] = 0;
            if (strlen(g_input_line) == 0)
            {
                printf("Dang dung chuong trinh...\n");
                break;
            }
            process_line(g_input_line);
        }
    }
    else
    {
        printf("Lua chon sai. Thoat chuong trinh.\n");
    }

    return 0;
}