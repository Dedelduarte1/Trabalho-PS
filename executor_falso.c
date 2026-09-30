/*
 * Executor FALSO, so para testar a interface Java.
 * Fala o mesmo protocolo do executor real, mas nao simula o SIC/XE:
 * a cada passo soma 1 em A e 3 em PC.
 *
 * Compilar: gcc -o executor executor_falso.c
 */
#include <stdio.h>
#include <string.h>

static unsigned A, X, L, B, S, T, PC, SW;

static void zerar(void) { A = X = L = B = S = T = PC = SW = 0; }

static void estado(void) {
    printf("STATE A=%06X X=%06X L=%06X B=%06X S=%06X T=%06X F=%012X PC=%06X SW=%06X\n",
           A, X, L, B, S, T, 0u, PC, SW);
}

static void passo(void) {
    printf("INSTR ADD #1 (simulado) em %06X\n", PC);
    A += 1;
    PC += 3;
}

int main(void) {
    char linha[1024];
    char cmd[16];

    while (fgets(linha, sizeof linha, stdin)) {
        cmd[0] = '\0';
        sscanf(linha, "%15s", cmd);

        if (strcmp(cmd, "LOAD") == 0) {
            zerar();
            printf("INSTR programa carregado (simulado)\n");
            estado();
        } else if (strcmp(cmd, "STEP") == 0) {
            passo();
            estado();
        } else if (strcmp(cmd, "RUN") == 0) {
            for (int i = 0; i < 10; i++) passo();
            estado();
        } else if (strcmp(cmd, "RESET") == 0) {
            zerar();
            estado();
        } else if (strcmp(cmd, "MEM") == 0) {
            unsigned ini, qtd;
            if (sscanf(linha, "%*s %x %u", &ini, &qtd) == 2) {
                printf("MEM %06X ", ini);
                for (unsigned i = 0; i < qtd; i++) printf("%02X", (ini + i) & 0xFF);
                printf("\n");
            } else {
                printf("ERR uso: MEM <endereco_hex> <quantidade>\n");
            }
        } else {
            printf("ERR comando desconhecido\n");
        }

        printf("END\n");
        fflush(stdout); /* sem isso o Java nunca recebe a resposta */
    }
    return 0;
}
