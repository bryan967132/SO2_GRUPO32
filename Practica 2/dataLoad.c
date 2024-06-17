#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <jansson.h>  // Biblioteca para manejar JSON

#define MAX_USERS 1000
#define MAX_OPERATIONS 1000

typedef struct {
    int linea;
    int no_cuenta;
    char nombre[100];
    double saldo;
} Usuario;

typedef struct {
    int operacion; // 1: Deposito, 2: Retiro, 3: Transferencia
    int cuenta1;
    int cuenta2; // Solo usado en transferencias
    double monto;
} Operacion;

typedef struct {
    int hilos[3];
    char errores[1000][100];
} ReporteUsuarios;

int errorCargaU = 0;
ReporteUsuarios r1 = { .hilos = {0, 0, 0}, .errores = {0} };

typedef struct {
    int hilos[4];
    int operaciones[3];
    char errores[1000][100];
} ReporteOperaciones;

int errorCargaO = 0;
ReporteOperaciones r2 = { .hilos = {0, 0, 0, 0}, .operaciones = {0, 0, 0}, .errores = {0} };

Usuario usuarios[MAX_USERS];
Usuario usuariosTmp[MAX_USERS];
Operacion operaciones[MAX_OPERATIONS];
int num_usuarios = 0;
int num_operaciones = 0;
int lineas_usuarios = 0;
int lineas_operaciones = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int procesado[3];

// Declaraciones de funciones
char* fecha_hora(char* formato);
void* cargar_usuarios(void* filename);
void* cargar_operaciones(void* filename);
void deposito(int no_cuenta, double monto, int index, int esCarga);
void retiro(int no_cuenta, double monto, int index, int esCarga);
void transferencia(int cuenta_origen, int cuenta_destino, double monto, int index, int esCarga);
void consultar_cuenta(int no_cuenta);

// Función para formatear fecha y hora
char* fecha_hora(char* formato) {
    static char datetime[29];
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    strftime(datetime, sizeof(datetime), formato, local);
    return datetime;
}

// Función para cargar usuarios desde un archivo JSON
void* cargar_usuarios(void* i) {
    const char* file = "usuarios.json";
    json_error_t error;
    json_t *root = json_load_file(file, 0, &error);
    if (!root) {
        fprintf(stderr, "Error al cargar JSON: %s\n", error.text);
        return NULL;
    }

    size_t index;
    json_t *user;
    int thread_id = *(int*) i;
    int start = thread_id == 0 ? 0 : (thread_id == 1 ? procesado[0] : procesado[0] + procesado[1]);
    int end = thread_id == 0 ? procesado[0] : (thread_id == 1 ? procesado[0] + procesado[1] : procesado[0] + procesado[1] + procesado[2]);
    for (index = start; index < end && (user = json_array_get(root, index)); index++) {
        int no_cuenta = json_integer_value(json_object_get(user, "no_cuenta"));
        const char* nombre = json_string_value(json_object_get(user, "nombre"));
        double saldo = json_real_value(json_object_get(user, "saldo"));

        lineas_usuarios++;
        int duplicado = 0;
        for(int i = 0; i < lineas_usuarios; i ++) {
            if(usuarios[i].no_cuenta == no_cuenta) {
                sprintf(r1.errores[errorCargaU], "    - Linea #%d: Número de cuenta duplicado %d", lineas_usuarios, no_cuenta);
                errorCargaU++;
                duplicado = 1;
                break;
            }
        }
        if(no_cuenta <= 0) {
            sprintf(r1.errores[errorCargaU], "    - Linea #%d: Número de cuenta no puede ser menor que 0", lineas_usuarios);
            errorCargaU++;
            continue;
        } else if(saldo < 0) {
            sprintf(r1.errores[errorCargaU], "    - Linea #%d: Saldo no puede ser menor que 0", lineas_usuarios);
            errorCargaU++;
            continue;
        }

        if(!duplicado) {
            // Agregar usuario al arreglo
            usuarios[num_usuarios].no_cuenta = no_cuenta;
            strncpy(usuarios[num_usuarios].nombre, nombre, sizeof(usuarios[num_usuarios].nombre) - 1);
            usuarios[num_usuarios].saldo = saldo;
            num_usuarios++;
            r1.hilos[thread_id]++;
        }
    }
    return NULL;
}

