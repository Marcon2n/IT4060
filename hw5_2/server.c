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
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024
#define ACCOUNT_FILE "account.txt"
#define MAX_ACCOUNTS 100
#define MAX_SESSIONS 200
#define MAX_FAILED_ATTEMPT 5

// Thông tin 1 tài khoản đọc từ account.txt
typedef struct
{
    char userid[64];
    char password[64];
    int status;      // 0: bi khoa, 1: hoat dong
    int fail_count;  // So lan dang nhap sai lien tiep, reset khi dang nhap thanh cong
} Account;

// Thông tin 1 phiên đăng nhập đang hoạt động (1 kết nối TCP đã login thành công)
typedef struct
{
    int active;
    char userid[64];
    char ip[INET_ADDRSTRLEN];
    time_t login_time;
} Session;

Account accounts[MAX_ACCOUNTS];
int account_count = 0;
pthread_mutex_t accounts_mutex = PTHREAD_MUTEX_INITIALIZER;

Session sessions[MAX_SESSIONS];
pthread_mutex_t sessions_mutex = PTHREAD_MUTEX_INITIALIZER;

// Đọc danh sách tài khoản từ file account.txt vào mảng accounts, chạy 1 lần khi server khởi động
void load_accounts(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("Khong tim thay %s, khoi tao danh sach tai khoan rong\n", filename);
        return;
    }

    while (account_count < MAX_ACCOUNTS &&
           fscanf(fp, "%63s %63s %d", accounts[account_count].userid,
                  accounts[account_count].password, &accounts[account_count].status) == 3)
    {
        accounts[account_count].fail_count = 0;
        account_count++;
    }

    fclose(fp);
    printf("Da nap %d tai khoan tu %s\n", account_count, filename);
}

// Ghi lại toàn bộ danh sách tài khoản xuống file, dùng khi 1 tài khoản bị khóa
// Lưu ý: hàm này giả định accounts_mutex đã được lock bởi hàm gọi
void save_accounts_locked(const char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        perror("Khong the ghi lai file tai khoan");
        return;
    }

    for (int i = 0; i < account_count; i++)
    {
        fprintf(fp, "%s %s %d\n", accounts[i].userid, accounts[i].password, accounts[i].status);
    }

    fclose(fp);
}

// Tìm tài khoản theo userid, trả về con trỏ hoặc NULL nếu không có
// Lưu ý: hàm này giả định accounts_mutex đã được lock bởi hàm gọi
Account *find_account_locked(const char *userid)
{
    for (int i = 0; i < account_count; i++)
    {
        if (strcmp(accounts[i].userid, userid) == 0)
        {
            return &accounts[i];
        }
    }
    return NULL;
}

// Thêm 1 phiên đăng nhập mới vào mảng sessions, trả về chỉ số phiên hoặc -1 nếu đầy
int add_session(const char *userid, const char *ip)
{
    pthread_mutex_lock(&sessions_mutex);

    int idx = -1;
    for (int i = 0; i < MAX_SESSIONS; i++)
    {
        if (!sessions[i].active)
        {
            idx = i;
            break;
        }
    }

    if (idx != -1)
    {
        sessions[idx].active = 1;
        strncpy(sessions[idx].userid, userid, sizeof(sessions[idx].userid) - 1);
        strncpy(sessions[idx].ip, ip, sizeof(sessions[idx].ip) - 1);
        sessions[idx].login_time = time(NULL);
    }

    pthread_mutex_unlock(&sessions_mutex);
    return idx;
}

// Giải phóng phiên đăng nhập khi client đăng xuất hoặc ngắt kết nối
void remove_session(int idx)
{
    if (idx < 0)
        return;

    pthread_mutex_lock(&sessions_mutex);
    sessions[idx].active = 0;
    pthread_mutex_unlock(&sessions_mutex);
}

// Đọc 1 dòng dữ liệu từ socket (kết thúc bởi \n), xử lý stream byte của TCP
// Trả về số byte đọc được (0 nếu client đóng kết nối ngay từ đầu, -1 nếu lỗi)
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

// Gửi 1 dòng dữ liệu, tự thêm ký tự xuống dòng
void send_line(int sk, const char *line)
{
    char buf[BUFFER_SIZE];
    snprintf(buf, sizeof(buf), "%s\n", line);
    send(sk, buf, strlen(buf), 0);
}

// Gửi dòng kết thúc phản hồi để client biết dừng đọc
void send_end(int sk)
{
    send_line(sk, "END");
}

