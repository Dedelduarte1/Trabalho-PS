/*
 * main.c - janela principal do simulador SIC/XE em Raylib (parte 5: layout e integracao).
 *
 * Layout:
 *   topo     - botoes (Passo, Executar/Pausar, Resetar) e caixa do endereco da memoria
 *   esquerda - registradores (o que mudou no ultimo passo fica amarelo)
 *   centro   - painel de memoria (memview.c, parte 6); a roda do mouse rola a memoria
 *   baixo    - log das instrucoes executadas
 *
 * Como carregar um programa: arraste o arquivo para a janela, ou passe o caminho
 * como argumento (./simulador programa.obj). O Raylib nao tem caixa de dialogo de arquivo.
 *
 * Integracao: a janela so usa as funcoes sim_* de sicxe.h e memview_* de memview.h.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memview.h"
#include "raylib.h"
#include "sicxe.h"

#define LARGURA      1000
#define ALTURA       680
#define NUM_REGS     9
#define LOG_MAX      200
#define LOG_LINHA    160
#define LOG_VISIVEIS 6
#define INTERVALO_S  0.15f /* tempo entre passos no modo Executar */

static const char *NOMES[NUM_REGS] = {"A", "X", "L", "B", "S", "T", "F", "PC", "SW"};

static const Rectangle AREA_MEM = {280, 82, 710, 410};
static const Rectangle CAIXA_END = {590, 10, 110, 36};

typedef struct {
    Maquina *m;
    int carregado;              /* ja carregou algum programa? */
    int executando;             /* modo Executar ligado? */
    float acumulado;            /* tempo desde o ultimo passo automatico */
    uint32_t mem_inicio;        /* primeiro endereco mostrado no painel de memoria */
    char antes[NUM_REGS][16];   /* registradores antes do ultimo passo */
    int tem_antes;
    char log[LOG_MAX][LOG_LINHA];
    int log_n;
    int caixa_ativa;            /* caixa de endereco em edicao? */
    char caixa[8];
    int caixa_n;
} App;

/* ---------- utilitarios ---------- */

static void log_texto(App *a, const char *texto) {
    if (a->log_n == LOG_MAX) {
        memmove(a->log[0], a->log[1], (size_t)(LOG_MAX - 1) * LOG_LINHA);
        a->log_n--;
    }
    snprintf(a->log[a->log_n++], LOG_LINHA, "%s", texto);
}

/* Escreve o valor do registrador i em hexadecimal com largura fixa. */
static void valor_reg(const Maquina *m, int i, char *buf, size_t n) {
    switch (i) {
        case 0: snprintf(buf, n, "%06X", (unsigned)(m->A & MASK24)); break;
        case 1: snprintf(buf, n, "%06X", (unsigned)(m->X & MASK24)); break;
        case 2: snprintf(buf, n, "%06X", (unsigned)(m->L & MASK24)); break;
        case 3: snprintf(buf, n, "%06X", (unsigned)(m->B & MASK24)); break;
        case 4: snprintf(buf, n, "%06X", (unsigned)(m->S & MASK24)); break;
        case 5: snprintf(buf, n, "%06X", (unsigned)(m->T & MASK24)); break;
        case 6: snprintf(buf, n, "%012llX", (unsigned long long)(m->F & MASK48)); break;
        case 7: snprintf(buf, n, "%06X", (unsigned)(m->PC & MASK24)); break;
        default: snprintf(buf, n, "%06X", (unsigned)(m->SW & MASK24)); break;
    }
}

static void guardar_registradores(App *a) {
    for (int i = 0; i < NUM_REGS; i++) valor_reg(a->m, i, a->antes[i], sizeof a->antes[i]);
    a->tem_antes = 1;
}

static void esquecer_mudancas(App *a) {
    a->tem_antes = 0;
    memview_esquecer_mudancas();
}

/* ---------- acoes ---------- */

static int exige_programa(App *a) {
    if (!a->carregado) {
        log_texto(a, "Carregue um programa primeiro (arraste o arquivo para a janela).");
        return 0;
    }
    return 1;
}

/* Executa uma instrucao, registra no log e devolve o codigo de sim_passo. */
static int dar_passo(App *a) {
    char desc[LOG_LINHA - 16] = "";

    guardar_registradores(a);
    memview_antes_do_passo(a->m);

    int r = sim_passo(a->m, desc, sizeof desc);
    if (r == SIM_ERRO) {
        char msg[LOG_LINHA];
        snprintf(msg, sizeof msg, "ERRO: %s", desc);
        log_texto(a, msg);
    } else if (desc[0] != '\0') {
        log_texto(a, desc);
    }
    return r;
}

