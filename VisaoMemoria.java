import javax.swing.JComponent;

/**
 * Ponto de encaixe entre a janela principal (parte 5) e a apresentacao
 * grafica da memoria (parte 6). Quem faz o painel de memoria implementa
 * esta interface e a janela principal passa a usa-la sem mudar mais nada.
 */
public interface VisaoMemoria {

    /** Componente Swing que a janela principal coloca no centro da tela. */
    JComponent componente();

    /**
     * Mostra um trecho da memoria.
     *
     * @param enderecoInicial endereco do primeiro byte do trecho
     * @param hex bytes em hexadecimal, sem espacos (2 caracteres por byte)
     */
    void atualizar(int enderecoInicial, String hex);

    /** Limpa a exibicao. */
    void limpar();
}
