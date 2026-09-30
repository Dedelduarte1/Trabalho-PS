import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * Ponte entre a interface Java e o executor (simulador SIC/XE) escrito em C.
 *
 * O executor roda como processo filho. A comunicacao e por texto, uma linha
 * por comando (stdin do C) e varias linhas de resposta terminadas em "END"
 * (stdout do C).
 *
 * Comandos: LOAD <caminho> | STEP | RUN | RESET | MEM <endereco_hex> <quantidade>
 * Respostas: STATE A=.. X=.. ... | INSTR <texto> | MEM <endereco_hex> <bytes_hex> | ERR <msg> | END
 */
public class ExecutorClient implements AutoCloseable {

    /** Resultado de um comando, ja interpretado. */
    public static class Resposta {
        public static final String[] REGISTRADORES =
                {"A", "X", "L", "B", "S", "T", "F", "PC", "SW"};

        public final Map<String, String> registradores = new LinkedHashMap<>();
        public final List<String> instrucoes = new ArrayList<>();
        public final List<BlocoMemoria> memoria = new ArrayList<>();
        public String erro;

        public static class BlocoMemoria {
            public final int endereco;
            public final String hex;

            BlocoMemoria(int endereco, String hex) {
                this.endereco = endereco;
                this.hex = hex;
            }
        }

        void processarLinha(String linha) {
            if (linha.startsWith("STATE ")) {
                for (String par : linha.substring(6).trim().split("\\s+")) {
                    int i = par.indexOf('=');
                    if (i > 0) {
                        registradores.put(par.substring(0, i), par.substring(i + 1));
                    }
                }
            } else if (linha.startsWith("INSTR ")) {
                instrucoes.add(linha.substring(6));
            } else if (linha.startsWith("MEM ")) {
                String[] p = linha.split("\\s+", 3);
                try {
                    if (p.length == 3) {
                        memoria.add(new BlocoMemoria(Integer.parseInt(p[1], 16), p[2].trim()));
                    }
                } catch (NumberFormatException e) {
                    erro = "Linha MEM invalida: " + linha;
                }
            } else if (linha.startsWith("ERR")) {
                erro = linha.length() > 4 ? linha.substring(4) : "erro desconhecido";
            }
        }

        /** Acrescenta o conteudo de outra resposta a esta. */
        void juntar(Resposta outra) {
            registradores.putAll(outra.registradores);
            instrucoes.addAll(outra.instrucoes);
            memoria.addAll(outra.memoria);
            if (erro == null) {
                erro = outra.erro;
            }
        }
    }

    private Process processo;
    private BufferedWriter saida;
    private BufferedReader entrada;

    /** Inicia o executor. Ex.: iniciar("./executor"). */
    public void iniciar(String... comando) throws IOException {
        ProcessBuilder pb = new ProcessBuilder(comando);
        pb.redirectError(ProcessBuilder.Redirect.INHERIT); // erros do C aparecem no console
        processo = pb.start();
        saida = new BufferedWriter(
                new OutputStreamWriter(processo.getOutputStream(), StandardCharsets.UTF_8));
        entrada = new BufferedReader(
                new InputStreamReader(processo.getInputStream(), StandardCharsets.UTF_8));
    }

    public boolean estaAtivo() {
        return processo != null && processo.isAlive();
    }

    /** Envia um comando e le as linhas de resposta ate o "END". Bloqueante. */
    public synchronized Resposta enviar(String comando) throws IOException {
        if (!estaAtivo()) {
            throw new IOException("Executor nao esta rodando");
        }
        saida.write(comando);
        saida.newLine();
        saida.flush();

        Resposta resposta = new Resposta();
        String linha;
        while ((linha = entrada.readLine()) != null) {
            if (linha.equals("END")) {
                return resposta;
            }
            resposta.processarLinha(linha);
        }
        throw new IOException("Executor encerrou sem enviar END");
    }

    @Override
    public void close() {
        if (processo != null) {
            processo.destroy();
        }
    }
}
