*Universidad de San Carlos de Guatemala*  
*Escuela de Ingeniería en Ciencias y Sistemas, Facultad de Ingenieria*  
*Sistema Operativos 2, Junio 2024.*  

___
## **Practica 1**
### **Monitoreo de llamadas del sistema con SystemTap**
___
**201944994 - Robin Omar Buezo Díaz**

**201908355 - Danny Hugo Bryan Tejaxún Pichiyá**

**201907774 - Dyllan José Rodrigo García Mejía**

#### **Resumen**
Esta práctica fue realizada con el objetivo de conocer acerta de los procesos del sistema, sus procesos hijos y la manera poder monitorear cada uno de estos utilizando herramientas como Systemtap.

El sistema resultante consiste en un proceso principal que crea dos procesos hijos, los cuales realizan operaciones de apertura, lectura y escritura sobre un archivo en común, luego estás operaciones son monitoreadas por medio de Systemap y al finalizar el proceso padre se muestra un resumen de las llamadas realizadas por los procesos hijos.
___
#### **Introducción**  
El presente documento tiene como finalidad describir los puntos importantes del desarrollo del sistema para poder comprender de mejor forma su estructura y funcionamiento a nivel de código.  

Se busca profundizar en los puntos claves del funcionamiento del sistema como la creación de los procesos hijos, el script de Systemtap que se encarga de monitorear los procesos hijos y la captura del comando de finalización del proceso padre.
___
___
#### **Proceso padre**  
Es el encargado de dirigir y monitorear las actividades de los dos procesos hijo que crea. Este proceso debe ser capaz de interceptar y registrar todas las llamadas de sistema realizadas por los procesos hijo, almacenando esta información en un archivo de log utlizando la herramienta de rastreo y sondeo "SystemTap". Además, hace un recuento de las llamadas de cada tipo que realizan los hijos ya sean de Escritura o Lectura.

##### *Codigo Principal*

~~~c
#define NUM_CHILDREN 2
#define MAX_LINE_LENGTH 256
#define NUM_SYSCALLS 2

FILE *log_file;
char buffer[MAX_LINE_LENGTH];

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
~~~

##### *Función de captura del comando de finalización del proceso*
Esta función se encarga de capturar el comando *Ctrl + C* de consola y cambiar la acción por defecto.

~~~c
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
~~~

#### **Procesos hijo**
Estos procesos son encargados de realizar diferentes operaciones de apertura, lectura y escritura sobre un archivo en común, cada una de las operaciones de lectura y escritura son de 8 caracteres aleatorios. Cada una de las llamadas se realiza en un tiempo aleatorio entre 1 y 3 segundos

##### *Codigo Principal*

~~~c
int main(int argc, char *argv[]) {
    printf("Soy el hijo %d\n", getpid());
    srand(time(NULL));
    perform_syscalls();

    // Exit code
    exit(1);
}
~~~

##### *Función returno de caracteres aleatorios*
Esta función se encarga de generar los caracteres aleatorios.

~~~c
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
~~~

##### *Función de generación de acciones aleatorias*
Esta función se encarga de generar acciones aleatorias para los procesos hijos.

~~~c
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
~~~

#### **Systemtap**
Este sistema es utilizado para monitorear las llamadas realizadas por los procesos hijos y escribir dichas operaciones en un archivo log en donde se guarda el pid del proceso, la acción realizada y la fecha y la hora de la acción.

~~~c
#!/usr/bin/stap

probe syscall.read {
    if(pid() == $1 || pid() == $2){
        printf("Proceso %d : Read (%s)\n", pid(), tz_ctime(gettimeofday_s()));
    }
}

probe syscall.write {
    if(pid() == $1 || pid() == $2){
        printf("Proceso %d : Write (%s)\n", pid(), tz_ctime(gettimeofday_s()));
    }
}
~~~