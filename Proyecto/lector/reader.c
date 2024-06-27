#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <mysql/mysql.h>

#define MAX_LINE_LENGTH 256

struct data {
    int pid;
    char nombre[100];
    char tipo[15];
    int tamano;
    char fecha[20];
};

char* monthToNum(char *month) {
    static char month_num[3];

    if (strcmp(month, "Jan") == 0) strcpy(month_num, "01");
    else if (strcmp(month, "Feb") == 0) strcpy(month_num, "02");
    else if (strcmp(month, "Mar") == 0) strcpy(month_num, "03");
    else if (strcmp(month, "Apr") == 0) strcpy(month_num, "04");
    else if (strcmp(month, "May") == 0) strcpy(month_num, "05");
    else if (strcmp(month, "Jun") == 0) strcpy(month_num, "06");
    else if (strcmp(month, "Jul") == 0) strcpy(month_num, "07");
    else if (strcmp(month, "Aug") == 0) strcpy(month_num, "08");
    else if (strcmp(month, "Sep") == 0) strcpy(month_num, "09");
    else if (strcmp(month, "Oct") == 0) strcpy(month_num, "10");
    else if (strcmp(month, "Nov") == 0) strcpy(month_num, "11");
    else if (strcmp(month, "Dec") == 0) strcpy(month_num, "12");
    else strcpy(month_num, "00");

    return month_num;
}

void set_env(const char *filename){
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error abriendo el archivo");
        exit(1);
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;

        char *delimiter = strchr(line, '=');
        if (!delimiter) {
            continue;
        }

        *delimiter = 0;
        char *key = line;
        char *value = delimiter + 1;

        if (setenv(key, value, 1) != 0) {
            perror("Error al establecer variable de entorno");
            exit(1);
        }
    }

    fclose(file);
}

void process_line(char *line, MYSQL *conn) {
    char call[10], name[20], _[3], month[4], numDay[3], time[9], year[5];
    int pid, size;

    sscanf(line, "%s %d %s %d %s %s %s %s %s", call, &pid, name, &size, _, month, numDay, time, year);

    // printf("Llamada: %s, PID: %d, Nombre: %s, Tamaño Segmento: %d, Fecha Hora: %s-%s-%s %s\n", call, pid, name, size, year, monthToNum(month), numDay, time);

    struct data data;
    data.pid = pid;
    strcpy(data.nombre, name);
    strcpy(data.tipo, call);
    data.tamano = size;
    sprintf(data.fecha, "%s-%s-%s %s", year, monthToNum(month), numDay, time);

    char query[500];
    sprintf(query, "INSERT INTO Memoria (pid, nombre, tipo, tamano, fecha) VALUES (%d, '%s', '%s', %d, '%s')", data.pid, data.nombre, data.tipo, data.tamano, data.fecha);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
    } else {
        printf("Llamada: %s, PID: %d, Nombre: %s, Tamaño Segmento: %d, Fecha Hora: %s-%s-%s %s\n", call, pid, name, size, year, monthToNum(month), numDay, time);
    }
}

int main() {
    set_env("../.env");
    // Conexión a base de datos
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    char *server = getenv("MYSQL_HOST");
    char *user = getenv("MYSQL_USER");
    char *password = getenv("MYSQL_PASS");
    char *database = getenv("MYSQL_DB");

    conn = mysql_init(NULL);

    if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        return 1;
    }

    // Iniciar systemtap
    int result = system("sudo stap memory_tracker.stp > memory_tracker.log 2>/dev/null &");
    if (result == -1) {
        perror("system");
        return 1;
    }

    sleep(1);

    // Leer archivo
    FILE *file;
    char line[MAX_LINE_LENGTH];
    long last_pos = 0;

    file = fopen("memory_tracker.log", "r");
    if (!file) {
        perror("Error abriendo el archivo");
        return 1;
    }

    // fseek(file, 0, SEEK_END); // Puntero en final de archivo

    while (1) {
        last_pos = ftell(file); // Posición actual del puntero

        // Leer nuevas líneas
        while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
            // Procesar línea y guardar en base de datos
            process_line(line, conn);
        }

        sleep(1);

        fseek(file, last_pos, SEEK_SET); // Mover puntero al final de archivo
    }

    fclose(file);

    return 0;
}