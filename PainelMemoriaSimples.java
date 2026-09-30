import java.awt.Font;
import javax.swing.JComponent;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;

/** Implementacao provisoria de VisaoMemoria: um dump hexadecimal em texto. */
public class PainelMemoriaSimples implements VisaoMemoria {

    private final JTextArea area = new JTextArea();
    private final JScrollPane rolagem = new JScrollPane(area);

    public PainelMemoriaSimples() {
        area.setEditable(false);
        area.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 13));
    }

    @Override
    public JComponent componente() {
        return rolagem;
    }

    @Override
    public void atualizar(int enderecoInicial, String hex) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i + 1 < hex.length(); i += 2) {
            int deslocamento = i / 2;
            if (deslocamento % 16 == 0) {
                if (deslocamento > 0) {
                    sb.append('\n');
                }
                sb.append(String.format("%06X: ", enderecoInicial + deslocamento));
            }
            sb.append(hex, i, i + 2).append(' ');
        }
        area.setText(sb.toString());
        area.setCaretPosition(0);
    }

    @Override
    public void limpar() {
        area.setText("");
    }
}
