/* bankAccount.h - Simulação de Banco de Diretores de Cinema
 * Autores: Coppola, Leone, Hitchcock, Scorsese, Carpenter
*/

#ifndef BANK_ACCOUNT_H
#define BANK_ACCOUNT_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_ACCOUNTS 5

// 1. "Molde" da Conta Bancária
typedef struct {
    int id;
    char nome[50];
    double saldo;
} ContaBancaria;

// 2. Vetor criado com as 5 contas preenchidas (Iniciando com R$ 1000 cada)
ContaBancaria banco[NUM_ACCOUNTS] = {
    {0, "Francisco Coppola", 1000.00},
    {1, "Serginho Leone",     1000.00},
    {2, "Alfredo Hitchcock", 1000.00},
    {3, "Martinho Scorcese", 1000.00},
    {4, "Joao Carpenter",     1000.00}
};

// Um cadeado (Mutex) para cada conta para que os caixas não mexam na mesma gaveta juntos
pthread_mutex_t locks[NUM_ACCOUNTS] = {
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER
};

// Arquivo de texto para o Extrato e seu cadeado de escrita
FILE *logFile = NULL;
pthread_mutex_t logLock = PTHREAD_MUTEX_INITIALIZER;

void initBank() {
    logFile = fopen("extrato_bancario.txt", "w");
    if (logFile == NULL) {
        perror("Erro ao abrir arquivo de log");
        exit(1);
    }
    fprintf(logFile, "=== EXTRATO DO BANCO DOS DIRETORES ===\n\n");
}

// Função auxiliar para salvar no arquivo de texto e mostrar na tela
void registrar_log(const char* msg) {
    pthread_mutex_lock(&logLock);
    if (logFile != NULL) { fprintf(logFile, "%s\n", msg); fflush(logFile); }
    printf("%s\n", msg);
    pthread_mutex_unlock(&logLock);
}

// 1. FUNÇÃO DE DEPÓSITO (Recebe qual conta vai mexer e o valor)
void deposit(int account_id, double amount, unsigned long thread_id) {
    pthread_mutex_lock(&locks[account_id]); // Tranca a conta sorteada

    banco[account_id].saldo += amount; // Entra na conta e soma no saldo dela

    char msg[256];
    sprintf(msg, "[Caixa %lu] DEPOSITOU R$ %.2f para %s | Novo Saldo: R$ %.2f",
            thread_id, amount, banco[account_id].nome, banco[account_id].saldo);
    registrar_log(msg);

    pthread_mutex_unlock(&locks[account_id]); // Destranca
}

// 2. FUNÇÃO DE SAQUE (Com a restrição de saldo insuficiente)
int withdraw(int account_id, double amount, unsigned long thread_id) {
    pthread_mutex_lock(&locks[account_id]);
    int sucesso = 0;
    char msg[256];

    if (banco[account_id].saldo >= amount) {
        banco[account_id].saldo -= amount;
        sucesso = 1;
        sprintf(msg, "[Caixa %lu] SAQUE de R$ %.2f realizado por %s | Novo Saldo: R$ %.2f",
                thread_id, amount, banco[account_id].nome, banco[account_id].saldo);
    } else {
        sprintf(msg, "[Caixa %lu] SAQUE de R$ %.2f NEGADO para %s | Saldo Insuficiente (Saldo: R$ %.2f)",
                thread_id, amount, banco[account_id].nome, banco[account_id].saldo);
    }

    registrar_log(msg);
    pthread_mutex_unlock(&locks[account_id]);
    return sucesso;
}

// 3. FUNÇÃO DE CONSULTA DE SALDO
double balanceinquiry(int account_id, unsigned long thread_id) {
    pthread_mutex_lock(&locks[account_id]);

    double atual = banco[account_id].saldo;
    char msg[256];
    sprintf(msg, "[Caixa %lu] CONSULTA: %s possui R$ %.2f", thread_id, banco[account_id].nome, atual);
    registrar_log(msg);

    pthread_mutex_unlock(&locks[account_id]);
    return atual;
}

// 4. FUNÇÃO DE TRANSFERÊNCIA (Tira de um diretor e coloca em outro + Evita Travamento/Deadlock)
void transfermoney(int de_id, int para_id, double amount, unsigned long thread_id) {
    if (de_id == para_id) return;

    // Regra de segurança: tranca sempre o ID menor primeiro para evitar Deadlock
    if (de_id < para_id) {
        pthread_mutex_lock(&locks[de_id]);
        pthread_mutex_lock(&locks[para_id]);
    } else {
        pthread_mutex_lock(&locks[para_id]);
        pthread_mutex_lock(&locks[de_id]);
    }

    char msg[256];
    if (banco[de_id].saldo >= amount) {
        banco[de_id].saldo -= amount;
        banco[para_id].saldo += amount;
        sprintf(msg, "[Caixa %lu] TRANSFERENCIA: R$ %.2f de %s -> %s | SUCESSO!",
                thread_id, amount, banco[de_id].nome, banco[para_id].nome);
    } else {
        sprintf(msg, "[Caixa %lu] TRANSFERENCIA: R$ %.2f de %s -> %s | NEGADA! %s sem saldo (Saldo: R$ %.2f)",
                thread_id, amount, banco[de_id].nome, banco[para_id].nome, banco[de_id].nome, banco[de_id].saldo);
    }

    registrar_log(msg);
    pthread_mutex_unlock(&locks[de_id]);
    pthread_mutex_unlock(&locks[para_id]);
}

void cleanup() {
    for(int i=0; i<NUM_ACCOUNTS; i++) pthread_mutex_destroy(&locks[i]);
    pthread_mutex_destroy(&logLock);
    if (logFile != NULL) fclose(logFile);
}

#endif
