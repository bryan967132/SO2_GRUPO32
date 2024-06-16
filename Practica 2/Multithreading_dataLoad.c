#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <jansson.h>  // Biblioteca para manejar JSON

#define MAX_USERS 1000
#define MAX_OPERATIONS 1000
#define LOG_FILE_PREFIX "carga_"
#define MAX_ERRORS 1000

typedef struct {
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
    int thread_id;
    int num_usuarios_cargados;
} ReporteCarga;

typedef struct {
    size_t linea;
    char mensaje[256];
} Error;

Usuario usuarios[MAX_USERS];
Operacion operaciones[MAX_OPERATIONS];
ReporteCarga reporte_carga[4];
Error errores[MAX_ERRORS];
int num_usuarios = 0;
int num_operaciones = 0;
int num_errores = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
FILE *log_file;
char log_filename[64];

// Declaraciones de funciones
void* cargar_usuarios(void* arg);
void* cargar_operaciones(void* arg);
void deposito(int no_cuenta, double monto);
void retiro(int no_cuenta, double monto);
void transferencia(int cuenta_origen, int cuenta_destino, double monto);
void consultar_cuenta(int no_cuenta);
void* procesar_usuarios(void* arg);
void* procesar_operaciones(void* arg);
void generar_nombre_log(char* buffer, size_t size);
void escribir_log(const char* mensaje);
void registrar_error(size_t linea, const char* mensaje);
void ver_reporte(const char* log_filename);
void generar_reporte();

// Función para generar el nombre del archivo de log con la fecha y hora actual
void generar_nombre_log(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, LOG_FILE_PREFIX "%Y_%m_%d-%H_%M_%S.log", t);
}

// Función para escribir mensajes en el archivo de log
void escribir_log(const char* mensaje) {
    if (log_file) {
        fprintf(log_file, "%s\n", mensaje);
    }
}

// Función para registrar errores en memoria
void registrar_error(size_t linea, const char* mensaje) {
    if (num_errores < MAX_ERRORS) {
        errores[num_errores].linea = linea;
        strncpy(errores[num_errores].mensaje, mensaje, sizeof(errores[num_errores].mensaje) - 1);
        num_errores++;
    }
}

// Función para cargar usuarios desde un archivo JSON
void* cargar_usuarios(void* arg) {
    const char* file = "usuarios.json";
    json_error_t error;
    json_t *root = json_load_file(file, 0, &error);
    if (!root) {
        fprintf(stderr, "Error al cargar JSON: %s\n", error.text);
        return NULL;
    }

    size_t index;
    json_t *user;
    json_array_foreach(root, index, user) {
        int no_cuenta = json_integer_value(json_object_get(user, "no_cuenta"));
        const char* nombre = json_string_value(json_object_get(user, "nombre"));
        double saldo = json_real_value(json_object_get(user, "saldo"));

        if (no_cuenta <= 0) {
            registrar_error(index + 1, "Número de cuenta no es entero positivo");
            continue;
        }

        if (saldo < 0) {
            registrar_error(index + 1, "Saldo no puede ser menor a 0");
            continue;
        }

        pthread_mutex_lock(&mutex);

        // Verificar si el número de cuenta ya existe
        int i;
        int duplicado = 0;
        for (i = 0; i < num_usuarios; ++i) {
            if (usuarios[i].no_cuenta == no_cuenta) {
                registrar_error(index + 1, "Número de cuenta duplicado");
                duplicado = 1;
                break;
            }
        }

        if (!duplicado) {
            // Agregar usuario al arreglo
            usuarios[num_usuarios].no_cuenta = no_cuenta;
            strncpy(usuarios[num_usuarios].nombre, nombre, sizeof(usuarios[num_usuarios].nombre) - 1);
            usuarios[num_usuarios].saldo = saldo;
            num_usuarios++;
        }
        pthread_mutex_unlock(&mutex);
    }
    json_decref(root);
    return NULL;
}

// Función para cargar operaciones desde un archivo JSON
void* cargar_operaciones(void* arg) {
    const char* file = (const char*)arg;
    json_error_t error;
    json_t *root = json_load_file(file, 0, &error);
    if (!root) {
        fprintf(stderr, "Error al cargar JSON: %s\n", error.text);
        return NULL;
    }

    size_t index;
    json_t *operation;
    json_array_foreach(root, index, operation) {
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
        num_operaciones++;
        pthread_mutex_unlock(&mutex);
    }
    json_decref(root);
    return NULL;
}

