/*
 * sicxe.h - contrato entre a INTERFACE (partes 5 e 6) e o SIMULADOR (partes 1 a 4).
 *
 * A interface so conhece o que esta neste arquivo. Quem implementa o simulador
 * precisa fornecer as funcoes sim_* declaradas abaixo (em qualquer .c seu).
 * Enquanto o simulador real nao fica pronto, sim_stub.c faz um simulador falso.
 */
#ifndef SICXE_H
#define SICXE_H

#include <stddef.h>
#include <stdint.h>

#define MEM_TAMANHO (1u << 20)        /* 1 MB: enderecos de 20 bits (formato 4) */
#define MASK24      0xFFFFFFu         /* registradores de 24 bits */
#define MASK48      0xFFFFFFFFFFFFull /* registrador F de 48 bits */

/* Resultado de sim_passo */
enum { SIM_OK = 0, SIM_FIM = 1, SIM_ERRO = -1 };

/* Estado da maquina. Os registradores usam so os 24 bits (F: 48 bits) de baixo. */
typedef struct {
    uint32_t A, X, L, B, S, T, PC, SW;
    uint64_t F;
    uint8_t *mem;      /* memoria, em bytes */
    size_t   mem_tam;  /* tamanho da memoria em bytes */
} Maquina;

/* Cria a maquina com a memoria alocada e zerada. Retorna NULL se falhar. */
Maquina *sim_criar(void);

/* Libera a maquina. */
void sim_destruir(Maquina *m);

/* Zera registradores (inclusive PC). A memoria e mantida. */
void sim_reset(Maquina *m);

/* Carrega um programa do arquivo para a memoria e zera registradores.
 * Retorna 0 em sucesso; em falha retorna != 0 e escreve a mensagem em erro. */
int sim_carregar(Maquina *m, const char *caminho, char *erro, size_t erro_tam);

/* Executa UMA instrucao.
 * desc recebe um texto curto da instrucao executada (ex.: "000000: LDA #5"),
 * ou a mensagem de erro se o retorno for SIM_ERRO.
 * Retorna SIM_OK (pode continuar), SIM_FIM (programa terminou) ou SIM_ERRO. */
int sim_passo(Maquina *m, char *desc, size_t desc_tam);

#endif
