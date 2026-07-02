// Viết chương trình sử dụng đa luồng (thread) để giải quyết các yêu cầu sau
// Bài 1.  Xây dựng một ứng dụng mạng sử dụng TCP socket như sau:
//     • Server:
//     • Sử dụng địa chỉ IP: 127.0.0.1
//     • Chờ yêu cầu kết nối từ client trên cổng 5500
//     • Nhận thông điệp từ client, chuyển thông điệp sang dạng viết hoa và gửi lại cho client. Nếu nhận được xâu “q” hoặc “Q” thì đóng kết nối.
//     • Client:
//     • Kết nối tới server. Lưu ý: thông báo nếu không kết nối được
//     • Nhận thông điệp người dùng nhập từ bàn phím và gửi cho server.
//     • Hiển thị thông điệp nhận được từ server.
//     • Ngừng gửi và đóng kết nối nếu người dùng nhập xâu “q” hoặc “Q”. Hiển thị số byte đã gửi tới server trước khi kết thúc chương trình