// Función para realizar operaciones desde un archivo JSON
void* cargar_operaciones(void* arg) {
    const char* file = "operaciones.json";
    json_error_t error;
    json_t *root = json_load_file(file, 0, &error);
    if (!root) {
        fprintf(stderr, "Error al cargar JSON: %s\n", error.text);
        return NULL;
    }

    size_t index;
    json_t *operation;
    int thread_id = *(int*) arg;
    for (index = thread_id; index < json_array_size(root) && (operation = json_array_get(root, index)); index+=4) {
        pthread_mutex_lock(&mutex);
        if (num_operaciones >= MAX_OPERATIONS) {
            printf("Error: Demasiadas operaciones\n");
            pthread_mutex_unlock(&mutex);
            break;
        }

        operaciones[num_operaciones].operacion = json_integer_value(json_object_get(operation, "operacion"));
        operaciones[num_operaciones].cuenta1 = json_integer_value(json_object_get(operation, "cuenta1"));
        operaciones[num_operaciones].cuenta2 = json_integer_value(json_object_get(operation, "cuenta2"));
        operaciones[num_operaciones].monto = json_real_value(json_object_get(operation, "monto"));

        int operacion = operaciones[num_operaciones].operacion;
        int cuenta1 = operaciones[num_operaciones].cuenta1;
        int cuenta2 = operaciones[num_operaciones].cuenta2;
        double monto = operaciones[num_operaciones].monto;

        lineas_operaciones ++;

        switch (operacion) {
            case 1:
                deposito(cuenta1, monto, thread_id, 1);
                break;
            case 2:
                retiro(cuenta1, monto, thread_id, 1);
                break;
            case 3:
                transferencia(cuenta1, cuenta2, monto, thread_id, 1);
                break;
            default:
                printf("Operación desconocida: %d\n", operacion);
                break;
        }

        num_operaciones++;
        pthread_mutex_unlock(&mutex);
    }
    // printf("termina: %s\n", filename);
    return NULL;
}

// Funciones para las operaciones individuales
void deposito(int no_cuenta, double monto, int index, int esCarga) {
    if (monto <= 0) {
        if(esCarga) {
            sprintf(r2.errores[errorCargaO], "    - Linea #%d: Monto no válido para depósito: %.2f", lineas_operaciones, monto);
            errorCargaO ++;
        } else {
            printf("\x1b[31m" "Monto no válido para depósito: %.2f\n" "\x1b[0m", monto);
        }
        return;
    }
    for (int i = 0; i < num_usuarios; ++i) {
        if (usuarios[i].no_cuenta == no_cuenta) {
            usuarios[i].saldo += monto;
            if(esCarga) {
                r2.operaciones[1]++;
                r2.hilos[index]++;
            } else {
                printf("\x1b[32m" "Depósito exitoso. Nuevo saldo de la cuenta %d: %.2f\n" "\x1b[0m", no_cuenta, usuarios[i].saldo);
            }
            return;
        }
    }
    if(esCarga) {
        sprintf(r2.errores[errorCargaO], "    - Linea #%d: Número de cuenta no encontrado: %d", lineas_operaciones, no_cuenta);
        errorCargaO ++;
    } else {
        printf("\x1b[31m" "Número de cuenta no encontrado: %d\n" "\x1b[0m", no_cuenta);
    }
}