static void carregar_programa(App *a, const char *caminho) {
    char erro[120] = "";
    char msg[LOG_LINHA];

    a->executando = 0;
    if (sim_carregar(a->m, caminho, erro, sizeof erro) == 0) {
        snprintf(msg, sizeof msg, "Programa carregado: %s", GetFileName(caminho));
        a->carregado = 1;
        esquecer_mudancas(a);
    } else {
        snprintf(msg, sizeof msg, "ERRO ao carregar: %s", erro);
    }
    log_texto(a, msg);
}

static void resetar(App *a) {
    a->executando = 0;
    sim_reset(a->m);
    esquecer_mudancas(a);
    log_texto(a, "-- registradores zerados --");
}

/* Aplica o endereco digitado na caixa (hexadecimal) ao painel de memoria. */
static void aplicar_endereco(App *a) {
    unsigned long v = a->caixa_n > 0 ? strtoul(a->caixa, NULL, 16) : 0;
    if (v >= a->m->mem_tam) {
        log_texto(a, "Endereco fora da memoria (maximo FFFFF).");
        return;
    }
    a->mem_inicio = (uint32_t)v & ~(uint32_t)(MEMVIEW_COLUNAS - 1);
}

/* ---------- entrada (teclado, mouse, arquivos) ---------- */

static void tratar_entrada(App *a) {
    Vector2 mouse = GetMousePosition();

    if (IsFileDropped()) {
        FilePathList arquivos = LoadDroppedFiles();
        if (arquivos.count > 0) carregar_programa(a, arquivos.paths[0]);
        UnloadDroppedFiles(arquivos);
    }

    /* caixa de texto do endereco */
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        a->caixa_ativa = CheckCollisionPointRec(mouse, CAIXA_END);
    }
    if (a->caixa_ativa) {
        int c;
        while ((c = GetCharPressed()) > 0) {
            if (c < 128 && isxdigit(c) && a->caixa_n < 6) {
                a->caixa[a->caixa_n++] = (char)toupper(c);
                a->caixa[a->caixa_n] = '\0';
            }
        }
        if (IsKeyPressed(KEY_BACKSPACE) && a->caixa_n > 0) a->caixa[--a->caixa_n] = '\0';
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            aplicar_endereco(a);
            a->caixa_ativa = 0;
        }
    } else if (IsKeyPressed(KEY_SPACE)) {
        if (exige_programa(a)) dar_passo(a);
    }

    /* roda do mouse rola a memoria (3 linhas por clique) */
    float roda = GetMouseWheelMove();
    if (roda != 0.0f && CheckCollisionPointRec(mouse, AREA_MEM)) {
        long novo = (long)a->mem_inicio - (long)(roda * 3 * MEMVIEW_COLUNAS);
        long maximo = (long)a->m->mem_tam - MEMVIEW_COLUNAS;
        if (novo < 0) novo = 0;
        if (novo > maximo) novo = maximo;
        a->mem_inicio = (uint32_t)novo;
    }

    /* execucao automatica, animada */
    if (a->executando) {
        a->acumulado += GetFrameTime();
        if (a->acumulado >= INTERVALO_S) {
            a->acumulado = 0.0f;
            if (dar_passo(a) != SIM_OK) a->executando = 0;
        }
    }
}

/* ---------- desenho ---------- */

