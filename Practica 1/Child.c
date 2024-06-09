#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#define FILENAME "practica1.txt"

void random_string(char *str, size_t size) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
}

void perform_syscalls() {
    int fd = open(FILENAME, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        exit(1);
    }

    char buffer[9];
    buffer[8] = '\0';
    while (1) {
        int syscall_type = rand() % 3;
        switch (syscall_type) {
            case 0: // Write
                random_string(buffer, 8);
                write(fd, buffer, 8);
                break;
            case 1: // Read
                lseek(fd, 0, SEEK_SET);
                read(fd, buffer, 8);
                break;
            case 2: // Open (reopen the file)
                close(fd);
                fd = open(FILENAME, O_RDWR);
                if (fd == -1) {
                    perror("open");
                    exit(1);
                }
                break;
        }
        sleep(1 + rand() % 3); // random exec time 
    }
}

int main(int argc, char *argv[]) {
    printf("Argumento 1: %s\n", argv[1]);
    printf("Argumento 2: %s\n", argv[2]);

    srand(time(NULL));
    perform_syscalls();

    // Exit code
    exit(1);
}
