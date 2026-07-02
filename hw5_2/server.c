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