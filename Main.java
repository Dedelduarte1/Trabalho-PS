import java.io.IOException;
import javax.swing.SwingUtilities;

/**
 * Ponto de entrada do simulador.
 * Uso: java Main [caminho_do_executor]   (padrao: ./executor)
 *
 * Para usar o painel de memoria da parte 6, troque
 * "new PainelMemoriaSimples()" pela classe do Vitor que implementa VisaoMemoria.
 */
public class Main {

    public static void main(String[] args) {
        final String executavel = args.length > 0 ? args[0] : "./executor";

        SwingUtilities.invokeLater(() -> {
            ExecutorClient cliente = new ExecutorClient();
            boolean iniciou = true;
            String motivo = "";
            try {
                cliente.iniciar(executavel);
            } catch (IOException e) {
                iniciou = false;
                motivo = e.getMessage();
            }

            JanelaPrincipal janela = new JanelaPrincipal(cliente, new PainelMemoriaSimples());
            janela.setVisible(true);
            if (!iniciou) {
                janela.avisarErro("Nao foi possivel iniciar o executor '" + executavel + "':\n" + motivo);
            }
        });
    }
}
