// Bài 2 (Tùy chọn) Sử dụng TCP socket để xây dựng ứng dụng đăng nhập, đăng xuất cho người dùng.
//     • Server khởi động với số hiệu cổng là giá trị truyền qua tham số dòng lệnh:
// $./server Port_Number	(Ví dụ: $./server 5500)
//     • Client khởi động với địa chỉ server là các giá trị truyền qua tham số dòng lệnh có cú pháp như sau:
// $./client IP_Addr Port_Number (Ví dụ: $./client 10.0.0.1 5500)
//     • Yêu cầu:
//     • Mỗi cửa sổ client chỉ đăng nhập được 1 tài khoản
//     • Mỗi tài khoản có thể đăng nhập đồng thời trên nhiều cửa sổ
//     • Nếu đăng nhập sai quá 5 lần, tài khoản bị khóa
//     • Tài khoản người dùng lưu trên file văn bản account.txt, mỗi dòng một tài khoản dạng(xem file ví dụ):
// UserID Password Status
// Trong đó Status có giá trị 0: Tài khoản bị khóa, 1: Tài khoản hoạt động
//     • Tính năng nâng cao:
//         ◦ Sau khi đăng nhập, mỗi người dùng có khả năng xem có bao nhiêu kết nối đến cùng 1 tài khoản (như địa chỉ IP, thời gian kết nối, …)
//         ◦ Sau khi đăng nhập, người dùng có tính năng hiển thị những tài khoản nào đang online tại thời điểm yêu cầu

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

// Đọc 1 dòng dữ liệu từ socket (kết thúc bởi \n), xử lý stream byte của TCP
int recv_line(int sk, char *buf, int maxlen)
{
    int i = 0;
    while (i < maxlen - 1)
    {
        char c;
        int n = recv(sk, &c, 1, 0);
        if (n <= 0)
        {
            if (i == 0)
                return n;
            break;
        }
        if (c == '\n')
            break;
        if (c != '\r')
            buf[i++] = c;
    }
    buf[i] = '\0';
    return i;
}

// Gửi 1 dòng lệnh tới server, tự thêm ký tự xuống dòng
int send_line(int sk, const char *line)
{
    char buf[BUFFER_SIZE];
    snprintf(buf, sizeof(buf), "%s\n", line);
    return send(sk, buf, strlen(buf), 0);
}

// Nhận toàn bộ phản hồi nhiều dòng từ server cho tới khi gặp dòng "END", in ra màn hình
// Trả về 1 nếu dòng đầu tiên là OK, 0 nếu là ERR, -1 nếu mất kết nối
int recv_response(int sk)
{
    char line[BUFFER_SIZE];
    int first = 1;
    int result = 0;

    while (1)
    {
        int n = recv_line(sk, line, sizeof(line));
        if (n <= 0)
        {
            printf("Server da dong ket noi\n");
            return -1;
        }

        if (strcmp(line, "END") == 0)
            break;

        if (first)
        {
            first = 0;
            result = (strncmp(line, "OK", 2) == 0) ? 1 : 0;
            // Bỏ tiền tố "OK|" hoặc "ERR|" trước khi hiển thị cho người dùng
            char *sep = strchr(line, '|');
            printf("%s\n", sep != NULL ? sep + 1 : line);
        }
        else
        {
            printf("%s\n", line);
        }
    }

    return result;
}

// Giai đoạn đăng nhập: hỏi tài khoản/mật khẩu cho tới khi thành công hoặc người dùng bỏ qua
int do_login(int client_sk, char *userid_out)
{
    char userid[64], password[64];
    char line[BUFFER_SIZE];

    while (1)
    {
        printf("Nhap UserID (de trong de thoat): ");
        if (fgets(userid, sizeof(userid), stdin) == NULL)
            return 0;
        userid[strcspn(userid, "\r\n")] = '\0';

        if (strlen(userid) == 0)
            return 0;

        printf("Nhap mat khau: ");
        if (fgets(password, sizeof(password), stdin) == NULL)
            return 0;
        password[strcspn(password, "\r\n")] = '\0';

        snprintf(line, sizeof(line), "LOGIN %s %s", userid, password);
        send_line(client_sk, line);

        int result = recv_response(client_sk);
        if (result == 1)
        {
            strncpy(userid_out, userid, 63);
            return 1;
        }
        else if (result == -1)
        {
            return 0;
        }
        // result == 0: dang nhap that bai, quay lai hoi lai tu dau
    }
}

// Hiển thị menu chức năng sau khi đăng nhập thành công và xử lý lựa chọn của người dùng
void menu_loop(int client_sk, const char *userid)
{
    char choice[16];

    while (1)
    {
        printf("\n===== Xin chao %s =====\n", userid);
        printf("1. Xem cac ket noi dang dang nhap cung tai khoan\n");
        printf("2. Xem cac tai khoan dang online\n");
        printf("3. Dang xuat\n");
        printf("4. Thoat chuong trinh\n");
        printf("Lua chon: ");

        if (fgets(choice, sizeof(choice), stdin) == NULL)
            return;
        choice[strcspn(choice, "\r\n")] = '\0';

        if (strcmp(choice, "1") == 0)
        {
            send_line(client_sk, "COUNT");
            if (recv_response(client_sk) == -1)
                return;
        }
        else if (strcmp(choice, "2") == 0)
        {
            send_line(client_sk, "ONLINE");
            if (recv_response(client_sk) == -1)
                return;
        }
        else if (strcmp(choice, "3") == 0)
        {
            send_line(client_sk, "LOGOUT");
            recv_response(client_sk);
            return;
        }
        else if (strcmp(choice, "4") == 0)
        {
            send_line(client_sk, "EXIT");
            recv_response(client_sk);
            exit(0);
        }
        else
        {
            printf("Lua chon khong hop le\n");
        }
    }
}

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

    client_sk = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sk < 0)
    {
        perror("Tao socket khong thanh cong");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "Dia chi IP khong hop le: %s\n", server_ip);
        close(client_sk);
        return 1;
    }

    // Thông báo nếu không kết nối được tới server
    if (connect(client_sk, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Khong the ket noi toi server");
        close(client_sk);
        return 1;
    }
    printf("Ket noi thanh cong toi server [%s:%d]\n", server_ip, server_port);

    // Mỗi cửa sổ client: đăng nhập, dùng menu, đăng xuất, rồi có thể đăng nhập tài khoản khác
    char userid[64];
    while (do_login(client_sk, userid))
    {
        menu_loop(client_sk, userid);
    }

    close(client_sk);
    return 0;
}
