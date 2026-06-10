/* mutualExclusion.c - Simulador de Movimentações Aleatórias
 * Adaptado para o Banco dos Diretores de Cinema
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// Importa o arquivo de cabeçalho com os nossos diretores e funções
#include "bankAccount.h"
#define TRANSACTIONS_PER_THREAD 25 // Quantas operações cada caixa vai fazer

// Esta é a rotina que cada caixa (Thread) fará de forma independente
void doRandomTransactions(unsigned long thread_id) {
    // Cria uma semente de sorteio diferente para cada caixa não repetir os mesmos números
    unsigned int seed = (unsigned int)(time(NULL) ^ thread_id);

    for (unsigned i = 0; i < TRANSACTIONS_PER_THREAD; i++) {
        int operacao = rand_r(&seed) % 4; // Sorteia: 0=Depósito, 1=Saque, 2=Consulta, 3=Transferência
        int conta_sorteada = rand_r(&seed) % NUM_ACCOUNTS; // Sorteia um Diretor (0 a 4)
    
        // Sorteia um valor quebrado para a transação entre R$ 10.00 e R$ 109.90
        // double valor = ((rand_r(&seed) % 1000) / 10.0) + 10.0;
        // Sorteia um valor quebrado para a transação entre R$ 500.00 e R$ 1499.00
        double valor = (rand_r(&seed) % 1000) + 500.0;

        // O Caixa executa a função correta dependendo do número que foi sorteado:
        if (operacao == 0) {
            deposit(conta_sorteada, valor, thread_id);
        }
        else if (operacao == 1) {
            withdraw(conta_sorteada, valor, thread_id);
        }
        else if (operacao == 2) {
            balanceinquiry(conta_sorteada, thread_id);
        }
        else if (operacao == 3) {
            // Se for transferência, precisamos sortear um SEGUNDO diretor (o destino)
            int destino = rand_r(&seed) % NUM_ACCOUNTS;

            // Garante que o caixa não vai tentar transferir de um diretor para ele mesmo
            while (destino == conta_sorteada) {
                destino = rand_r(&seed) % NUM_ACCOUNTS;
            }
            transfermoney(conta_sorteada, destino, valor, thread_id);
        }

        // Uma pausa (300 milissegundos) para fazer os caixas trabalharem juntos
        usleep(300000);
  }
}

// Função intermediária que o sistema do Linux exige para disparar a Thread
void* child(void * buf) {
    unsigned long childID = (unsigned long) buf;
    doRandomTransactions(childID); 
    return NULL;
}

// Lê quantos caixas (threads) você digitou no terminal na hora de rodar
unsigned long processCommandLine(int argc, char** argv) {
    if (argc == 2) {
        return strtoul(argv[1], 0, 10);
    } else if (argc == 1) {
        return 4; // Se você não digitar nada, ele roda com 4 caixas por padrão
    } else {
        fprintf(stderr, "\nUso correto: ./mutualExclusion [numeroDeThreads]\n");
        exit(1);
    }
}

int main(int argc, char** argv) {
    pthread_t * children;
    unsigned long id = 0;
    unsigned long numThreads = 0;
    numThreads = processCommandLine(argc, argv);
    // Abre e prepara o arquivo "extrato_bancario.txt"
    initBank();
    printf("\n========================================================\n");
    printf(" Iniciando o Banco dos Diretores com %lu Caixas (Threads)...\n", numThreads);
    printf(" O histórico está sendo gravado em 'extrato_bancario.txt'\n");
    printf("========================================================\n\n");

    // Reserva espaço na memória do computador para criar os caixas
    children = malloc( (numThreads - 1) * sizeof(pthread_t) );

    // DISPARO (FORK): Cria e liga os caixas eletrônicos em paralelo
    for (id = 1; id < numThreads; id++) {
        pthread_create( &(children[id-1]), NULL, child, (void*) id );
    }

    // O Caixa principal (ID 0) também trabalha fazendo transações aleatórias
    doRandomTransactions(0);

    // ESPERA (JOIN): A função principal aguarda todos os caixas terminarem as transações
    for (id = 1; id < numThreads; id++) {
        pthread_join( children[id-1], NULL );
    }

    // AUDITORIA FINAL: Imprime na tela o saldo final de cada diretor após a correria
    printf("\n========================================================\n");
    printf("             BALANÇO FINAL DO BANCO DO CINEMA           \n");
    printf("========================================================\n");
    double patrimonioTotal = 0;
    for(int i = 0; i < NUM_ACCOUNTS; i++) {
        printf("Diretor: %-20s | Saldo Consolidado = R$ %.2f\n", banco[i].nome, banco[i].saldo);
        patrimonioTotal += banco[i].saldo;
    }
    printf("--------------------------------------------------------\n");
    printf("Dinheiro Total guardado no Banco: R$ %.2f\n", patrimonioTotal);
    printf("========================================================\n\n");

    // Libera a memória e fecha os arquivos com segurança
    free(children);
    cleanup();
    return 0;
}
