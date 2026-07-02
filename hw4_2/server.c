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
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>

// Lý do tại sao chọn 4096 là do trong một page hoặc một block của ổ cứng thường quy định 4Kb nên việc truyền 1 block lên sẽ tối ưu cho việc nhận như ghi dữ liệu xuống ổ cứng
#define BUFFER_SIZE 4096
// Folder để lưu trữ file được đẩy lên server
#define UPLOAD_DIR "uploads/"

int main(int argc, char *argv[])
{
    // Kiểm tra tham số dòng lệnh: ./server PortNumber
    if (argc != 2)
    {
        fprintf(stderr, "Cach dung: %s PortNumber\n", argv[0]);
        return 1;
    };

    // Khai báo các biến sử dụng trong chương trình
    int port = atoi(argv[1]);
    int listen_sk, conn_sk;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    struct stat st = {0};

    // Kiểm tra có folder lưu file chưa chưa có thì tạo mới
    if (stat(UPLOAD_DIR, &st) == -1)
    {
        // Tiến hành tạo thư mục nếu chưa có
        if (mkdir(UPLOAD_DIR, 0777) == 0)
        {
            printf("Tao moi thu muc luu file client thanh cong\n");
        }
        else
        {
            // Báo lỗi không tạo được thư mục và dừng chương trình
            perror("Loi tao thu muc luu tru dung chuong trinh\n");
            return 1;
        }
    }
    else
    {
        printf("Da co thu muc luu tru tiep tuc chuong trinh\n");
    };

    // Tạo socket lắng nghe trên cổng nhập
    if ((listen_sk = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Tao socket that bai");
        return 1;
    };

    // Chống kẹt time để có thể restart chạy được luôn với port cần sử dụng
    int opt = 1;
    setsockopt(listen_sk, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Cấu hình địa chỉ và bind cổng như các bài trước
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(listen_sk, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind cong that bai");
        return 1;
    };

    // Mở cổng lắng nghe
    if (listen(listen_sk, 5) < 0)
    {
        perror("Mo cong listen that bai");
        return 1;
    }
    printf("Server dang chay listen tren cong %d\n", port);

    // Vòng lặp ngoài luôn lắng nghe client mới kết nối đến
    while (1)
    {
        conn_sk = accept(listen_sk, (struct sockaddr *)&client_addr, &client_len);
        if (conn_sk < 0)
        {
            perror("Accept ket noi that bai");
            continue;
        }
        printf("Client %s da ket noi toi server\n", inet_ntoa(client_addr.sin_addr));

        // Vòng lặp trong: 1 client có thể gửi nhiều file cho tới khi ngắt ket noi hoặc gửi tên file rỗng
        while (1)
        {
            // Bước 1 nhận tên file thực hiện kiểm tra trùng tên
            memset(buffer, 0, BUFFER_SIZE);
            int n = recv(conn_sk, buffer, BUFFER_SIZE - 1, 0);

            if (n <= 0)
            {
                printf("Client dung ket noi o buoc gui ten file...\n");
                break;
            }

            // Xóa ký tự xuống dòng dư thừa nếu có
            buffer[strcspn(buffer, "\r\n")] = 0;

            // Client gửi tên file rỗng nghĩa là đã dừng gửi, kết thúc phiên
            if (strlen(buffer) == 0)
            {
                printf("Client bao ket thuc phien upload\n");
                break;
            }

            printf("Client muon upload filename %s\n", buffer);

            // Tạo đường dẫn đầy đủ: uploads/tên_file
            char file_path[BUFFER_SIZE + sizeof(UPLOAD_DIR)];
            snprintf(file_path, sizeof(file_path), "%s%s", UPLOAD_DIR, buffer);

            // Kiểm tra xem file đã tồn tại trên đĩa của Server chưa
            if (access(file_path, F_OK) == 0)
            {
                printf("File đã tồn tại trên Server.\n");
                send(conn_sk, "EXIST", 5, 0);
                continue; // Quay lên đầu vòng lặp trong, đợi Client gửi tên file khác
            }
            else
            {
                send(conn_sk, "READY", 5, 0); // Bước 1 thành công
            }

            // Bước 2 nhận dung lượng file kiểm tra valid và còn dung lượng trên ổ cứng không
            long file_size = 0;
            n = recv(conn_sk, &file_size, sizeof(file_size), 0);

            if (n <= 0)
            {
                printf("Client dung ket noi o buoc gui kich thuoc file...\n");
                break;
            }

            printf("Client upload file voi dung luong %ld bytes (~%.2f MB)\n", file_size, (double)file_size / (1024 * 1024));

            // Kiểm tra dung lượng ổ cứng
            struct statvfs disk_stat;
            long free_bytes = 0;
            if (statvfs(UPLOAD_DIR, &disk_stat) == 0)
            {
                free_bytes = disk_stat.f_bsize * disk_stat.f_bavail;
            }

            // Nếu dung lượng file lớn hơn dung lượng trống của ổ đĩa
            if (file_size <= 0 || file_size > free_bytes)
            {
                printf("Disk full bao Clinet khong ghi duoc file %ld bytes.\n", free_bytes);
                send(conn_sk, "DISK_FULL", 9, 0);
                continue; // Quay lên đầu vòng lặp trong, hủy lượt truyền file này
            }
            else
            {
                send(conn_sk, "READY_TO_RECEIVE", 16, 0); // Bước 2 thành công
            }

            // Bước 3 thực hiện nhận stream byte của file và tiến hành ghi vào folder
            FILE *fp = fopen(file_path, "wb"); // Mở file mới ghi dạng nhị phân
            if (fp == NULL)
            {
                perror("Tao file that bai");
                send(conn_sk, "INTERRUPT", 9, 0);
                continue;
            }

            long bytes_received_total = 0;
            int is_interrupted = 0;

            printf("Bat dau tien hanh nhan va ghi file...\n");

            // Chạy vòng lặp hứng dòng byte cho tới khi nhận đủ file_size
            while (bytes_received_total < file_size)
            {
                long remaining = file_size - bytes_received_total;
                size_t to_read = remaining < (long)BUFFER_SIZE ? (size_t)remaining : BUFFER_SIZE;

                // Nhận cuốn chiếu tối đa 4KB từ mạng
                int bytes_read = recv(conn_sk, buffer, to_read, 0);
                if (bytes_read <= 0)
                {
                    is_interrupted = 1; // Dừng lại và báo lỗi nếu nhận
                    break;
                }

                // Ghi nối đuôi lượng byte vừa nhận vào ổ cứng
                size_t written = fwrite(buffer, 1, bytes_read, fp);
                if (written < (size_t)bytes_read)
                {
                    is_interrupted = 1; // Dừng lại khi bị gián đoạn giữa chừng
                    break;
                }

                bytes_received_total += bytes_read;
            }

            fclose(fp); // Đóng file lại để lưu thay đổi

            // Xử lý khi ngừng giữa chừng hoặc dung lượng file thấp hơn khai báo
            if (is_interrupted || bytes_received_total < file_size)
            {
                printf("Loi trong qua trinh upload file, thuc hien xoa file rac\n");
                send(conn_sk, "INTERRUPT", 9, 0);
                unlink(file_path); // Lệnh xóa
            }
            else
            {
                printf("Upload file thanh cong\n");
                send(conn_sk, "SUCCESS", 7, 0);
            }
        }

        close(conn_sk); // Kết thúc phục vụ khách này, đóng phòng họp để đón khách mới
    }

    close(listen_sk); // Đóng cổng tổng khi tắt hẳn chương trình
    return 0;
}