void retiro(int no_cuenta, double monto, int index, int esCarga) {
    if (monto <= 0) {
        if(esCarga) {
            sprintf(r2.errores[errorCargaO], "    - Linea #%d: Monto no válido para retiro: %.2f", lineas_operaciones, monto);
            errorCargaO ++;
        } else {
            printf("\x1b[31m" "Monto no válido para retiro: %.2f\n" "\x1b[0m", monto);
        }
        return;
    }
    for (int i = 0; i < num_usuarios; ++i) {
        if (usuarios[i].no_cuenta == no_cuenta) {
            if (usuarios[i].saldo >= monto) {
                usuarios[i].saldo -= monto;
                if(esCarga) {
                    r2.operaciones[0]++;
                    r2.hilos[index]++;
                    lineas_operaciones++;
                } else {
                    printf("\x1b[32m" "Retiro exitoso. Nuevo saldo de la cuenta %d: %.2f\n" "\x1b[0m", no_cuenta, usuarios[i].saldo);
                }
            } else {
                if(esCarga) {
                    sprintf(r2.errores[errorCargaO], "    - Linea #%d: Saldo insuficiente en la cuenta %d", lineas_operaciones, no_cuenta);
                    errorCargaO ++;
                } else {
                    printf("\x1b[31m" "Saldo insuficiente en la cuenta %d\n" "\x1b[0m", no_cuenta);
                }
            }
            return;
        }
    }
    if(esCarga) {
        sprintf(r2.errores[errorCargaO], "    - Linea #%d: Número de cuenta no encontrado: %d", lineas_operaciones, no_cuenta);
        errorCargaO ++;
    } else {
        printf("\x1b[31m" "Número de cuenta no encontrado: %d\n" "\x1b[0m", no_cuenta);
    }
}

void transferencia(int cuenta_origen, int cuenta_destino, double monto, int index, int esCarga) {
    if (monto <= 0) {
        if(esCarga) {
            sprintf(r2.errores[errorCargaO], "    - Linea #%d: Monto no válido para transferencia: %.2f", lineas_operaciones, monto);
            errorCargaO ++;
        } else {
            printf("\x1b[31m" "Monto no válido para transferencia: %.2f\n" "\x1b[0m", monto);
        }
        return;
    }
    Usuario *origen = NULL, *destino = NULL;
    for (int i = 0; i < num_usuarios; ++i) {
        if (usuarios[i].no_cuenta == cuenta_origen) {
            origen = &usuarios[i];
        }
        if (usuarios[i].no_cuenta == cuenta_destino) {
            destino = &usuarios[i];
        }
    }
    if (origen && destino) {
        if (origen->saldo >= monto) {
            origen->saldo -= monto;
            destino->saldo += monto;
            if(esCarga) {
                r2.operaciones[2]++;
                r2.hilos[index]++;
            } else {
                printf("\x1b[32m" "Transferencia exitosa de la cuenta %d a la cuenta %d por %.2f\n" "\x1b[0m", cuenta_origen, cuenta_destino, monto);
            }
        } else {
            if(esCarga) {
                sprintf(r2.errores[errorCargaO], "    - Linea #%d: Saldo insuficiente en la cuenta origen %d", lineas_operaciones, cuenta_origen);
                errorCargaO ++;
            } else {
                printf("\x1b[31m" "Saldo insuficiente en la cuenta %d\n" "\x1b[0m", cuenta_origen);
            }
        }
    } else {
        if (!origen) {
            if(esCarga) {
                sprintf(r2.errores[errorCargaO], "    - Linea #%d: Número de cuenta de origen no encontrado: %d", lineas_operaciones, cuenta_origen);
                errorCargaO ++;
            } else {
                printf("\x1b[31m" "Número de cuenta de origen no encontrado: %d\n" "\x1b[0m", cuenta_origen);
            }
        }
        if (!destino) {
            if(esCarga) {
                sprintf(r2.errores[errorCargaO], "    - Linea #%d: Número de cuenta de destino no encontrado: %d", lineas_operaciones, cuenta_destino);
                errorCargaO ++;
            } else {
                printf("\x1b[31m" "Número de cuenta de destino no encontrado: %d\n" "\x1b[0m", cuenta_destino);
            }
        }
    }
}

void consultar_cuenta(int no_cuenta) {
    for (int i = 0; i < num_usuarios; ++i) {
        if (usuarios[i].no_cuenta == no_cuenta) {
            printf("\n--------------------------\n");
            printf("Número de cuenta: %d\nNombre: %s\nSaldo: %.2f\n", usuarios[i].no_cuenta, usuarios[i].nombre, usuarios[i].saldo);
            pthread_mutex_unlock(&mutex);
            return;
        }
    }
    printf("\x1b[31m" "Número de cuenta no encontrado: %d\n" "\x1b[0m", no_cuenta);
    pthread_mutex_unlock(&mutex);
}


