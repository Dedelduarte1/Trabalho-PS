import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.GridLayout;
import java.awt.FlowLayout;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.File;
import java.util.LinkedHashMap;
import java.util.Map;
import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;
import javax.swing.SwingWorker;

/**
 * Janela principal do simulador SIC/XE (parte 5: layout e integracao).
 *
 * Layout:
 *   NORTE  - barra de botoes (carregar, passo, executar, resetar) e endereco da memoria
 *   OESTE  - registradores (o que mudou no ultimo passo fica destacado)
 *   CENTRO - painel de memoria (qualquer VisaoMemoria; parte 6)
 *   SUL    - log das instrucoes executadas e mensagens
 */
public class JanelaPrincipal extends JFrame {

    private static final int BYTES_MEMORIA = 256;
    private static final Color COR_NORMAL = Color.WHITE;
    private static final Color COR_DESTAQUE = new Color(255, 235, 130);

    private final ExecutorClient cliente;
    private final VisaoMemoria visaoMemoria;

    private final Map<String, JLabel> valoresRegistradores = new LinkedHashMap<>();
    private final Map<String, String> valoresAnteriores = new LinkedHashMap<>();
    private final JTextArea log = new JTextArea();
    private final JTextField campoEnderecoMem = new JTextField("000000", 7);

    private final JButton btnCarregar = new JButton("Carregar programa");
    private final JButton btnPasso = new JButton("Passo");
    private final JButton btnExecutar = new JButton("Executar tudo");
    private final JButton btnResetar = new JButton("Resetar");

