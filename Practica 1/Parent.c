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

int syscall_count = 0;
int syscall_type_count[1024] = {0}; // Asumimos un máximo de 1024 tipos de llamadas al sistema
FILE *log_file;

void handle_sigint(int sig) {
    printf("Número total de llamadas al sistema: %d\n", syscall_count);
    for (int i = 0; i < 1024; i++) {
        if (syscall_type_count[i] > 0) {
            printf("Syscall %d: %d\n", i, syscall_type_count[i]);
        }
    }
    fclose(log_file);
    exit(0);
}

void log_syscall(pid_t pid, int syscall) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(log_file, "Proceso %d: syscall %d (%04d-%02d-%02d %02d:%02d:%02d)\n", pid, syscall,
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    fflush(log_file);
    syscall_count++;
    syscall_type_count[syscall]++;
}

void trace_child(pid_t child_pid) {
    int status;
    printf("Esperando a proceso hijo con PID: %d\n", child_pid);
    ptrace(PTRACE_SYSCALL, child_pid, 0, 0); // Empezar la traza
    while (1) {
        waitpid(child_pid, &status, 0);
        if (WIFEXITED(status)) {
            printf("Proceso hijo con PID %d terminó\n", child_pid);
            break;
        }
        if (WIFSTOPPED(status)) {
            struct user_regs_struct regs;
            if (ptrace(PTRACE_GETREGS, child_pid, 0, &regs) == -1) {
                perror("ptrace GETREGS");
                exit(1);
            }
            printf("Proceso %d realizó syscall %lld\n", child_pid, regs.orig_rax);
            log_syscall(child_pid, regs.orig_rax);
        }
        if (ptrace(PTRACE_SYSCALL, child_pid, 0, 0) == -1) {
            perror("ptrace SYSCALL");
            exit(1);
        }
    }
}

int main() {
    pid_t children[NUM_CHILDREN];
    signal(SIGINT, handle_sigint);
    log_file = fopen("syscalls.log", "w");

    if (log_file == NULL) {
        perror("No se pudo abrir el archivo syscalls.log");
        exit(1);
    }

    printf("INICIO\n");

    int start_systemtap = 0;
    int childs = 0;

    for (int i = 0; i < NUM_CHILDREN; i++) {
        if ((children[i] = fork()) == 0) {
            ptrace(PTRACE_TRACEME, 0, 0, 0);
            kill(getpid(), SIGSTOP); // Esperar hasta que el padre esté listo para rastrear
            execl("./child.bin", "child.bin", "Hola", "Soy el Proceso Hijo!", NULL);
            perror("execl");
            exit(1);
        } else if (children[i] == -1) {
            perror("fork");
            exit(1);
        } else {
            printf("Proceso hijo creado con PID: %d\n", children[i]);
            childs ++;
        }
        if(childs == NUM_CHILDREN && start_systemtap == 0) {
            // Ejecución de comando systemstap
            char command[256];
            sprintf(command, "sudo stap syscall_monitor.stp %d %d > log.log 2>/dev/null &", children[0], children[1]);
            printf("Ejecutando: %s\n", command);
            int result = system(command);
            if (result == -1) {
                perror("Error ejecutando systemtap");
                exit(1);
            }
            childs = -1;
            start_systemtap = 1;
        }
    }

    for (int i = 0; i < NUM_CHILDREN; i++) {
        // Esperar a que el hijo se detenga
        waitpid(children[i], NULL, 0);
        ptrace(PTRACE_SYSCALL, children[i], 0, 0); // Empezar la traza
        trace_child(children[i]);
    }

    fclose(log_file);
    return 0;
}
