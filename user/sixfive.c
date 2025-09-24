#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

void process_file(int fd) {
    char buf[1];
    char num_buf[64];
    int num_len = 0;
    int prev_is_separator = 1;
    char separators[] = " -\r\t\n./,";

    while (read(fd, buf, 1) == 1) {
        if (strchr(separators, buf[0]) != 0) {
            if (num_len > 0) {
                num_buf[num_len] = '\0';
                int num = atoi(num_buf);
                if (num != 0 && (num % 5 == 0 || num % 6 == 0)) {
                    printf("%d\n", num);
                }
                num_len = 0;
            }
            prev_is_separator = 1;
        } else {
            if (prev_is_separator) {
                num_len = 0;
            }
            if (num_len < sizeof(num_buf) - 1) {
                num_buf[num_len++] = buf[0];
            }
            prev_is_separator = 0;
        }
    }

    if (num_len > 0) {
        num_buf[num_len] = '\0';
        int num = atoi(num_buf);
        if (num != 0 && (num % 5 == 0 || num % 6 == 0)) {
            printf("%d\n", num);
        }
    }
}

int main(int argc, char *argv[]) {
    int i, fd;

    if (argc < 2) {
        fprintf(2, "Usage: sixfive file...\n");
        exit(1);
    }

    for (i = 1; i < argc; i++) {
        if ((fd = open(argv[i], O_RDONLY)) < 0) {
            fprintf(2, "sixfive: cannot open %s\n", argv[i]);
            continue;
        }
        process_file(fd);
        close(fd);
    }
    exit(0);
}