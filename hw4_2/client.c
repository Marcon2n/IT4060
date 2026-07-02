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