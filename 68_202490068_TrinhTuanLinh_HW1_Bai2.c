// Cho hai cấu trúc sau:
// typedef struct point{
// double x;
// double y;
// } point_t;
// typedef struct circle{
// point_t center;
// double radius;
// } circle_t;
// Viết hàm is_in_circle trả về 1 nếu điểm p nằm trong đường tròn c.
// int is_in_circle(point_t *p, circle_t *c);
// Kiểm tra hàm bằng một chương trình.
// Input:
//     • Dòng thứ nhất: 3 số thực biểu diễn tọa độ x, y và bán kính của tâm đường tròn
//     • Dòng thứ hai: 2 số thực biểu diễn tọa độ x, y của điểm p
// 0 0 5.5
// 2.3 -3.1
// Output:
// YES nếu p nằm trong đường tròn
// NO nếu p không nằm trong đường tròn (p nằm trên hoặc nằm ngoài đường tròn)

#include <stdio.h>

// Khai báo struct của điểm
typedef struct point
{
    double x;
    double y;
} point_t;

// Khai báo struct của hình tròn
typedef struct circle
{
    point_t center;
    double radius;
} circle_t;

// Hàm check xem điểm p có nằm trong hình tròn c hay không
int is_in_circle(point_t *p, circle_t *c)
{
    // Tỉnh khoảng cách trục x y của điểm đến tâm đường tròn
    double dx = p->x - c->center.x;
    double dy = p->y - c->center.y;

    // So sánh bình phương của công thức khoảng cách với bình phương bán kính
    double binh_phuong_khoang_cach = dx * dx + dy * dy;
    double binh_phuong_ban_kinh = c->radius * c->radius;
    return (binh_phuong_khoang_cach < binh_phuong_ban_kinh);
}

int main()
{
    point_t p;
    circle_t c;

    // Nhập tọa độ tâm và bán kính của hình tròn
    printf("Nhap toa do tam va ban kinh hinh tron: ");
    if (scanf("%lf %lf %lf", &c.center.x, &c.center.y, &c.radius) != 3)
        return 1;

    // Nhập tọa độ điểm p
    printf("Nhap toa do diem p: ");
    if (scanf("%lf %lf", &p.x, &p.y) != 2)
        return 1;

    // Kiểm tra và in kết quả
    if (is_in_circle(&p, &c))
    {
        printf("YES\n");
    }
    else
    {
        printf("NO\n");
    }

    return 0;
}