/* Desenha um botao e devolve 1 no frame em que ele e clicado. */
static int botao(Rectangle r, const char *texto, int destacado) {
    int sobre = CheckCollisionPointRec(GetMousePosition(), r);
    Color fundo = sobre ? (Color){70, 110, 190, 255} : (Color){55, 85, 150, 255};
    if (destacado) fundo = (Color){190, 110, 40, 255};

    DrawRectangleRec(r, fundo);
    DrawRectangleLinesEx(r, 1, LIGHTGRAY);
    int w = MeasureText(texto, 20);
    DrawText(texto, (int)(r.x + (r.width - (float)w) / 2), (int)(r.y + (r.height - 20) / 2), 20, WHITE);
    return sobre && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

static void desenhar_barra(App *a) {
    if (botao((Rectangle){10, 10, 100, 36}, "Passo", 0)) {
        if (exige_programa(a)) dar_passo(a);
    }
    if (botao((Rectangle){120, 10, 120, 36}, a->executando ? "Pausar" : "Executar", a->executando)) {
        if (a->executando) {
            a->executando = 0;
        } else if (exige_programa(a)) {
            a->executando = 1;
            a->acumulado = INTERVALO_S; /* primeiro passo imediato */
        }
    }
    if (botao((Rectangle){250, 10, 100, 36}, "Resetar", 0)) resetar(a);

    DrawText("Memoria a partir de (hex):", 370, 20, 16, LIGHTGRAY);
    DrawRectangleRec(CAIXA_END, WHITE);
    DrawRectangleLinesEx(CAIXA_END, a->caixa_ativa ? 2 : 1, a->caixa_ativa ? ORANGE : GRAY);
    DrawText(a->caixa, (int)CAIXA_END.x + 8, (int)CAIXA_END.y + 8, 20, BLACK);
    if (a->caixa_ativa && ((int)(GetTime() * 2) % 2 == 0)) {
        int w = MeasureText(a->caixa, 20);
        DrawRectangle((int)CAIXA_END.x + 10 + w, (int)CAIXA_END.y + 8, 2, 20, BLACK);
    }

    DrawText("Arraste o programa", 715, 10, 14, GRAY);
    DrawText("para a janela. Espaco: passo", 715, 28, 14, GRAY);
}

static void desenhar_registradores(App *a) {
    DrawText("Registradores", 10, 60, 18, LIGHTGRAY);
    DrawRectangle(10, 82, 260, 326, (Color){24, 26, 33, 255});
    DrawRectangleLines(10, 82, 260, 326, GRAY);

    for (int i = 0; i < NUM_REGS; i++) {
        char atual[16];
        valor_reg(a->m, i, atual, sizeof atual);
        int mudou = a->tem_antes && strcmp(atual, a->antes[i]) != 0;

        int y = 82 + 14 + i * 34;
        Rectangle caixa = {80, (float)y, 175, 26};
        DrawText(NOMES[i], 24, y + 3, 20, WHITE);
        DrawRectangleRec(caixa, mudou ? (Color){255, 235, 130, 255} : (Color){245, 245, 245, 255});
        DrawRectangleLinesEx(caixa, 1, GRAY);
        DrawText(atual, (int)caixa.x + 8, y + 3, 20, BLACK);
    }
}

static void desenhar_memoria(App *a) {
    int linhas = memview_linhas_visiveis(AREA_MEM);
    uint32_t fim = a->mem_inicio + (uint32_t)linhas * MEMVIEW_COLUNAS - 1;
    if (fim >= a->m->mem_tam) fim = (uint32_t)a->m->mem_tam - 1;

    char titulo[64];
    snprintf(titulo, sizeof titulo, "Memoria   %06X - %06X   (amarelo = mudou, azul = PC)",
             (unsigned)a->mem_inicio, (unsigned)fim);
    DrawText(titulo, (int)AREA_MEM.x, 60, 18, LIGHTGRAY);
    memview_desenhar(a->m, AREA_MEM, a->mem_inicio);
}

static void desenhar_log(App *a) {
    DrawText("Execucao", 10, 500, 18, LIGHTGRAY);
    DrawRectangle(10, 522, 980, 148, (Color){24, 26, 33, 255});
    DrawRectangleLines(10, 522, 980, 148, GRAY);

    int primeira = a->log_n > LOG_VISIVEIS ? a->log_n - LOG_VISIVEIS : 0;
    for (int i = primeira; i < a->log_n; i++) {
        DrawText(a->log[i], 20, 530 + (i - primeira) * 22, 18, (Color){200, 210, 220, 255});
    }
}

int main(int argc, char **argv) {
    App a;
    memset(&a, 0, sizeof a);

    a.m = sim_criar();
    if (!a.m) {
        fprintf(stderr, "Falha ao criar a maquina (memoria insuficiente?)\n");
        return 1;
    }
    strcpy(a.caixa, "000000");
    a.caixa_n = 6;

    InitWindow(LARGURA, ALTURA, "Simulador SIC/XE");
    SetTargetFPS(60);

    log_texto(&a, "Pronto. Arraste o arquivo do programa para a janela para carregar.");
    if (argc > 1) carregar_programa(&a, argv[1]);

    while (!WindowShouldClose()) {
        tratar_entrada(&a);

        BeginDrawing();
        ClearBackground((Color){30, 32, 40, 255});
        desenhar_barra(&a);
        desenhar_registradores(&a);
        desenhar_memoria(&a);
        desenhar_log(&a);
        EndDrawing();
    }

    CloseWindow();
    sim_destruir(a.m);
    return 0;
}