// Tính năng nâng cao 1: liệt kê các kết nối hiện đang đăng nhập cùng 1 tài khoản
void handle_count(int conn_sk, const char *userid)
{
    char line[BUFFER_SIZE];
    int total = 0;

    pthread_mutex_lock(&sessions_mutex);
    for (int i = 0; i < MAX_SESSIONS; i++)
    {
        if (sessions[i].active && strcmp(sessions[i].userid, userid) == 0)
        {
            total++;
        }
    }

    snprintf(line, sizeof(line), "OK|Tai khoan '%s' dang co %d ket noi:", userid, total);
    send_line(conn_sk, line);

    for (int i = 0; i < MAX_SESSIONS; i++)
    {
        if (sessions[i].active && strcmp(sessions[i].userid, userid) == 0)
        {
            char time_buf[64];
            struct tm *tm_info = localtime(&sessions[i].login_time);
            strftime(time_buf, sizeof(time_buf), "%H:%M:%S %d/%m/%Y", tm_info);

            snprintf(line, sizeof(line), "  - IP: %s | Dang nhap luc: %s", sessions[i].ip, time_buf);
            send_line(conn_sk, line);
        }
    }
    pthread_mutex_unlock(&sessions_mutex);

    send_end(conn_sk);
}

// Tính năng nâng cao 2: liệt kê những tài khoản đang online (kèm số kết nối của mỗi tài khoản)
void handle_online(int conn_sk)
{
    char line[BUFFER_SIZE];
    char seen[MAX_SESSIONS][64];
    int seen_count = 0;

    pthread_mutex_lock(&sessions_mutex);

    send_line(conn_sk, "OK|Danh sach tai khoan dang online:");

    for (int i = 0; i < MAX_SESSIONS; i++)
    {
        if (!sessions[i].active)
            continue;

        // Kiểm tra userid này đã liệt kê chưa để tránh trùng lặp
        int already_seen = 0;
        for (int j = 0; j < seen_count; j++)
        {
            if (strcmp(seen[j], sessions[i].userid) == 0)
            {
                already_seen = 1;
                break;
            }
        }
        if (already_seen)
            continue;

        strncpy(seen[seen_count], sessions[i].userid, sizeof(seen[seen_count]) - 1);
        seen[seen_count][sizeof(seen[seen_count]) - 1] = '\0';
        seen_count++;

        int cnt = 0;
        for (int j = 0; j < MAX_SESSIONS; j++)
        {
            if (sessions[j].active && strcmp(sessions[j].userid, sessions[i].userid) == 0)
                cnt++;
        }

        snprintf(line, sizeof(line), "  - %s (%d ket noi)", sessions[i].userid, cnt);
        send_line(conn_sk, line);
    }

    if (seen_count == 0)
    {
        send_line(conn_sk, "  (Khong co tai khoan nao dang online)");
    }

    pthread_mutex_unlock(&sessions_mutex);

    send_end(conn_sk);
}

// Xử lý quá trình đăng nhập: kiểm tra tài khoản/mật khẩu, đếm số lần sai, khóa tài khoản khi vượt quá giới hạn
// Trả về 1 nếu đăng nhập thành công, 0 nếu thất bại
int try_login(int conn_sk, const char *userid, const char *password, const char *client_ip, int *session_idx)
{
    pthread_mutex_lock(&accounts_mutex);

    Account *acc = find_account_locked(userid);
    if (acc == NULL)
    {
        pthread_mutex_unlock(&accounts_mutex);
        send_line(conn_sk, "ERR|Tai khoan khong ton tai");
        send_end(conn_sk);
        return 0;
    }

    if (acc->status == 0)
    {
        pthread_mutex_unlock(&accounts_mutex);
        send_line(conn_sk, "ERR|Tai khoan da bi khoa, vui long lien he quan tri vien");
        send_end(conn_sk);
        return 0;
    }

    if (strcmp(acc->password, password) != 0)
    {
        acc->fail_count++;
        char msg[BUFFER_SIZE];

        if (acc->fail_count >= MAX_FAILED_ATTEMPT)
        {
            acc->status = 0;
            save_accounts_locked(ACCOUNT_FILE);
            snprintf(msg, sizeof(msg), "ERR|Sai mat khau qua %d lan, tai khoan '%s' da bi khoa", MAX_FAILED_ATTEMPT, userid);
        }
        else
        {
            snprintf(msg, sizeof(msg), "ERR|Sai mat khau (lan %d/%d)", acc->fail_count, MAX_FAILED_ATTEMPT);
        }

        pthread_mutex_unlock(&accounts_mutex);
        send_line(conn_sk, msg);
        send_end(conn_sk);
        return 0;
    }

    // Đăng nhập thành công: reset số lần sai
    acc->fail_count = 0;
    pthread_mutex_unlock(&accounts_mutex);

    *session_idx = add_session(userid, client_ip);

    char msg[BUFFER_SIZE];
    snprintf(msg, sizeof(msg), "OK|Dang nhap thanh cong voi tai khoan '%s'", userid);
    send_line(conn_sk, msg);
    send_end(conn_sk);

    return 1;
}

