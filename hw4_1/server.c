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