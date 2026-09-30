# Compilar: make    |    Rodar: ./simulador [programa]
# Quando o simulador real estiver pronto, troque sim_stub.c pelos .c das partes 1 a 4 em FONTES.
#
# Linux : precisa do raylib instalado. As bibliotecas extras (-lm, -lGL, ...) sao necessarias
#         quando o raylib e a versao estatica (.a), que e o caso da instalacao padrao.
# Windows (w64devkit + raylib): troque LIBS por  -lraylib -lopengl32 -lgdi32 -lwinmm
CC = gcc
CFLAGS = -Wall -Wextra -g $(shell pkg-config --cflags raylib 2>/dev/null)
LIBS = $(shell pkg-config --libs raylib 2>/dev/null || echo -lraylib) -lGL -lm -lpthread -ldl -lrt -lX11
FONTES = main.c memview.c sim_stub.c

simulador: $(FONTES) sicxe.h memview.h
	$(CC) $(CFLAGS) -o simulador $(FONTES) $(LIBS)

clean:
	rm -f simulador
