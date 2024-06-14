#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <jansson.h>  // Biblioteca para manejar JSON

#define MAX_USERS 1000
#define MAX_OPERATIONS 1000

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

Usuario usuarios[MAX_USERS];
int num_usuarios = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Declaraciones de funciones
void* cargar_usuarios(void* filename);
void* cargar_operaciones(void* filename);
void deposito(int no_cuenta, double monto);
void retiro(int no_cuenta, double monto);
void transferencia(int cuenta_origen, int cuenta_destino, double monto);
void consultar_cuenta(int no_cuenta);

// Función para cargar usuarios desde un archivo JSON
void* cargar_usuarios(void* filename) {
    const char* file = (const char*)filename;
    json_error_t error;
    json_t *root = json_load_file(file, 0, &error);
    if (!root) {
        fprintf(stderr, "Error al cargar JSON: %s\n", error.text);
        return NULL;
    }

    size_t index;
    json_t *user;
    json_array_foreach(root, index, user) {
        pthread_mutex_lock(&mutex);
        int no_cuenta = json_integer_value(json_object_get(user, "no_cuenta"));
        const char* nombre = json_string_value(json_object_get(user, "nombre"));
        double saldo = json_real_value(json_object_get(user, "saldo"));

        // Verificar si el número de cuenta ya existe
        int i;
        for (i = 0; i < num_usuarios; ++i) {
            if (usuarios[i].no_cuenta == no_cuenta) {
                printf("Error: Número de cuenta duplicado %d\n", no_cuenta);
                pthread_mutex_unlock(&mutex);
                continue;
            }
        }

        // Agregar usuario al arreglo
        usuarios[num_usuarios].no_cuenta = no_cuenta;
        strncpy(usuarios[num_usuarios].nombre, nombre, sizeof(usuarios[num_usuarios].nombre) - 1);
        usuarios[num_usuarios].saldo = saldo;
        num_usuarios++;
        pthread_mutex_unlock(&mutex);
    }
    json_decref(root);
    return NULL;
}

// Función para realizar operaciones desde un archivo JSON
void* cargar_operaciones(void* filename) {
    const char* file = (const char*)filename;
    json_error_t error;
    json_t *root = json_load_file(file, 0, &error);
    if (!root) {
        fprintf(stderr, "Error al cargar JSON: %s\n", error.text);
        return NULL;
    }

    size_t index;
    json_t *operation;
    json_array_foreach(root, index, operation) {
        int operacion = json_integer_value(json_object_get(operation, "operacion"));
        int cuenta1 = json_integer_value(json_object_get(operation, "cuenta1"));
        int cuenta2 = json_integer_value(json_object_get(operation, "cuenta2"));
        double monto = json_real_value(json_object_get(operation, "monto"));

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

int main() {
    pthread_t threads[3];

    // Cargar usuarios en 3 hilos
    pthread_create(&threads[0], NULL, cargar_usuarios, "usuarios1.json");
    pthread_create(&threads[1], NULL, cargar_usuarios, "usuarios2.json");
    pthread_create(&threads[2], NULL, cargar_usuarios, "usuarios3.json");

    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }

    // Menú de operaciones
    int opcion;
    while (1) {
        printf("Menú de Operaciones:\n1. Deposito\n2. Retiro\n3. Transferencia\n4. Consultar cuenta\n5. Salir\n");
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
                exit(0);
            default:
                printf("Opción no válida\n");
        }
    }

    return 0;
}