// Funciones para las operaciones individuales
void deposito(int no_cuenta, double monto) {
    if (monto <= 0) {
        printf("Monto no válido para depósito: %.2f\n", monto);
        return;
    }
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < num_usuarios; ++i) {
        if (usuarios[i].no_cuenta == no_cuenta) {
            usuarios[i].saldo += monto;
            printf("Depósito exitoso. Nuevo saldo de la cuenta %d: %.2f\n", no_cuenta, usuarios[i].saldo);
            pthread_mutex_unlock(&mutex);
            return;
        }
    }
    printf("Número de cuenta no encontrado: %d\n", no_cuenta);
    pthread_mutex_unlock(&mutex);
}

void retiro(int no_cuenta, double monto) {
    if (monto <= 0) {
        printf("Monto no válido para retiro: %.2f\n", monto);
        return;
    }
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < num_usuarios; ++i) {
        if (usuarios[i].no_cuenta == no_cuenta) {
            if (usuarios[i].saldo >= monto) {
                usuarios[i].saldo -= monto;
                printf("Retiro exitoso. Nuevo saldo de la cuenta %d: %.2f\n", no_cuenta, usuarios[i].saldo);
            } else {
                printf("Saldo insuficiente en la cuenta %d\n", no_cuenta);
            }
            pthread_mutex_unlock(&mutex);
            return;
        }
    }
    printf("Número de cuenta no encontrado: %d\n", no_cuenta);
    pthread_mutex_unlock(&mutex);
}

void transferencia(int cuenta_origen, int cuenta_destino, double monto) {
    if (monto <= 0) {
        printf("Monto no válido para transferencia: %.2f\n", monto);
        return;
    }
    pthread_mutex_lock(&mutex);
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
            printf("Transferencia exitosa de la cuenta %d a la cuenta %d por %.2f\n", cuenta_origen, cuenta_destino, monto);
        } else {
            printf("Saldo insuficiente en la cuenta %d\n", cuenta_origen);
        }
    } else {
        if (!origen) {
            printf("Número de cuenta de origen no encontrado: %d\n", cuenta_origen);
        }
        if (!destino) {
            printf("Número de cuenta de destino no encontrado: %d\n", cuenta_destino);
        }
    }
    pthread_mutex_unlock(&mutex);
}

void consultar_cuenta(int no_cuenta) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < num_usuarios; ++i) {
        if (usuarios[i].no_cuenta == no_cuenta) {
            printf("Número de cuenta: %d\nNombre: %s\nSaldo: %.2f\n", usuarios[i].no_cuenta, usuarios[i].nombre, usuarios[i].saldo);
            pthread_mutex_unlock(&mutex);
            return;
        }
    }
    printf("Número de cuenta no encontrado: %d\n", no_cuenta);
    pthread_mutex_unlock(&mutex);
}

void* procesar_usuarios(void* arg) {
    int thread_id = *(int*)arg;
    int total_threads = 4;
    int usuarios_por_hilo = num_usuarios / total_threads;
    int start = thread_id * usuarios_por_hilo;
    int end = (thread_id == total_threads - 1) ? num_usuarios : start + usuarios_por_hilo;

    reporte_carga[thread_id].num_usuarios_cargados = 0;
    for (int i = start; i < end; ++i) {
        // Simula algún procesamiento para el usuario
        printf("Hilo %d procesando usuario %d\n", thread_id, usuarios[i].no_cuenta);
        reporte_carga[thread_id].num_usuarios_cargados++;
    }
    return NULL;
}

void* procesar_operaciones(void* arg) {
    int thread_id = *(int*)arg;
    for (int i = thread_id; i < num_operaciones; i += 4) {
        int operacion = operaciones[i].operacion;
        int cuenta1 = operaciones[i].cuenta1;
        int cuenta2 = operaciones[i].cuenta2;
        double monto = operaciones[i].monto;

        switch (operacion) {
            case 1:
                deposito(cuenta1, monto);
                break;
            case 2:
                retiro(cuenta1, monto);
                break;
            case 3:
                transferencia(cuenta1, cuenta2, monto);
                break;
            default:
                printf("Operación desconocida: %d\n", operacion);
                break;
        }
    }
    return NULL;
}

