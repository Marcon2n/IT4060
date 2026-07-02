// Nội dung yêu cầu
// Xây dựng và chạy minh họa (demo) hàm sau
// char* subStr(char* s1, intoffset, int number);
// Hàm tách xâu con từ xâu s1 bắt đầu từ ký tự tại chỉ số offset (tính từ 0) và có độ dài number ký tự.
// Chú ý kiểm tra tính hợp lệ của các đối số. Trong trường hợp giá trị number lớn hơn độ dài phần còn lại của xâu s1 tính từ vị trí offset, trả về xâu con là phần còn lại của s1 tính từ vị trí offset.
// Lưu ý:
//     • Không dùng mảng ký tự tĩnh
//     • Không sử dụng các hàm xử lý xâu có sẵn trong thư viện string.h
// Input:
//     • Dòng thứ nhất: Hai số nguyên biểu diễn offset và number
//     • Dòng thứ hai: Xâu str (tối đa 80 ký tự)
// 6 3
// hust.edu.vn
// Output:
// Xâu con vừa tách, bao bọc bởi dấu '-'
// -edu-

#include <stdio.h>
#include <stdlib.h>

// Xây dựng hàm đếm len của chuỗi ký tự
int str_len(char *str)
{
    // Tạo một con trỏ để chạy thực hiện việc đếm só lượng phần tử trong chuỗi
    char *p = str;
    // Vòng lặp cho con trỏ chạy đến khi gặp giá trị 0
    while (*p)
        p++;
    // Trả về độ dài của chuỗi bằng chênh lệch vị trí của p và str ban đầu
    return p - str;

    // Tại sao phải phức tạp sử dụng 2 con trỏ để đếm độ dài chuỗi, lý do nếu để con trỏ str chạy đếm khi kết thúc đếm nó sẽ ở vị trí cuối cụ thể là 0 trong chuối thì khi đó việc tính toán vị trí và cắt chuối với con trỏ str sẽ phức tạp hơn
}

// Kiểm tra phần tử bắt đầu có hợp lệ không
int is_valid_offset(char *str, int offset, int number)
{
    int len = str_len(str);
    if (offset < 0 || offset >= len || number < 0)
    {
        return 0;
    }
    return 1;
}

// Hàm tách chuối theo yêu cầu
char *subStr(char *str, int offset, int number)
{
    int len = str_len(str); // Đếm len

    if (!is_valid_offset(str, offset, number))
    {
        printf("Phan tu bat dau khong hop le");
        return NULL;
    } // Kiểm tra offset hợp lệ

    int remain = len - offset;

    if (number > remain)
    {
        number = remain;
    }

    // Tạo chuỗi có độ dài +1 để lưu kết quả
    char *result = (char *)malloc(number + 1);
    if (!result)
    {
        return NULL;
    }

    // Sao chép nội dung vào chuỗi kết quả
    char *p = result;
    // Chuyển con trỏ từ đầu chuỗi đến vị trí offset
    str += offset;

    // Chạy ngược để sao chép nội dung gán giá trị tăng dần theo str
    while (number--)
    {
        *p++ = *str++;
    }
    // Gán phần tử cuối cho chuỗi là 0 để đánh dấu kết thúc chuỗi
    *p = 0;
    return result;
}

int main()
{
    int offset, number;
    // Khai báo chuỗi 1000 ký tự để tránh crash chương trình
    char str[1000];
    int len;

    // Vòng lặp nhập liệu đảm bảo valid input
    printf("Nhap offset va number: ");
    if (scanf("%d %d", &offset, &number) != 2)
        return 1;
    do
    {
        printf("Nhap chuoi (TOI DA 80 ky tu): ");
        scanf(" %[^\n]", str);

        len = str_len(str);

        if (len > 80)
        {
            printf("Loi: Chuoi ban vua nhap dai %d ky tu, da vuot qua gioi han 80! Vui long nhap lai.\n", len);
        }
    } while (len > 80);

    // Chạy hàm in ra kết quả
    char *sub = subStr(str, offset, number);
    if (sub)
    {
        printf("Ket qua tach chuoi '-'\n");
        printf("-%s-\n", sub);
        free(sub);
    }
    return 0;
}