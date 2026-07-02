// Bài 2. Viết ứng dụng để truyền file từ client lên server. File có định dạng bất kỳ và kích thước file có thể lên tới 100MB
//     • Server:
//     • Khởi động với số hiệu cổng là giá trị truyền qua tham số dòng lệnh:
// $./server Port_Number		(Ví dụ: $./server 5500)
//     • Sử dụng một thư mục chung để lưu file của người dùng gửi lên
//     • Nếu tên file client gửi lên trùng với file đã có trong thư mục, báo lỗi.
//     •  Client:
//     • Khởi động với địa chỉ server là các giá trị truyền qua tham số dòng lệnh:
// $./client IP_Addr Port_Number (Ví dụ: $./client 10.0.0.1 5500)
//     • Nhận đường dẫn file do người dùng nhập từ bàn phím
//     • Gửi file lên cho server
//     • Hiển thị kết quả truyền file
//     • Chức năng lặp lại cho tới khi người dùng nhập vào đường dẫn file là xâu rỗng

// Lưu ý: Tạo Makefile với tên file thực thi sau khi biên dịch là server và client
// INPUT
// OUTPUT
// D:/test/testfile.jpg
// Successful transfering
// D:/test/testfile.rar
// Error: File not found
// D:/test/testfile.jpg
// Error: File is existent on server
// D:/test/testfile.avi
// Error: File tranfering is interupted

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// Cùng kích thước block với server để hai bên gửi/nhận ăn khớp nhau
#define BUFFER_SIZE 4096

// Lấy phần tên file (bỏ đường dẫn thư mục cha) từ đường dẫn đầy đủ người dùng nhập
// vì server chỉ lưu file phẳng trong UPLOAD_DIR, không tạo lại cây thư mục con
const char *get_filename_from_path(const char *path)
{
    const char *slash = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');
    const char *last = slash;
    if (backslash != NULL && (last == NULL || backslash > last))
    {
        last = backslash;
    }
    return last != NULL ? last + 1 : path;
}

int main(int argc, char *argv[])
{
    // Kiểm tra tham số dòng lệnh
    if (argc != 3)
    {
        fprintf(stderr, "Cach dung: %s IP_Addr Port_Number\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    // Tạo socket và kết nối tới server, dùng chung 1 kết nối cho toàn bộ phiên upload
    int sock_fd;
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Tao socket that bai");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "Dia chi IP khong hop le: %s\n", server_ip);
        close(sock_fd);
        return 1;
    }

    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Ket noi toi server that bai");
        close(sock_fd);
        return 1;
    }

    printf("Da ket noi toi server %s:%d\n", server_ip, port);

    char input_path[BUFFER_SIZE];
    char reply[64];
    char buffer[BUFFER_SIZE];

    // Vòng lặp chính: hỏi đường dẫn file, dừng khi người dùng nhập chuỗi rỗng
    while (1)
    {
        printf("Nhap duong dan file can upload (Enter de ket thuc): ");
        if (fgets(input_path, sizeof(input_path), stdin) == NULL)
        {
            break; // EOF tren stdin, coi nhu nguoi dung muon dung
        }

        // Xóa ký tự xuống dòng do fgets giữ lại
        input_path[strcspn(input_path, "\r\n")] = 0;

        // Người dùng nhập chuỗi rỗng -> dừng vòng lặp, đóng kết nối
        if (strlen(input_path) == 0)
        {
            break;
        }

        // Mở file local để kiểm tra tồn tại và lấy kích thước
        FILE *fp = fopen(input_path, "rb");
        if (fp == NULL)
        {
            printf("Error: File not found\n");
            continue;
        }

        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        // Bước 1: gửi tên file (không kèm đường dẫn) để server kiểm tra trùng tên
        const char *filename = get_filename_from_path(input_path);
        send(sock_fd, filename, strlen(filename), 0);

        memset(reply, 0, sizeof(reply));
        int n = recv(sock_fd, reply, sizeof(reply) - 1, 0);
        if (n <= 0)
        {
            printf("Error: File tranfering is interupted\n");
            fclose(fp);
            break;
        }

        if (strncmp(reply, "EXIST", 5) == 0)
        {
            printf("Error: File is existent on server\n");
            fclose(fp);
            continue;
        }

        // Bước 2: gửi kích thước file để server kiểm tra dung lượng ổ đĩa
        send(sock_fd, &file_size, sizeof(file_size), 0);

        memset(reply, 0, sizeof(reply));
        n = recv(sock_fd, reply, sizeof(reply) - 1, 0);
        if (n <= 0)
        {
            printf("Error: File tranfering is interupted\n");
            fclose(fp);
            break;
        }

        if (strncmp(reply, "DISK_FULL", 9) == 0)
        {
            printf("Error: Server disk is full\n");
            fclose(fp);
            continue;
        }

        // Bước 3: gửi dữ liệu file theo từng block BUFFER_SIZE cho tới hết file
        int send_failed = 0;
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0)
        {
            size_t total_sent = 0;
            while (total_sent < bytes_read)
            {
                ssize_t sent = send(sock_fd, buffer + total_sent, bytes_read - total_sent, 0);
                if (sent <= 0)
                {
                    send_failed = 1;
                    break;
                }
                total_sent += (size_t)sent;
            }
            if (send_failed)
            {
                break;
            }
        }

        fclose(fp);

        if (send_failed)
        {
            printf("Error: File tranfering is interupted\n");
            continue;
        }

        // Chờ kết quả cuối cùng từ server sau khi gửi xong toàn bộ file
        memset(reply, 0, sizeof(reply));
        n = recv(sock_fd, reply, sizeof(reply) - 1, 0);
        if (n <= 0 || strncmp(reply, "SUCCESS", 7) != 0)
        {
            printf("Error: File tranfering is interupted\n");
        }
        else
        {
            printf("Successful transfering\n");
        }
    }

    close(sock_fd);
    return 0;
}