void ver_reporte(const char* log_filename) {
    FILE *file = fopen(log_filename, "r");
    if (file == NULL) {
        printf("No se pudo abrir el archivo de log: %s\n", log_filename);
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        printf("%s", line);
    }
    fclose(file);
}

void generar_reporte() {
    log_file = fopen(log_filename, "a");
    if (!log_file) {
        fprintf(stderr, "No se pudo abrir el archivo de log para escritura\n");
        return;
    }

    char log_msg[256];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(log_msg, sizeof(log_msg), "Fecha: %Y-%m-%d %H:%M:%S\n", t);
    escribir_log(log_msg);

    escribir_log("\nUsuarios Cargados:");
    int total_usuarios_cargados = 0;
    for (int i = 0; i < 4; i++) {
        snprintf(log_msg, sizeof(log_msg), "Hilo #%d: %d", i + 1, reporte_carga[i].num_usuarios_cargados);
        escribir_log(log_msg);
        total_usuarios_cargados += reporte_carga[i].num_usuarios_cargados;
    }
    snprintf(log_msg, sizeof(log_msg), "Total: %d", total_usuarios_cargados);
    escribir_log(log_msg);

    escribir_log("\nErrores:");
    for (int i = 0; i < num_errores; i++) {
        snprintf(log_msg, sizeof(log_msg), "-Línea#%zu: %s", errores[i].linea, errores[i].mensaje);
        escribir_log(log_msg);
    }

    fclose(log_file);
}

int main() {
    pthread_t threads[8];
    int thread_ids[4];

    // Generar nombre del archivo de log
    generar_nombre_log(log_filename, sizeof(log_filename));
    log_file = fopen(log_filename, "w");
    if (!log_file) {
        fprintf(stderr, "No se pudo crear el archivo de log\n");
        return 1;
    }
    fclose(log_file);

    // Inicializar los IDs de los hilos
    for (int i = 0; i < 4; i++) {
        thread_ids[i] = i;
    }

    // Cargar usuarios en un solo hilo
    pthread_create(&threads[0], NULL, cargar_usuarios, &thread_ids[0]);
    pthread_join(threads[0], NULL);

    // Procesar usuarios en 4 hilos
    for (int i = 0; i < 4; i++) {
        pthread_create(&threads[i], NULL, procesar_usuarios, &thread_ids[i]);
    }

    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }

    // Generar el reporte después de cargar y procesar usuarios
    generar_reporte();

    // Cargar operaciones en un hilo
    pthread_create(&threads[4], NULL, cargar_operaciones, "operaciones.json");
    pthread_join(threads[4], NULL);

    // Procesar operaciones concurrentemente con 4 hilos
    for (int i = 0; i < 4; i++) {
        pthread_create(&threads[i + 4], NULL, procesar_operaciones, &thread_ids[i]);
    }

    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i + 4], NULL);
    }

    // Menú de operaciones
    int opcion;
    while (1) {
        printf("Menú de Operaciones:\n1. Depósito\n2. Retiro\n3. Transferencia\n4. Consultar cuenta\n5. Ver reporte de carga\n6. Salir\n");
        scanf("%d", &opcion);

        int no_cuenta, cuenta_origen, cuenta_destino;
        double monto;

        switch (opcion) {
            case 1:
                printf("Número de cuenta: ");
                scanf("%d", &no_cuenta);
                printf("Monto a depositar: ");
                scanf("%lf", &monto);
                deposito(no_cuenta, monto);
                break;
            case 2:
                printf("Número de cuenta: ");
                scanf("%d", &no_cuenta);
                printf("Monto a retirar: ");
                scanf("%lf", &monto);
                retiro(no_cuenta, monto);
                break;
            case 3:
                printf("Número de cuenta origen: ");
                scanf("%d", &cuenta_origen);
                printf("Número de cuenta destino: ");
                scanf("%d", &cuenta_destino);
                printf("Monto a transferir: ");
                scanf("%lf", &monto);
                transferencia(cuenta_origen, cuenta_destino, monto);
                break;
            case 4:
                printf("Número de cuenta: ");
                scanf("%d", &no_cuenta);
                consultar_cuenta(no_cuenta);
                break;
            case 5:
                ver_reporte(log_filename);
                break;
            case 6:
                exit(0);
            default:
                printf("Opción no válida\n");
        }
    }

    return 0;
}
