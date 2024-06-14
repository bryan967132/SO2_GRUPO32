#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <fcntl.h>

#define NUM_CHILDREN 2
#define MAX_LINE_LENGTH 256
#define NUM_SYSCALLS 2

FILE *log_file;
char buffer[MAX_LINE_LENGTH];

void handle_sigint(int sig) {
    int read_syscalls = 0;
    int write_syscalls = 0;

    log_file = fopen("syscalls.log", "r");
    if (log_file == NULL) {
        perror("fopen");
        exit(1);
    }

    while (fgets(buffer, MAX_LINE_LENGTH, log_file) != NULL) {
        if (strstr(buffer, "Read") != NULL) {
            read_syscalls++;
        } else if (strstr(buffer, "Write") != NULL) {
            write_syscalls++;
        }
    }

    printf("Total: %d\n", read_syscalls + write_syscalls);
    printf("Read: %d\n", read_syscalls);
    printf("Write: %d\n", write_syscalls);

    fclose(log_file);
    exit(0);
}

int main() {
    pid_t children[NUM_CHILDREN];
    signal(SIGINT, handle_sigint);

    printf("INICIO\n");

    int start_systemtap = 0;
    int childs = 0;

    for (int i = 0; i < NUM_CHILDREN; i++) {
        if ((children[i] = fork()) == 0) {
            execl("./child.bin", "child.bin", NULL, NULL, NULL);
            perror("execl");
            exit(1);
        } else if (children[i] == -1) {
            perror("fork");
            exit(1);
        } else {
            printf("Proceso hijo creado con PID: %d\n", children[i]);
            childs ++;
        }
    }

    if(childs == NUM_CHILDREN && start_systemtap == 0) {
        // Ejecución de comando systemstap
        char command[256];
        sprintf(command, "sudo stap syscall_monitor.stp %d %d > syscalls.log 2>/dev/null &", children[0], children[1]);
        printf("Ejecutando SystemTap...\n");
        int result = system(command);
        if (result == -1) {
            perror("Error ejecutando systemtap");
            exit(1);
        }
        childs = -1;
        start_systemtap = 1;
    }

    for (int i = 0; i < NUM_CHILDREN; i++) {
        // Esperar a que el hijo se detenga
        waitpid(children[i], NULL, 0);
    }

    return 0;
}
