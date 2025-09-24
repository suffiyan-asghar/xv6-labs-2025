#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

void find(char *path, const char *target, int exec_flag, char *argv_exec[]) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type) {
        case T_FILE:
            const char *filename = path;
            for (const char *q = path; *q; q++) {
                if (*q == '/') {
                    filename = q + 1;
                }
            }
            if (strcmp(filename, target) == 0) {
                if (exec_flag) {
                    if (fork() == 0) {
                        char *argv[MAXARG];
                        int i;
                        for (i = 0; argv_exec[i]; i++) {
                            argv[i] = argv_exec[i];
                        }
                        argv[i] = (char *)path;
                        argv[i + 1] = 0;
                        exec(argv[0], argv);
                        fprintf(2, "find: exec failed for %s\n", argv[0]);
                        exit(1);
                    }
                    wait(0);
                } else {
                    printf("%s\n", path);
                }
            }
            break;

        case T_DIR:
            if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
                fprintf(2, "find: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';

            while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                if (de.inum == 0)
                    continue;

                if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                    continue;

                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = '\0';
                
                find(buf, target, exec_flag, argv_exec);
            }
            break;
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    int exec_flag = 0;
    char *argv_exec[MAXARG];
    int i = 0;

    if (argc < 3) {
        fprintf(2, "Usage: find <directory> <filename> [-exec cmd]\n");
        exit(1);
    }

    
    if (argc > 3 && strcmp(argv[3], "-exec") == 0) {
        exec_flag = 1;
        if (argc < 5) {
            fprintf(2, "Usage: find <directory> <filename> -exec <cmd>\n");
            exit(1);
        }
        for (int j = 4; j < argc; j++) {
            argv_exec[i++] = argv[j];
        }
        argv_exec[i] = 0;
    } else if (argc > 3) {
        fprintf(2, "Usage: find <directory> <filename> [-exec cmd]\n");
        exit(1);
    }

    find(argv[1], argv[2], exec_flag, argv_exec);

    exit(0);
}