void generar_numeros(int n, int* a, int* b, int* c) {
    int max_diff = 4;
    *a = rand() % (n + 1);
    *b = *a + (rand() % (2 * max_diff + 1)) - max_diff;
    *c = *a + (rand() % (2 * max_diff + 1)) - max_diff;
    *c = n - *a - *b;
    if (abs(*a - *b) > max_diff || abs(*a - *c) > max_diff || abs(*b - *c) > max_diff) {
        generar_numeros(n, a, b, c);
    }
}

void reporteCargaUsuarios() {
    char nombre[29];
    sprintf(nombre, "carga_%s.log", fecha_hora("%Y_%m_%d-%H_%M_%S"));
    char fecha[19];
    sprintf(fecha, "%s", fecha_hora("%Y-%m-%d %H:%M:%S"));
    // abrir o crear archivo
    int fd = open(nombre, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        exit(1);
    }
    //
    int total = 0;
    char buffer[4096 * 2];
    int offset = snprintf(buffer, sizeof(buffer), "-------------- Carga de Usuarios --------------\n\nFecha: %s\n\nUsuarios Cargados:\n", fecha);
    for (int i = 0; i < 3; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "Hilo #%d: %d\n", i + 1, r1.hilos[i]);
        total += r1.hilos[i];
    }
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "Total: %d\n\nErrores:\n", total);
    for (int i = 0; i < 1000 && r1.errores[i][0] != '\0'; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s\n", r1.errores[i]);
    }

    // Escribir el contenido del buffer en el archivo
    write(fd, buffer, strlen(buffer));

    close(fd);
    //
    printf("\x1b[32m" "¡Reporte de Carga de Usuarios generado!\n\n" "\x1b[0m");
}


void reporteCargaOperaciones() {
    char nombre[35];
    sprintf(nombre, "operaciones_%s.log", fecha_hora("%Y_%m_%d-%H_%M_%S"));
    char fecha[19];
    sprintf(fecha, "%s", fecha_hora("%Y-%m-%d %H:%M:%S"));
    // abrir o crear archivo
    int fd = open(nombre, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        exit(1);
    }
    //
    int total = 0;
    char buffer[4096 * 2];
    int offset = snprintf(buffer, sizeof(buffer), "-------------- Resumen de Operaciones --------------\n\nFecha: %s\n\nOperaciones realizadas:\n", fecha);
    for (int i = 0; i < 3; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, i == 0 ? "Retiros: %d\n" : (i == 1 ? "Depositos: %d\n" : "Transferencias: %d\n"), r2.operaciones[i]);
        total += r2.operaciones[i];
    }
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "Total: %d\n\nOperaciones por hilo:\n", total);
    total = 0;
    for (int i = 0; i < 4; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "Hilo #%d: %d\n", i + 1, r2.hilos[i]);
        total += r2.hilos[i];
    }
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "Total: %d\n\nErrores:\n", total);
    for (int i = 0; i < 1000 && r2.errores[i][0] != '\0'; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s\n", r2.errores[i]);
    }

    // Escribir el contenido del buffer en el archivo
    write(fd, buffer, strlen(buffer));

    close(fd);
    //
    printf("\x1b[32m" "¡Reporte de Carga de Operaciones generado!\n\n" "\x1b[0m");
}

void reporteEstadosCuenta() {
    int fd = open("estado_cuenta.json", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        exit(1);
    }
    //
    json_t *root = json_array();
    if (!root) {
        perror("json_array");
        close(fd);
        exit(1);
    }

    // Agregar cada usuario al arreglo JSON
    for (int i = 0; i < MAX_USERS; i++) {
        if (usuarios[i].no_cuenta != 0) {  // Verificar si el usuario es válido
            json_t *usuario = json_object();
            json_object_set_new(usuario, "no_cuenta", json_integer(usuarios[i].no_cuenta));
            json_object_set_new(usuario, "nombre", json_string(usuarios[i].nombre));
            json_object_set_new(usuario, "saldo", json_real(usuarios[i].saldo));
            json_array_append_new(root, usuario);
        }
    }

    // Escribir el objeto JSON en el archivo
    char *json_str = json_dumps(root, JSON_INDENT(4));
    if (!json_str) {
        perror("json_dumps");
        json_decref(root);
        close(fd);
        exit(1);
    }

    if (write(fd, json_str, strlen(json_str)) == -1) {
        perror("write");
        free(json_str);
        json_decref(root);
        close(fd);
        exit(1);
    }

    free(json_str);
    json_decref(root);
    close(fd);
    //
    close(fd);
    //
    printf("\x1b[32m" "¡Estados de Cuenta generado!\n\n" "\x1b[0m");
}

