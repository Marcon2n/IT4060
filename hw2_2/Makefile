CC = gcc
CFLAGS = -Wall -D_GNU_SOURCE

# Mục tiêu mặc định dịch duy nhất bài resolver
all: resolver

resolver: resolver.c
	$(CC) $(CFLAGS) resolver.c -o resolver

# Dọn dẹp file chạy và file log cũ
clean:
	rm -f resolver log.txt