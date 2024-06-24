#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAX_LINE_LENGTH 256

int i = 0;

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

void process_line(char *line) {
    char call[10], name[20], _[3], month[4], numDay[3], time[9], year[5];
    int pid, size;

    sscanf(line, "%s %d %s %d %s %s %s %s %s", call, &pid, name, &size, _, month, numDay, time, year);

    printf("Llamada: %s, PID: %d, Nombre: %s, Tamaño Segmento: %d, Fecha Hora: %s-%s-%s %s\n", call, pid, name, size, year, monthToNum(month), numDay, time);
}

int main() {
    FILE *file;
    char line[MAX_LINE_LENGTH];
    long last_pos = 0;

    file = fopen("memory_tracker.log", "r");
    if (!file) {
        perror("Error abriendo el archivo");
        return 1;
    }

    fseek(file, 0, SEEK_END); // Puntero en final de archivo

    while (1) {
        last_pos = ftell(file); // Posición actual del puntero

        // Leer nuevas líneas
        while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
            process_line(line); // Procesar líneas
        }

        sleep(1);

        fseek(file, last_pos, SEEK_SET); // Mover puntero al final de archivo
    }

    fclose(file);

    return 0;
}