// Hàm chạy trên thread riêng, phục vụ toàn bộ phiên làm việc của 1 client (có thể đăng nhập/đăng xuất nhiều lần)
void *handle_client(void *arg)
{
    int conn_sk = *(int *)arg;
    free(arg);

    struct sockaddr_in peer_addr;
    socklen_t peer_len = sizeof(peer_addr);
    getpeername(conn_sk, (struct sockaddr *)&peer_addr, &peer_len);
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer_addr.sin_addr, client_ip, sizeof(client_ip));

    char line[BUFFER_SIZE];
    int logged_in = 0;
    int session_idx = -1;
    char current_userid[64] = {0};

    while (1)
    {
        int n = recv_line(conn_sk, line, sizeof(line));
        if (n <= 0)
        {
            printf("Client %s da dong ket noi\n", client_ip);
            break;
        }

        if (!logged_in)
        {
            // Giai đoạn chưa đăng nhập: chỉ chấp nhận lệnh LOGIN
            char cmd[16], userid[64], password[64];
            int parsed = sscanf(line, "%15s %63s %63s", cmd, userid, password);

            if (parsed == 3 && strcmp(cmd, "LOGIN") == 0)
            {
                printf("Client %s thu dang nhap voi tai khoan '%s'\n", client_ip, userid);
                if (try_login(conn_sk, userid, password, client_ip, &session_idx))
                {
                    logged_in = 1;
                    strncpy(current_userid, userid, sizeof(current_userid) - 1);
                }
            }
            else
            {
                send_line(conn_sk, "ERR|Vui long dang nhap truoc (LOGIN userid password)");
                send_end(conn_sk);
            }
        }
        else
        {
            // Giai đoạn đã đăng nhập: xử lý menu chức năng
            if (strcmp(line, "COUNT") == 0)
            {
                handle_count(conn_sk, current_userid);
            }
            else if (strcmp(line, "ONLINE") == 0)
            {
                handle_online(conn_sk);
            }
            else if (strcmp(line, "LOGOUT") == 0)
            {
                remove_session(session_idx);
                session_idx = -1;
                logged_in = 0;
                printf("Client %s (%s) da dang xuat\n", client_ip, current_userid);
                memset(current_userid, 0, sizeof(current_userid));

                send_line(conn_sk, "OK|Da dang xuat thanh cong");
                send_end(conn_sk);
            }
            else if (strcmp(line, "EXIT") == 0)
            {
                remove_session(session_idx);
                session_idx = -1;
                send_line(conn_sk, "OK|Tam biet");
                send_end(conn_sk);
                break;
            }
            else
            {
                send_line(conn_sk, "ERR|Lenh khong hop le");
                send_end(conn_sk);
            }
        }
    }

    // Nếu client mất kết nối đột ngột trong lúc vẫn đang đăng nhập, giải phóng phiên
    if (logged_in && session_idx != -1)
    {
        remove_session(session_idx);
    }

    close(conn_sk);
    return NULL;
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

    load_accounts(ACCOUNT_FILE);

    int listen_sk;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    listen_sk = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sk < 0)
    {
        perror("Tao socket that bai");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_sk, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(listen_sk, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind cong that bai");
        return 1;
    }

    if (listen(listen_sk, 10) < 0)
    {
        perror("Mo cong listen that bai");
        return 1;
    }

    printf("Server dang lang nghe tren cong %d\n", port);

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

        printf("Client %s da ket noi\n", inet_ntoa(client_addr.sin_addr));

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, conn_sk) != 0)
        {
            perror("Tao thread that bai");
            close(*conn_sk);
            free(conn_sk);
            continue;
        }
        pthread_detach(tid);
    }

    close(listen_sk);
    return 0;
}