    public JanelaPrincipal(ExecutorClient cliente, VisaoMemoria visaoMemoria) {
        super("Simulador SIC/XE");
        this.cliente = cliente;
        this.visaoMemoria = visaoMemoria;

        setDefaultCloseOperation(EXIT_ON_CLOSE);
        setLayout(new BorderLayout(8, 8));
        add(criarBarra(), BorderLayout.NORTH);
        add(criarPainelRegistradores(), BorderLayout.WEST);
        add(criarPainelMemoria(), BorderLayout.CENTER);
        add(criarPainelLog(), BorderLayout.SOUTH);

        btnCarregar.addActionListener(e -> escolherECarregar());
        btnPasso.addActionListener(e -> executar("STEP"));
        btnExecutar.addActionListener(e -> executar("RUN"));
        btnResetar.addActionListener(e -> executar("RESET"));

        addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(WindowEvent e) {
                cliente.close();
            }
        });

        habilitarBotoes(cliente.estaAtivo());
        setSize(900, 620);
        setLocationRelativeTo(null);
    }

    // ---------- montagem da tela ----------

    private JPanel criarBarra() {
        JPanel barra = new JPanel(new FlowLayout(FlowLayout.LEFT, 8, 6));
        barra.add(btnCarregar);
        barra.add(btnPasso);
        barra.add(btnExecutar);
        barra.add(btnResetar);
        barra.add(new JLabel("   Memoria a partir de (hex):"));
        barra.add(campoEnderecoMem);
        return barra;
    }

    private JPanel criarPainelRegistradores() {
        JPanel grade = new JPanel(new GridLayout(0, 2, 8, 6));
        Font mono = new Font(Font.MONOSPACED, Font.PLAIN, 14);
        for (String nome : ExecutorClient.Resposta.REGISTRADORES) {
            JLabel rotulo = new JLabel(nome);
            rotulo.setFont(mono.deriveFont(Font.BOLD));

            String zeros = nome.equals("F") ? "000000000000" : "000000";
            JLabel valor = new JLabel(zeros);
            valor.setFont(mono);
            valor.setOpaque(true);
            valor.setBackground(COR_NORMAL);
            valor.setBorder(BorderFactory.createLineBorder(Color.LIGHT_GRAY));

            valoresRegistradores.put(nome, valor);
            grade.add(rotulo);
            grade.add(valor);
        }
        JPanel painel = new JPanel(new BorderLayout());
        painel.setBorder(BorderFactory.createTitledBorder("Registradores"));
        painel.add(grade, BorderLayout.NORTH);
        painel.setPreferredSize(new Dimension(230, 0));
        return painel;
    }

    private JPanel criarPainelMemoria() {
        JPanel painel = new JPanel(new BorderLayout());
        painel.setBorder(BorderFactory.createTitledBorder("Memoria"));
        painel.add(visaoMemoria.componente(), BorderLayout.CENTER);
        return painel;
    }

    private JScrollPane criarPainelLog() {
        log.setEditable(false);
        log.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 12));
        JScrollPane rolagem = new JScrollPane(log);
        rolagem.setBorder(BorderFactory.createTitledBorder("Execucao"));
        rolagem.setPreferredSize(new Dimension(0, 160));
        return rolagem;
    }

    // ---------- acoes ----------

    private void escolherECarregar() {
        JFileChooser chooser = new JFileChooser(new File("."));
        if (chooser.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {
            String caminho = chooser.getSelectedFile().getAbsolutePath();
            registrar("> Carregando " + caminho);
            executar("LOAD " + caminho);
        }
    }

    /**
     * Envia o comando ao executor e, em seguida, pede a memoria para atualizar
     * a tela. Roda fora da thread da interface para a janela nao travar
     * (o comando RUN pode demorar).
     */
    private void executar(String comando) {
        final String enderecoMem = campoEnderecoMem.getText().trim();
        if (!enderecoMem.matches("[0-9A-Fa-f]{1,6}")) {
            registrar("Endereco de memoria invalido: " + enderecoMem);
            return;
        }
        habilitarBotoes(false);

        new SwingWorker<ExecutorClient.Resposta, Void>() {
            @Override
            protected ExecutorClient.Resposta doInBackground() throws Exception {
                ExecutorClient.Resposta r = cliente.enviar(comando);
                if (r.erro == null) {
                    r.juntar(cliente.enviar("MEM " + enderecoMem + " " + BYTES_MEMORIA));
                }
                return r;
            }

            @Override
            protected void done() {
                try {
                    aplicar(get());
                } catch (Exception e) {
                    registrar("Falha na comunicacao com o executor: " + e.getMessage());
                }
                habilitarBotoes(cliente.estaAtivo());
            }
        }.execute();
    }

    /** Atualiza registradores, log e memoria a partir da resposta do executor. */
    private void aplicar(ExecutorClient.Resposta r) {
        if (r.erro != null) {
            registrar("ERRO do executor: " + r.erro);
            return;
        }
        for (String instrucao : r.instrucoes) {
            registrar(instrucao);
        }
        atualizarRegistradores(r.registradores);
        for (ExecutorClient.Resposta.BlocoMemoria bloco : r.memoria) {
            visaoMemoria.atualizar(bloco.endereco, bloco.hex);
        }
    }

    private void atualizarRegistradores(Map<String, String> novos) {
        for (Map.Entry<String, String> e : novos.entrySet()) {
            JLabel rotulo = valoresRegistradores.get(e.getKey());
            if (rotulo == null) {
                continue;
            }
            String anterior = valoresAnteriores.get(e.getKey());
            boolean mudou = anterior != null && !anterior.equals(e.getValue());
            rotulo.setText(e.getValue());
            rotulo.setBackground(mudou ? COR_DESTAQUE : COR_NORMAL);
            valoresAnteriores.put(e.getKey(), e.getValue());
        }
    }

    // ---------- utilitarios ----------

    private void registrar(String texto) {
        log.append(texto + "\n");
        log.setCaretPosition(log.getDocument().getLength());
    }

    private void habilitarBotoes(boolean ativo) {
        btnCarregar.setEnabled(ativo);
        btnPasso.setEnabled(ativo);
        btnExecutar.setEnabled(ativo);
        btnResetar.setEnabled(ativo);
    }

    public void avisarErro(String mensagem) {
        JOptionPane.showMessageDialog(this, mensagem, "Erro", JOptionPane.ERROR_MESSAGE);
    }
}