int main() {
    pthread_mutex_init(&mutex, NULL);

    pthread_t threads[8];
    int thread_ids[4];

    // Inicializar los IDs de los hilos
    for (int i = 0; i < 4; i++) {
        thread_ids[i] = i;
    }

    printf("\x1b[32m" "\nCarga de Usuarios...\n" "\x1b[0m");


    // Cargar usuarios en un solo hilo
    const char* file = "usuarios.json";
    json_error_t error;
    json_t *root = json_load_file(file, 0, &error);
    generar_numeros(json_array_size(root), &procesado[0], &procesado[1], &procesado[2]);

    // Procesar usuarios en 3 hilos
    for (int i = 0; i < 3; i++) {
        pthread_create(&threads[i], NULL, cargar_usuarios, &thread_ids[i]);
    }

    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\x1b[32m" "¡Carga de Usuarios Finalizada!\n" "\x1b[0m");

    // Generar el reporte después de cargar y procesar usuarios
    reporteCargaUsuarios();

    printf("\x1b[32m" "\nCarga de Operaciones...\n" "\x1b[0m");

    // Procesar operaciones concurrentemente con 4 hilos
    for (int i = 0; i < 4; i++) {
        pthread_create(&threads[i + 4], NULL, cargar_operaciones, &thread_ids[i]);
    }

    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i + 4], NULL);
    }

    printf("\x1b[32m" "¡Carga de Operaciones Finalizada!\n" "\x1b[0m");

    reporteCargaOperaciones();

    // Menú de operaciones
    int opcion;
    while (1) {
        printf("Menú de Operaciones:\n1. Deposito\n2. Retiro\n3. Transferencia\n4. Consultar cuenta\n5. Generar Estados de Cuenta\n6. Salir\n\nOpción: ");
        scanf("%d", &opcion);

        int no_cuenta, cuenta_origen, cuenta_destino;
        double monto;
        int index[4];

        switch (opcion) {
            case 1:
                printf("\n\x1b[36mDepósito\x1b[0m\nNúmero de cuenta: ");
                scanf("%d", &no_cuenta);
                printf("Monto a depositar: ");
                scanf("%lf", &monto);
                deposito(no_cuenta, monto, -1, 0);
                printf("\n");
                break;
            case 2:
                printf("\n\x1b[36mRetiro\x1b[0m\nNúmero de cuenta: ");
                scanf("%d", &no_cuenta);
                printf("Monto a retirar: ");
                scanf("%lf", &monto);
                retiro(no_cuenta, monto, -1, 0);
                printf("\n");
                break;
            case 3:
                printf("\n\x1b[36mTransferencia\x1b[0m\nNúmero de cuenta origen: ");
                scanf("%d", &cuenta_origen);
                printf("Número de cuenta destino: ");
                scanf("%d", &cuenta_destino);
                printf("Monto a transferir: ");
                scanf("%lf", &monto);
                transferencia(cuenta_origen, cuenta_destino, monto, -1, 0);
                printf("\n");
                break;
            case 4:
                printf("\n\x1b[36mConsulta de Cuenta\x1b[0m\nNúmero de cuenta: ");
                scanf("%d", &no_cuenta);
                consultar_cuenta(no_cuenta);
                printf("\n");
                break;
            case 5:
                printf("\x1b[32m" "\nEstados de Cuenta...\n" "\x1b[0m");
                reporteEstadosCuenta();
                break;
            case 6:
                exit(0);
            default:
                printf("Opción no válida\n");
        }
    }

    pthread_mutex_destroy(&mutex);
    return 0;
}