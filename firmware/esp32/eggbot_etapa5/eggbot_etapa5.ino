// EggBot — Etapa 5: configuração, calibração e interface final.
// Parte da etapa 4 (desenho → arquivo → impressão pela serial) e acrescenta a configuração da
// máquina gravada no ESP32 (posições da caneta, tempo do servo, velocidades e limites do braço),
// a calibração guiada (o ESP32 acompanha a posição do braço) e a página com abas funcionando,
// padrões prontos e importação de SVG.
// Placa: ESP32 Dev Module (pacote "esp32 by Espressif Systems").
// Tools → Partition Scheme → "Default 4MB with spiffs" (o LittleFS usa essa partição).
//
// Ligação: Uno TX (D1) → divisor 1 kΩ / 2 kΩ → GPIO16 (RX2); Uno RX (D0) ← GPIO17 (TX2); GND comum.

#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <math.h>
#include <stdarg.h>
#include "pagina.h"

// Senha da rede do EggBot (mínimo de 8 caracteres). Para usar uma senha própria sem publicá-la,
// crie "senha_wifi.h" nesta pasta (modelo em ../senha_wifi.exemplo.h; o .gitignore o deixa fora do git).
#if __has_include("senha_wifi.h")
#include "senha_wifi.h"
#else
const char SENHA_WIFI[] = "eggbot123";  // senha padrão, pública: troquem
#endif

// Serial com o Uno. Se a placa for WROVER (GPIO16/17 usados pela PSRAM), trocar para 26 e 27.
const int PINO_RX_UNO = 16;
const int PINO_TX_UNO = 17;
const unsigned long BAUD_UNO = 9600;

const long LIMITE_ROT = 6400;              // rotação aceita no arquivo: ±6400 passos (2 voltas)

const unsigned long LIMITE_BYTES = 65536;  // tamanho máximo do arquivo (64 KB)
const char *ARQ_JOB = "/job.txt";
const char *ARQ_TMP = "/job.tmp";

WebServer server(80);
char ssid[16];  // "EggBot-XXXX"
bool fsOk = false;
uint32_t tabelaCrc[256];

// ---------- Tipos ----------

// Configuração da máquina (gravada na flash do ESP32; ajustada na aba Configuração)
struct Config {
  long pen_up = 60, pen_down = 40;                 // posição do servo em % (0–100)
  unsigned long pen_ms = 500;                      // espera em cada SP (ms)
  unsigned long speed_down = 300, speed_up = 800;  // passos/s com a caneta abaixada / levantada
  long arm_min = -400, arm_max = 400;              // limites do braço, em passos a partir do centro
};
Config cfg;

// Monta um JSON num buffer fixo, sem estourar o tamanho.
struct Json {
  char *buf;
  size_t cap, len;
  Json(char *b, size_t c) : buf(b), cap(c), len(0) { buf[0] = 0; }
  void add(const char *fmt, ...) {
    if (len + 1 >= cap) return;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf + len, cap - len, fmt, ap);
    va_end(ap);
    if (n > 0) len = (len + n < cap) ? len + n : cap - 1;
  }
  // Texto dentro de aspas, escapando " e \ (caracteres de controle viram '?').
  void texto(const char *s) {
    for (; *s; s++) {
      unsigned char c = *s;
      if (c == '"' || c == '\\') add("\\%c", c);
      else if (c < 0x20) add("?");
      else add("%c", c);
    }
  }
};

// Estatísticas do arquivo aceito.
struct Resumo {
  bool present;
  char name[25];
  unsigned long bytes, lines, points, strokes, crc, ebb_cmds, est_s, est_ms, upload_ms, parse_ms;
};

// Lê o arquivo linha por linha (em blocos, nunca inteiro na RAM) e vai calculando o CRC-32.
struct Leitor {
  File &f;
  uint8_t bloco[256];
  int n = 0, pos = 0;
  char linha[41];  // buffer de 40 caracteres + '\0'
  int len = 0;
  bool longa = false;  // a linha tinha mais de 40 caracteres
  uint32_t crc = 0xFFFFFFFF;
  Leitor(File &arq) : f(arq) {}
  void reinicia() {
    n = pos = len = 0;
    crc = 0xFFFFFFFF;
  }
  // Retorna false se o arquivo acabou sem nenhum byte novo.
  bool proxima() {
    len = 0;
    longa = false;
    bool leu = false;
    while (true) {
      if (pos >= n) {
        n = f.read(bloco, sizeof(bloco));
        pos = 0;
        if (n <= 0) { n = 0; break; }
      }
      uint8_t c = bloco[pos++];
      crc = tabelaCrc[(crc ^ c) & 0xFF] ^ (crc >> 8);
      leu = true;
      if (c == '\n') break;
      if (len < 40) linha[len++] = c;
      else longa = true;
    }
    linha[len] = 0;
    return leu;
  }
  bool igual(const char *s) { return !longa && len == (int)strlen(s) && memcmp(linha, s, len) == 0; }
};

// Fila pequena de comandos EBB prontos para enviar ao Uno.
struct Fila {
  // Pior caso de uma linha L: rotação 12800 + braço 3200 → ~13200 passos; a 50 passos/s (mínimo
  // configurável) são ~264 s, em SM de no máximo 2 s → 132 comandos. 160 dá folga.
  static const int TAM = 160;
  char cmd[TAM][24];
  unsigned long ms[TAM];
  int ini = 0, qtd = 0;
  void limpa() { ini = qtd = 0; }
  void poe(const char *c, unsigned long d) {
    if (qtd >= TAM) return;
    int i = (ini + qtd) % TAM;
    strncpy(cmd[i], c, sizeof(cmd[i]) - 1);
    cmd[i][sizeof(cmd[i]) - 1] = 0;
    ms[i] = d;
    qtd++;
  }
  bool tira(char *c, unsigned long &d) {
    if (qtd == 0) return false;
    strcpy(c, cmd[ini]);
    d = ms[ini];
    ini = (ini + 1) % TAM;
    qtd--;
    return true;
  }
};

// Converte o desenho em comandos EBB para o EggDuino. Sempre conta comandos e tempo;
// se "fila" estiver definida, também guarda os comandos nela.
struct Conversor {
  bool caneta = false;  // começa levantada
  long rot = 0, braco = 0;
  unsigned long cmds = 0, ms = 0;
  Fila *fila = nullptr;

  void emite(const char *c, unsigned long d) {
    if (fila) fila->poe(c, d);
    cmds++;
    ms += d;
  }
  void inicio() { emite("EM,1", 0); }
  void sobe() {
    if (caneta) {
      char c[16];
      snprintf(c, sizeof(c), "SP,0,%lu", cfg.pen_ms);
      emite(c, cfg.pen_ms);
      caneta = false;
    }
  }
  void desce() {
    if (!caneta) {
      char c[16];
      snprintf(c, sizeof(c), "SP,1,%lu", cfg.pen_ms);
      emite(c, cfg.pen_ms);
      caneta = true;
    }
  }
  void vai(long r, long a) {
    long dr = r - rot, da = a - braco;
    if (dr == 0 && da == 0) return;
    double velocidade = caneta ? cfg.speed_down : cfg.speed_up;  // passos/s
    unsigned long dur = (unsigned long)ceil(hypot((double)dr, (double)da) / velocidade * 1000);
    if (dur < 10) dur = 10;
    unsigned long partes = (dur + 1999) / 2000;         // nenhum SM com mais de 2 s
    unsigned long passo = (dur + partes - 1) / partes;  // ceil(dur / partes)
    long crAnt = 0, caAnt = 0;
    char c[32];
    for (unsigned long k = 1; k <= partes; k++) {
      long cr = (long)floor((double)dr * k / partes + 0.5);  // igual ao Math.round do navegador
      long ca = (long)floor((double)da * k / partes + 0.5);
      snprintf(c, sizeof(c), "SM,%lu,%ld,%ld", passo, ca - caAnt, cr - crAnt);  // braço antes de rotação
      emite(c, passo);
      crAnt = cr;
      caAnt = ca;
    }
    rot = r;
    braco = a;
  }
  // Fim do desenho: caneta sobe, volta à origem e desliga os motores
  void fim() {
    sobe();
    vai(0, 0);
    emite("EM,0", 0);
  }
};

Resumo job;  // o arquivo que está no EggBot (/job.txt)

// ---------- CRC-32 IEEE ----------

void iniciaCrc() {
  for (uint32_t i = 0; i < 256; i++) {
    uint32_t c = i;
    for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320 ^ (c >> 1) : c >> 1;
    tabelaCrc[i] = c;
  }
}

// ---------- Validação ----------

// Lê um inteiro com sinal opcional e 1 a 5 dígitos; avança o ponteiro.
bool lerInteiro(const char *&s, long &v) {
  bool neg = (*s == '-');
  if (neg) s++;
  int dig = 0;
  v = 0;
  while (*s >= '0' && *s <= '9' && dig < 6) {
    v = v * 10 + (*s - '0');
    s++;
    dig++;
  }
  if (dig < 1 || dig > 5) return false;
  if (neg) v = -v;
  return true;
}

// "L <rot> <braço>" com exatamente um espaço entre os campos.
bool lerLinhaL(Leitor &L, long &r, long &a) {
  if (L.longa || L.len < 5 || L.linha[0] != 'L' || L.linha[1] != ' ') return false;
  const char *s = L.linha + 2;
  if (!lerInteiro(s, r) || *s != ' ') return false;
  s++;
  if (!lerInteiro(s, a)) return false;
  return s == L.linha + L.len;
}

// Valida o arquivo linha por linha, preenche o resumo e alimenta o conversor.
// Em caso de erro, escreve a mensagem em "erro" e retorna false.
bool validaArquivo(File &f, Resumo &r, Conversor &conv, char *erro, size_t cap) {
  Leitor L(f);

  // linha 1: cabeçalho
  L.proxima();
  if (!L.igual("EGGJOB 1")) {
    snprintf(erro, cap, "linha 1: cabeçalho deve ser \"EGGJOB 1\"");
    return false;
  }

  // linha 2: nome
  L.proxima();
  bool nomeOk = !L.longa && L.len >= 3 && L.len <= 26 && L.linha[0] == 'N' && L.linha[1] == ' ';
  for (int i = 2; nomeOk && i < L.len; i++) {
    char c = L.linha[i];
    nomeOk = isalnum((unsigned char)c) || c == '_' || c == '-';
  }
  if (!nomeOk) {
    snprintf(erro, cap, "linha 2: nome inválido");
    return false;
  }
  strcpy(r.name, L.linha + 2);

  // linha 3: quantidade de linhas de dados
  L.proxima();
  long P = 0;
  const char *s = L.linha + 2;
  bool pOk = !L.longa && L.len >= 3 && L.linha[0] == 'P' && L.linha[1] == ' ' && *s != '-' &&
             lerInteiro(s, P) && s == L.linha + L.len && P >= 1 && P <= 8000;
  if (!pOk) {
    snprintf(erro, cap, "linha 3: contagem P inválida (1 a 8000)");
    return false;
  }

  // linhas de dados
  r.points = r.strokes = 0;
  conv.inicio();
  for (long k = 4; k <= P + 3; k++) {
    if (!L.proxima()) {
      snprintf(erro, cap, "linha %ld: arquivo terminou antes do esperado", k);
      return false;
    }
    long rot, braco;
    if (L.igual("U")) {
      conv.sobe();
    } else if (L.igual("D")) {
      r.strokes++;
      conv.desce();
    } else if (lerLinhaL(L, rot, braco)) {
      if (labs(rot) > LIMITE_ROT) {
        snprintf(erro, cap, "linha %ld: rotação %ld fora de ±%ld", k, rot, LIMITE_ROT);
        return false;
      }
      if (braco < cfg.arm_min || braco > cfg.arm_max) {
        snprintf(erro, cap, "linha %ld: braço %ld fora dos limites (%ld a %ld)", k, braco, cfg.arm_min, cfg.arm_max);
        return false;
      }
      r.points++;
      conv.vai(rot, braco);
    } else {
      snprintf(erro, cap, "linha %ld: linha inválida \"%.20s\"", k, L.linha);
      return false;
    }
  }
  conv.fim();

  // linha P+4: END <crc>, com o CRC de tudo antes dela
  uint32_t calculado = L.crc ^ 0xFFFFFFFF;
  L.proxima();
  bool endOk = !L.longa && L.len == 12 && memcmp(L.linha, "END ", 4) == 0;
  for (int i = 4; endOk && i < 12; i++) {
    char c = L.linha[i];
    endOk = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
  }
  if (!endOk) {
    snprintf(erro, cap, "linha %ld: esperado \"END <crc>\" (P não confere com o número de linhas?)", P + 4);
    return false;
  }
  unsigned long doArquivo = strtoul(L.linha + 4, nullptr, 16);
  if (doArquivo != calculado) {
    snprintf(erro, cap, "CRC não confere (arquivo diz %08lX, calculado %08lX)", doArquivo, (unsigned long)calculado);
    return false;
  }

  // depois do END só pode haver o '\n' final
  if (L.proxima()) {
    snprintf(erro, cap, "linha %ld: conteúdo depois do END", P + 5);
    return false;
  }

  r.bytes = f.size();
  r.lines = P + 4;
  r.crc = calculado;
  r.ebb_cmds = conv.cmds;
  r.est_ms = conv.ms;
  r.est_s = (conv.ms + 500) / 1000;
  return true;
}

// Abre o arquivo, valida e mede o tempo.
bool processa(const char *caminho, Resumo &r, Conversor &conv, char *erro, size_t cap) {
  unsigned long t0 = millis();
  File f = LittleFS.open(caminho, "r");
  if (!f) {
    snprintf(erro, cap, "não foi possível abrir o arquivo");
    return false;
  }
  bool ok = validaArquivo(f, r, conv, erro, cap);
  f.close();
  r.parse_ms = millis() - t0;
  return ok;
}

// ---------- Uno (serial) ----------

// Log: últimas 20 mensagens trocadas, num buffer circular (sem alocação dinâmica)
struct Entrada {
  unsigned long t;
  char dir;      // '>' ESP32→Uno, '<' Uno→ESP32, '!' erro local
  char txt[64];
};
const int TAM_LOG = 20;
Entrada logUno[TAM_LOG];
int logProx = 0, logQtd = 0;

enum EstadoUno { UNO_DESCONHECIDO, UNO_OK, UNO_SEM_RESPOSTA };
EstadoUno estadoUno = UNO_DESCONHECIDO;
char versaoUno[97] = "";
unsigned long errosUno = 0;

// Comando em andamento (no máximo um)
char pendente[33] = "";
bool temPendente = false, enviado = false;
unsigned long enviadoEm = 0, prazoMs = 0;
char dadoRecebido[97] = "";       // linha extra antes do OK (ex.: o "1" do QP)
unsigned long motionEndMs = 0;    // instante previsto de fim do último movimento
long posBracoAbs = 0;             // braço em passos a partir do centro (calibração), pelos SM enviados

// Última troca concluída
bool temUltimo = false;
char ultimoCmd[33] = "", ultimaResp[97] = "";
bool ultimoOk = false;
unsigned long ultimoMs = 0;

// Linha sendo recebida do Uno
char linhaRx[97];
int lenRx = 0;

void registra(char dir, const char *txt) {
  Entrada &e = logUno[logProx];
  e.t = millis();
  e.dir = dir;
  int i = 0;
  for (; txt[i] && i < (int)sizeof(e.txt) - 1; i++) {
    unsigned char c = txt[i];
    e.txt[i] = (c < 0x20 || c > 0x7E) ? '?' : c;
  }
  e.txt[i] = 0;
  logProx = (logProx + 1) % TAM_LOG;
  if (logQtd < TAM_LOG) logQtd++;
  Serial.printf("[UNO%c] %s\n", dir, e.txt);
}

// Quanto falta para o último movimento acabar (0 se já acabou)
unsigned long faltaMovimento() {
  long falta = (long)(motionEndMs - millis());
  return falta > 0 ? (unsigned long)falta : 0;
}

void enfileira(const char *cmd) {
  strncpy(pendente, cmd, sizeof(pendente) - 1);
  pendente[sizeof(pendente) - 1] = 0;
  temPendente = true;
  enviado = false;
}

// Fecha o comando pendente. "resposta" vazia = não houve resposta.
void conclui(bool ok, const char *resposta) {
  unsigned long agora = millis();
  temUltimo = true;
  strcpy(ultimoCmd, pendente);
  strncpy(ultimaResp, resposta, sizeof(ultimaResp) - 1);
  ultimaResp[sizeof(ultimaResp) - 1] = 0;
  ultimoOk = ok;
  ultimoMs = agora - enviadoEm;
  if (resposta[0]) estadoUno = UNO_OK;

  // O OK de SM/SP sai quando o movimento começa: guarda quando ele deve terminar
  if (ok && strncmp(pendente, "SM,", 3) == 0) {
    motionEndMs = agora + strtoul(pendente + 3, nullptr, 10);
  } else if (ok && strncmp(pendente, "SP,", 3) == 0) {
    const char *virgula = strchr(pendente + 3, ',');
    motionEndMs = agora + (virgula ? strtoul(virgula + 1, nullptr, 10) : 500);
  }
  temPendente = false;
  enviado = false;
}

void trataLinha(const char *l) {
  registra('<', l);
  if (!temPendente || !enviado) return;  // ninguém esperando: só registra

  if (strcmp(pendente, "v") == 0 && strncmp(l, "EBB", 3) == 0) {
    strncpy(versaoUno, l, sizeof(versaoUno) - 1);
    conclui(true, l);
  } else if (strcmp(l, "OK") == 0) {
    char resp[97];
    if (dadoRecebido[0]) snprintf(resp, sizeof(resp), "%.90s / OK", dadoRecebido);
    else strcpy(resp, "OK");
    conclui(true, resp);
  } else if (strcmp(l, "unknown CMD") == 0) {
    errosUno++;
    conclui(false, l);
  } else {
    strncpy(dadoRecebido, l, sizeof(dadoRecebido) - 1);  // dado: continua esperando o OK
    dadoRecebido[sizeof(dadoRecebido) - 1] = 0;
  }
}

// Chamada a cada volta do loop(): envia, recebe e confere o prazo. Nunca espera.
void atendeUno() {
  unsigned long agora = millis();

  if (temPendente && !enviado) {
    Serial2.print(pendente);
    Serial2.print('\r');
    registra('>', pendente);
    if (strncmp(pendente, "SM,", 3) == 0) {  // SM,dur,braço,rotação
      const char *p = strchr(pendente + 3, ',');
      if (p) posBracoAbs += strtol(p + 1, nullptr, 10);
    }
    prazoMs = faltaMovimento() + 1000;
    enviadoEm = agora;
    enviado = true;
    dadoRecebido[0] = 0;
  }

  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\r') continue;
    if (c == '\n') {
      if (lenRx > 0) {
        linhaRx[lenRx] = 0;
        trataLinha(linhaRx);
      }
      lenRx = 0;
    } else if (lenRx < 96) {
      linhaRx[lenRx++] = (c < 0x20 || c > 0x7E) ? '?' : c;  // não imprimível vira '?'
    }
  }

  if (temPendente && enviado && millis() - enviadoEm > prazoMs) {
    char msg[40];
    snprintf(msg, sizeof(msg), "sem resposta (%lu ms)", prazoMs);
    registra('!', msg);
    estadoUno = UNO_SEM_RESPOSTA;
    errosUno++;
    conclui(false, "");
  }
}

// Lê inteiros separados por vírgula ("500,-100,0"). Retorna quantos leu, ou -1 se mal formado.
int lerNumeros(const char *s, long *v, int max) {
  int n = 0;
  while (true) {
    const char *ini = s;
    if (*s == '-') s++;
    if (*s < '0' || *s > '9') return -1;
    long x = 0;
    int dig = 0;
    while (*s >= '0' && *s <= '9') {
      if (++dig > 6) return -1;
      x = x * 10 + (*s++ - '0');
    }
    if (n >= max) return -1;
    v[n++] = (*ini == '-') ? -x : x;
    if (*s == 0) return n;
    if (*s != ',') return -1;
    s++;
  }
}

// Valida o texto de /cmd e escreve em "saida" o comando a transmitir.
// Retorna nullptr se estiver ok, ou a mensagem de erro.
const char *validaComando(const char *entrada, char *saida) {
  size_t n = strlen(entrada);
  if (n < 1 || n > 32) return "Comando inválido.";
  for (size_t i = 0; i < n; i++) {
    char c = toupper((unsigned char)entrada[i]);
    if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ',' || c == '-')) return "Comando inválido.";
    saida[i] = c;
  }
  saida[n] = 0;

  long v[4];
  if (strncmp(saida, "SM,", 3) == 0) {
    if (lerNumeros(saida + 3, v, 4) != 3) return "SM precisa de 3 números inteiros: SM,duração,braço,rotação.";
    long dur = v[0];
    if (dur < 1 || dur > 5000) return "Duração do SM deve ficar entre 1 e 5000 ms.";
    for (int i = 1; i <= 2; i++) {
      if (labs(v[i]) > 1600) return "No máximo 1600 passos por eixo em um SM.";
      if (labs(v[i]) * 1000 > 1000 * dur) return "Rápido demais: no máximo 1000 passos/s por eixo (aumente a duração).";
    }
  } else if (strncmp(saida, "SP,", 3) == 0) {
    int q = lerNumeros(saida + 3, v, 3);
    if (q < 1 || q > 2) return "Use SP,0 (sobe) ou SP,1 (desce), com tempo opcional: SP,0,500.";
    if (v[0] != 0 && v[0] != 1) return "SP aceita só 0 (sobe) ou 1 (desce).";
    if (q == 2 && (v[1] < 0 || v[1] > 2000)) return "Tempo do SP deve ficar entre 0 e 2000 ms.";
  }

  // O EggDuino diferencia maiúsculas: a versão é pedida com "v" minúsculo
  if (strcmp(saida, "V") == 0) strcpy(saida, "v");
  return nullptr;
}

// ---------- Impressão ----------

enum EstadoImp { IMP_PARADO, IMP_IMPRIMINDO, IMP_PAUSADO, IMP_PARANDO, IMP_CONCLUIDO, IMP_ERRO };
EstadoImp estadoImp = IMP_PARADO;

File arqImp;                 // /job.txt aberto durante a impressão
Leitor leitorImp(arqImp);    // lê uma linha de dados por vez
Conversor convImp;           // gera os comandos da linha lida
Fila filaImp;                // comandos gerados, esperando a vez
Fila filaFim;                // fim seguro (cancelar / erro)
Fila filaCfg;                // comandos da configuração da caneta (SC/SP), mandados quando a serial livrar
unsigned long totalImp = 0, estMsImp = 0;  // comandos e tempo da impressão atual
long linhasFaltando = 0;     // linhas de dados ainda não lidas
bool geradorAcabou = false;

unsigned long feitos = 0;        // comandos do desenho com OK
unsigned long msFeitos = 0;      // tempo estimado desses comandos
unsigned long inicioImpMs = 0, fimImpMs = 0;
bool aguardando = false;         // o último comando enviado pela impressão ainda não foi avaliado
bool contaAtual = false;         // ele faz parte do desenho (conta no progresso)?
unsigned long msAtual = 0;
bool canetaAbaixada = false;     // estado da caneta pelos comandos já enviados
bool canetaPausa = false;        // a pausa levantou a caneta
bool baixarAoRetomar = false;
long posRot = 0, posBraco = 0;   // posição pelos SM já enviados (origem = início da impressão)
bool pararPorErro = false;
char cmdImp[24] = "";            // último comando enviado pela impressão
char msgImp[256] = "";

bool imprimindo() {
  return estadoImp == IMP_IMPRIMINDO || estadoImp == IMP_PAUSADO || estadoImp == IMP_PARANDO;
}

void enviaImp(const char *cmd, unsigned long ms, bool conta) {
  enfileira(cmd);
  strncpy(cmdImp, cmd, sizeof(cmdImp) - 1);
  aguardando = true;
  contaAtual = conta;
  msAtual = ms;
  if (strncmp(cmd, "SP,1", 4) == 0) {
    canetaAbaixada = true;
  } else if (strncmp(cmd, "SP,0", 4) == 0) {
    canetaAbaixada = false;
  } else if (strncmp(cmd, "SM,", 3) == 0) {
    long v[4];
    if (lerNumeros(cmd + 3, v, 4) == 3) {
      posBraco += v[1];
      posRot += v[2];
    }
  }
}

// Lê mais linhas do arquivo até ter comandos na fila (ou o arquivo acabar).
bool abastece() {
  while (filaImp.qtd == 0 && !geradorAcabou) {
    if (linhasFaltando == 0) {
      convImp.fim();
      geradorAcabou = true;
      break;
    }
    if (!leitorImp.proxima()) return false;
    linhasFaltando--;
    long r, a;
    if (leitorImp.igual("U")) convImp.sobe();
    else if (leitorImp.igual("D")) convImp.desce();
    else if (lerLinhaL(leitorImp, r, a)) convImp.vai(r, a);
    else return false;
  }
  return true;
}

// Fim seguro: caneta sobe, (volta à origem) e motores desligam.
void preparaFim(bool voltar) {
  if (arqImp) arqImp.close();
  filaFim.limpa();
  Conversor c;
  c.fila = &filaFim;
  c.rot = posRot;
  c.braco = posBraco;
  char sp[16];
  snprintf(sp, sizeof(sp), "SP,0,%lu", cfg.pen_ms);
  c.emite(sp, cfg.pen_ms);
  if (voltar) c.vai(0, 0);
  c.emite("EM,0", 0);
  estadoImp = IMP_PARANDO;
}

void falhaImp(const char *motivo) {
  snprintf(msgImp, sizeof(msgImp), "Parou no comando %lu de %lu (%s): %s.", feitos + 1, totalImp, cmdImp, motivo);
  Serial.printf("Impressão interrompida: %s\n", msgImp);
  pararPorErro = true;
  preparaFim(false);
}

// Chamada a cada volta do loop(): manda o próximo comando quando a serial estiver livre.
void atendeImpressao() {
  if (!imprimindo() || temPendente) return;

  if (aguardando) {  // o último comando enviado pela impressão terminou
    aguardando = false;
    if (contaAtual && ultimoOk) {
      feitos++;
      msFeitos += msAtual;
    } else if (contaAtual && estadoImp != IMP_PARANDO) {
      falhaImp(ultimaResp[0] ? "o Arduino respondeu \"unknown CMD\"" : "sem resposta do Arduino");
      return;
    }
  }

  char cmd[24];
  unsigned long ms;
  if (estadoImp == IMP_IMPRIMINDO) {
    if (baixarAoRetomar) {  // volta de uma pausa: abaixa a caneta antes de continuar
      baixarAoRetomar = false;
      snprintf(cmd, sizeof(cmd), "SP,1,%lu", cfg.pen_ms);
      enviaImp(cmd, cfg.pen_ms, false);
      return;
    }
    if (!abastece()) {
      falhaImp("arquivo ilegível na flash");
      return;
    }
    if (filaImp.tira(cmd, ms)) {
      enviaImp(cmd, ms, true);
      return;
    }
    // acabou: o próprio programa já levantou a caneta, voltou à origem e desligou os motores
    if (arqImp) arqImp.close();
    estadoImp = IMP_CONCLUIDO;
    fimImpMs = millis();
    unsigned long s = (fimImpMs - inicioImpMs) / 1000;
    snprintf(msgImp, sizeof(msgImp), "Desenho concluído: %lu comandos em %lu min %lu s.", feitos, s / 60, s % 60);
    Serial.println(msgImp);
  } else if (estadoImp == IMP_PAUSADO) {
    if (canetaAbaixada) {  // pausa com a caneta levantada, para não borrar
      canetaPausa = true;
      snprintf(cmd, sizeof(cmd), "SP,0,%lu", cfg.pen_ms);
      enviaImp(cmd, cfg.pen_ms, false);
    }
  } else if (estadoImp == IMP_PARANDO) {
    if (filaFim.tira(cmd, ms)) {
      enviaImp(cmd, ms, false);
      return;
    }
    estadoImp = pararPorErro ? IMP_ERRO : IMP_PARADO;
    fimImpMs = millis();
  }
}

// Ações da impressão. Retornam o código HTTP e, se não for 200, a mensagem.
int iniciaImpressao(const char *&msg) {
  if (!fsOk) { msg = "Sistema de arquivos indisponível."; return 503; }
  if (imprimindo()) { msg = "Já existe uma impressão em andamento."; return 409; }
  if (!job.present) { msg = "Nenhum arquivo no EggBot."; return 404; }
  if (temPendente || filaCfg.qtd) { msg = "Aguarde a resposta do comando anterior."; return 409; }
  // O desenho começa de onde o braço está: recalcula total e tempo a partir dessa posição
  Resumo r = {};
  Conversor contagem;
  contagem.braco = posBracoAbs;
  char erro[128];
  if (!processa(ARQ_JOB, r, contagem, erro, sizeof(erro))) { msg = "O arquivo no EggBot não é mais válido."; return 409; }
  totalImp = r.ebb_cmds;
  estMsImp = r.est_ms;
  arqImp = LittleFS.open(ARQ_JOB, "r");
  if (!arqImp) { msg = "Não foi possível abrir o arquivo."; return 500; }
  leitorImp.reinicia();
  for (int i = 0; i < 3; i++) leitorImp.proxima();  // cabeçalho, nome e P (já validados)
  linhasFaltando = job.lines - 4;
  filaImp.limpa();
  convImp = Conversor();
  convImp.fila = &filaImp;
  convImp.braco = posBracoAbs;
  convImp.inicio();
  geradorAcabou = false;
  feitos = msFeitos = 0;
  posRot = 0;
  posBraco = posBracoAbs;
  canetaAbaixada = canetaPausa = baixarAoRetomar = false;
  aguardando = pararPorErro = false;
  cmdImp[0] = msgImp[0] = 0;
  inicioImpMs = millis();
  estadoImp = IMP_IMPRIMINDO;
  Serial.printf("Impressão iniciada: \"%s\", %lu comandos, ~%lu s (braço em %ld)\n", job.name, totalImp, (estMsImp + 500) / 1000, posBracoAbs);
  return 200;
}

int pausaImpressao(const char *&msg) {
  if (estadoImp != IMP_IMPRIMINDO) { msg = "Não há impressão rodando."; return 409; }
  estadoImp = IMP_PAUSADO;
  return 200;
}

int retomaImpressao(const char *&msg) {
  if (estadoImp != IMP_PAUSADO) { msg = "A impressão não está pausada."; return 409; }
  baixarAoRetomar = canetaPausa;
  canetaPausa = false;
  estadoImp = IMP_IMPRIMINDO;
  return 200;
}

int cancelaImpressao(const char *&msg) {
  if (estadoImp != IMP_IMPRIMINDO && estadoImp != IMP_PAUSADO) { msg = "Não há impressão para cancelar."; return 409; }
  snprintf(msgImp, sizeof(msgImp), "Impressão cancelada no comando %lu de %lu.", feitos, totalImp);
  Serial.println(msgImp);
  pararPorErro = false;
  preparaFim(true);
  return 200;
}

// ---------- Configuração e calibração ----------

// Valor EBB do servo para o EggDuino: ângulo = (v − 6000) / 133,3, então 0–100 % → 0–180°
// (mesma escala do driver do Inkscape: 240 × (pct + 25)).
unsigned long valorServo(long pct) { return 240UL * (pct + 25); }

// Confere uma configuração nova. Retorna nullptr se estiver ok, ou a mensagem de erro.
const char *validaConfig(const Config &c) {
  if (c.pen_up < 0 || c.pen_up > 100 || c.pen_down < 0 || c.pen_down > 100) return "Posições da caneta vão de 0 a 100 %.";
  if (c.pen_ms < 100 || c.pen_ms > 2000) return "Tempo da caneta deve ficar entre 100 e 2000 ms.";
  if (c.speed_down < 50 || c.speed_down > 1000 || c.speed_up < 50 || c.speed_up > 1000)
    return "Velocidades devem ficar entre 50 e 1000 passos/s (limite seguro do EggDuino).";
  if (c.arm_max < 10 || c.arm_max > 1600) return "Limite superior do braço deve ficar entre 10 e 1600 passos.";
  if (c.arm_min > -10 || c.arm_min < -1600) return "Limite inferior do braço deve ficar entre −1600 e −10 passos.";
  return nullptr;
}

// Grava as posições da caneta na EEPROM do Uno. No EggDuino os nomes ficam invertidos:
// SC,5 é a posição usada por SP,0 (em cima) e SC,4 a usada por SP,1 (embaixo).
void enviaConfigCaneta() {
  char c[24];
  snprintf(c, sizeof(c), "SC,5,%lu", valorServo(cfg.pen_up));
  filaCfg.poe(c, 0);
  snprintf(c, sizeof(c), "SC,4,%lu", valorServo(cfg.pen_down));
  filaCfg.poe(c, 0);
}

// Manda os comandos da configuração quando a serial estiver livre (fora da impressão).
void atendeConfigUno() {
  if (temPendente || imprimindo() || filaCfg.qtd == 0) return;
  char c[24];
  unsigned long d;
  filaCfg.tira(c, d);
  enfileira(c);
}

// Aplica uma configuração nova (já validada). Revalida o arquivo guardado: as velocidades mudam
// o tempo estimado e os limites do braço podem deixá-lo inválido.
int aplicaConfig(const Config &novo, const char *&msg, bool &descartou) {
  descartou = false;
  if (imprimindo()) { msg = "Impressão em andamento: cancele antes de mudar a configuração."; return 409; }
  bool canetaMudou = novo.pen_up != cfg.pen_up || novo.pen_down != cfg.pen_down;
  cfg = novo;
  if (canetaMudou) enviaConfigCaneta();
  if (fsOk && job.present) {
    Resumo r = {};
    Conversor conv;
    char erro[128];
    if (processa(ARQ_JOB, r, conv, erro, sizeof(erro))) {
      r.present = true;
      r.upload_ms = job.upload_ms;
      job = r;
    } else {
      LittleFS.remove(ARQ_JOB);
      job.present = false;
      descartou = true;
      snprintf(msgImp, sizeof(msgImp), "O arquivo no EggBot ficou fora da nova configuração (%s) e foi apagado; envie de novo.", erro);
    }
    if (estadoImp == IMP_CONCLUIDO || estadoImp == IMP_ERRO || descartou) {
      estadoImp = IMP_PARADO;
      feitos = 0;
      if (!descartou) msgImp[0] = 0;
    }
  }
  return 200;
}

// Testa uma posição da caneta sem salvar: grava no Uno (SC) e move o servo (SP).
int testaCaneta(bool cima, long pct, const char *&msg) {
  if (imprimindo()) { msg = "Impressão em andamento."; return 409; }
  if (pct < 0 || pct > 100) { msg = "Posição da caneta vai de 0 a 100 %."; return 400; }
  if (temPendente || filaCfg.qtd) { msg = "Aguarde a resposta do comando anterior."; return 409; }
  char c[24];
  snprintf(c, sizeof(c), "SC,%d,%lu", cima ? 5 : 4, valorServo(pct));
  filaCfg.poe(c, 0);
  snprintf(c, sizeof(c), "SP,%d,%lu", cima ? 0 : 1, cfg.pen_ms);
  filaCfg.poe(c, cfg.pen_ms);
  return 200;
}

// A posição atual do braço passa a ser o centro (zero) da calibração.
int zeraBraco(const char *&msg) {
  if (imprimindo()) { msg = "Impressão em andamento."; return 409; }
  if (temPendente || filaCfg.qtd) { msg = "Aguarde a resposta do comando anterior."; return 409; }
  posBracoAbs = 0;
  return 200;
}

// ---------- Rotas ----------

char jsonStatus[4096];
char jsonResp[512];

// Estado do envio em andamento
File arqEnvio;
bool envioRecebido = false, envioGrande = false, envioAbortado = false, envioErroEscrita = false;
unsigned long envioBytes = 0, envioInicio = 0, envioMs = 0;

void jsonCamposJob(Json &j) {
  if (!job.present) {
    j.add("\"present\":false");
    return;
  }
  j.add("\"present\":true,\"name\":\"");
  j.texto(job.name);
  j.add("\",\"bytes\":%lu,\"lines\":%lu,\"points\":%lu,\"strokes\":%lu,\"crc\":\"%08lX\","
        "\"ebb_cmds\":%lu,\"est_s\":%lu,\"upload_ms\":%lu,\"parse_ms\":%lu",
        job.bytes, job.lines, job.points, job.strokes, job.crc,
        job.ebb_cmds, job.est_s, job.upload_ms, job.parse_ms);
}

void enviaErro(int codigo, const char *mensagem) {
  Json j(jsonResp, sizeof(jsonResp));
  j.add("{\"ok\":false,\"message\":\"");
  j.texto(mensagem);
  j.add("\"}");
  server.send(codigo, "application/json", jsonResp);
}

// GET / — a página
void rotaPagina() {
  server.send_P(200, "text/html", PAGINA);
}

// GET /status — rede, flash, arquivo, Uno, impressão e log (a página consulta a cada 1 s)
void rotaStatus() {
  static const char *estadosUno[] = {"unknown", "ok", "no_reply"};
  static const char *estadosImp[] = {"idle", "running", "paused", "stopping", "done", "error"};
  Json j(jsonStatus, sizeof(jsonStatus));
  j.add("{\"ssid\":\"");
  j.texto(ssid);
  j.add("\",\"clients\":%u,\"uptime_s\":%lu,\"free_heap\":%lu,\"min_free_heap\":%lu,",
        (unsigned)WiFi.softAPgetStationNum(), (unsigned long)(millis() / 1000),
        (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getMinFreeHeap());
  j.add("\"fs\":{\"total\":%lu,\"used\":%lu},\"job\":{",
        fsOk ? (unsigned long)LittleFS.totalBytes() : 0UL,
        fsOk ? (unsigned long)LittleFS.usedBytes() : 0UL);
  jsonCamposJob(j);

  j.add("},\"uno\":{\"state\":\"%s\",\"version\":\"", estadosUno[estadoUno]);
  j.texto(versaoUno);
  j.add("\",\"busy\":%s,\"pending\":\"", temPendente ? "true" : "false");
  j.texto(temPendente ? pendente : "");
  j.add("\",\"moving_ms\":%lu,\"errors\":%lu,\"last\":{", faltaMovimento(), errosUno);
  if (temUltimo) {
    j.add("\"cmd\":\"");
    j.texto(ultimoCmd);
    j.add("\",\"reply\":\"");
    j.texto(ultimaResp);
    j.add("\",\"ok\":%s,\"ms\":%lu", ultimoOk ? "true" : "false", ultimoMs);
  }

  unsigned long decorrido = estadoImp == IMP_PARADO && !inicioImpMs ? 0
                          : ((imprimindo() ? millis() : fimImpMs) - inicioImpMs) / 1000;
  bool usouImp = estadoImp != IMP_PARADO || feitos > 0;  // mostra os números da impressão atual
  unsigned long total = usouImp ? totalImp : (job.present ? job.ebb_cmds : 0UL);
  unsigned long restante = estMsImp > msFeitos ? (estMsImp - msFeitos + 500) / 1000 : 0;
  j.add("}},\"print\":{\"state\":\"%s\",\"done\":%lu,\"total\":%lu,\"elapsed_s\":%lu,\"remaining_s\":%lu,\"cmd\":\"",
        estadosImp[estadoImp], feitos, total, decorrido, imprimindo() ? restante : 0UL);
  j.texto(cmdImp);
  j.add("\",\"message\":\"");
  j.texto(msgImp);
  j.add("\"},\"cfg\":{\"pen_up\":%ld,\"pen_down\":%ld,\"pen_ms\":%lu,\"speed_down\":%lu,\"speed_up\":%lu,"
        "\"arm_min\":%ld,\"arm_max\":%ld},\"arm_pos\":%ld,\"cfg_busy\":%s,\"log\":[",
        cfg.pen_up, cfg.pen_down, cfg.pen_ms, cfg.speed_down, cfg.speed_up, cfg.arm_min, cfg.arm_max,
        posBracoAbs, filaCfg.qtd ? "true" : "false");
  int mostrar = logQtd < 12 ? logQtd : 12;
  for (int k = 0; k < mostrar; k++) {  // do mais recente para o mais antigo
    const Entrada &e = logUno[(logProx - 1 - k + 2 * TAM_LOG) % TAM_LOG];
    j.add("%s{\"t\":%lu,\"dir\":\"%c\",\"txt\":\"", k ? "," : "", e.t, e.dir);
    j.texto(e.txt);
    j.add("\"}");
  }
  j.add("]}");
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", jsonStatus);
}

// POST /cmd?c=<texto> — comando manual (recusado durante a impressão)
void rotaCmd() {
  char cmd[33];
  const char *erro = validaComando(server.arg("c").c_str(), cmd);
  if (erro) {
    enviaErro(400, erro);
    return;
  }
  if (imprimindo()) {
    enviaErro(409, "Impressão em andamento: use Pausar ou Cancelar.");
    return;
  }
  if (temPendente || filaCfg.qtd) {
    enviaErro(409, "Aguarde a resposta do comando anterior.");
    return;
  }
  enfileira(cmd);
  server.send(202, "application/json", "{\"ok\":true}");
}

// POST /job — chamada a cada pedaço do arquivo que chega (nunca o arquivo inteiro)
void aoReceberPedaco() {
  HTTPUpload &up = server.upload();
  if (!fsOk || imprimindo()) return;

  if (up.status == UPLOAD_FILE_START) {
    arqEnvio = LittleFS.open(ARQ_TMP, "w");
    envioRecebido = true;
    envioGrande = envioAbortado = false;
    envioErroEscrita = !arqEnvio;
    envioBytes = 0;
    envioInicio = millis();
  } else if (up.status == UPLOAD_FILE_WRITE) {
    envioBytes += up.currentSize;
    if (envioBytes > LIMITE_BYTES) {
      envioGrande = true;  // continua recebendo, mas não grava mais
    } else if (arqEnvio && arqEnvio.write(up.buf, up.currentSize) != up.currentSize) {
      envioErroEscrita = true;
    }
  } else if (up.status == UPLOAD_FILE_END) {
    if (arqEnvio) arqEnvio.close();
    envioMs = millis() - envioInicio;
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    if (arqEnvio) arqEnvio.close();
    LittleFS.remove(ARQ_TMP);
    envioAbortado = true;
    envioMs = millis() - envioInicio;
  }
}

// POST /job — chamada depois que o envio terminou
void aoTerminar() {
  if (!fsOk) {
    enviaErro(503, "sistema de arquivos indisponível");
    return;
  }
  if (imprimindo()) {
    enviaErro(409, "Impressão em andamento: cancele antes de enviar outro arquivo.");
    return;
  }
  if (!envioRecebido) {
    enviaErro(400, "nenhum arquivo recebido (campo \"file\")");
    return;
  }
  envioRecebido = false;

  char erro[128];
  int codigo = 200;
  Resumo novo = {};
  Conversor conv;

  if (envioAbortado) {
    codigo = 400;
    snprintf(erro, sizeof(erro), "envio interrompido");
  } else if (envioGrande) {
    LittleFS.remove(ARQ_TMP);
    codigo = 413;
    unsigned long decimos = (envioBytes * 10 + 512) / 1024;
    snprintf(erro, sizeof(erro), "arquivo com %lu,%lu KB, acima do limite de 64 KB", decimos / 10, decimos % 10);
  } else if (envioErroEscrita) {
    LittleFS.remove(ARQ_TMP);
    codigo = 500;
    snprintf(erro, sizeof(erro), "erro ao gravar na flash");
  } else if (!processa(ARQ_TMP, novo, conv, erro, sizeof(erro))) {
    LittleFS.remove(ARQ_TMP);
    codigo = 400;
  }

  if (codigo != 200) {
    Serial.printf("Envio recusado (HTTP %d): %lu B, recebido em %lu ms, validado em %lu ms: %s\n",
                  codigo, envioBytes, envioMs, novo.parse_ms, erro);
    enviaErro(codigo, erro);
    return;
  }

  // Aceito: o novo arquivo substitui o anterior
  LittleFS.remove(ARQ_JOB);
  LittleFS.rename(ARQ_TMP, ARQ_JOB);
  novo.present = true;
  novo.upload_ms = envioMs;
  job = novo;
  if (estadoImp == IMP_CONCLUIDO || estadoImp == IMP_ERRO) {  // o resultado era do arquivo anterior
    estadoImp = IMP_PARADO;
    feitos = 0;
    msgImp[0] = cmdImp[0] = 0;
  }

  Serial.printf("Envio aceito: \"%s\", %lu B, %lu linhas, %lu comandos EBB (~%lu s), recebido em %lu ms, validado em %lu ms, CRC %08lX\n",
                job.name, job.bytes, job.lines, job.ebb_cmds, job.est_s, job.upload_ms, job.parse_ms, job.crc);

  Json j(jsonResp, sizeof(jsonResp));
  j.add("{\"ok\":true,");
  jsonCamposJob(j);
  j.add("}");
  server.send(200, "application/json", jsonResp);
}

// GET /job/raw — o arquivo como está na flash
void rotaJobRaw() {
  if (!fsOk) {
    enviaErro(503, "sistema de arquivos indisponível");
    return;
  }
  File f = job.present ? LittleFS.open(ARQ_JOB, "r") : File();
  if (!f) {
    server.send(404, "text/plain", "Nao encontrado");
    return;
  }
  server.streamFile(f, "text/plain; charset=utf-8");
  f.close();
}

// POST /job/delete — apaga o arquivo
void rotaJobDelete() {
  if (!fsOk) {
    enviaErro(503, "sistema de arquivos indisponível");
    return;
  }
  if (imprimindo()) {
    enviaErro(409, "Impressão em andamento: cancele antes de apagar.");
    return;
  }
  LittleFS.remove(ARQ_JOB);
  job.present = false;
  estadoImp = IMP_PARADO;
  feitos = 0;
  msgImp[0] = cmdImp[0] = 0;
  Serial.println("Arquivo apagado");
  server.send(200, "application/json", "{\"ok\":true}");
}

// POST /print/start | pause | resume | cancel
void respondeAcao(int codigo, const char *msg) {
  if (codigo == 200) server.send(200, "application/json", "{\"ok\":true}");
  else enviaErro(codigo, msg);
}
void rotaPrintStart() { const char *m = ""; int c = iniciaImpressao(m); respondeAcao(c, m); }
void rotaPrintPause() { const char *m = ""; int c = pausaImpressao(m); respondeAcao(c, m); }
void rotaPrintResume() { const char *m = ""; int c = retomaImpressao(m); respondeAcao(c, m); }
void rotaPrintCancel() { const char *m = ""; int c = cancelaImpressao(m); respondeAcao(c, m); }

// Configuração gravada na flash do ESP32 (NVS), namespace "eggbot"
Preferences prefs;

void carregaConfig() {
  Config c;  // valores padrão
  prefs.begin("eggbot", true);
  c.pen_up = prefs.getLong("pen_up", c.pen_up);
  c.pen_down = prefs.getLong("pen_down", c.pen_down);
  c.pen_ms = prefs.getULong("pen_ms", c.pen_ms);
  c.speed_down = prefs.getULong("speed_down", c.speed_down);
  c.speed_up = prefs.getULong("speed_up", c.speed_up);
  c.arm_min = prefs.getLong("arm_min", c.arm_min);
  c.arm_max = prefs.getLong("arm_max", c.arm_max);
  prefs.end();
  if (validaConfig(c)) c = Config();  // algo estranho gravado: volta ao padrão
  cfg = c;
}

void salvaConfig() {
  prefs.begin("eggbot", false);
  prefs.putLong("pen_up", cfg.pen_up);
  prefs.putLong("pen_down", cfg.pen_down);
  prefs.putULong("pen_ms", cfg.pen_ms);
  prefs.putULong("speed_down", cfg.speed_down);
  prefs.putULong("speed_up", cfg.speed_up);
  prefs.putLong("arm_min", cfg.arm_min);
  prefs.putLong("arm_max", cfg.arm_max);
  prefs.end();
}

// POST /config?pen_up=&pen_down=&pen_ms=&speed_down=&speed_up=&arm_min=&arm_max= (campos ausentes ficam como estão)
void rotaConfig() {
  Config novo = cfg;
  if (server.hasArg("pen_up")) novo.pen_up = server.arg("pen_up").toInt();
  if (server.hasArg("pen_down")) novo.pen_down = server.arg("pen_down").toInt();
  if (server.hasArg("pen_ms")) novo.pen_ms = server.arg("pen_ms").toInt();
  if (server.hasArg("speed_down")) novo.speed_down = server.arg("speed_down").toInt();
  if (server.hasArg("speed_up")) novo.speed_up = server.arg("speed_up").toInt();
  if (server.hasArg("arm_min")) novo.arm_min = server.arg("arm_min").toInt();
  if (server.hasArg("arm_max")) novo.arm_max = server.arg("arm_max").toInt();
  const char *erro = validaConfig(novo);
  if (erro) {
    enviaErro(400, erro);
    return;
  }
  const char *msg = "";
  bool descartou = false;
  int codigo = aplicaConfig(novo, msg, descartou);
  if (codigo != 200) {
    enviaErro(codigo, msg);
    return;
  }
  salvaConfig();
  Serial.printf("Configuração salva: caneta %ld/%ld %% (%lu ms), velocidade %lu/%lu passos/s, braço %ld a %ld\n",
                cfg.pen_up, cfg.pen_down, cfg.pen_ms, cfg.speed_down, cfg.speed_up, cfg.arm_min, cfg.arm_max);
  server.send(200, "application/json", descartou ? "{\"ok\":true,\"job_dropped\":true}" : "{\"ok\":true,\"job_dropped\":false}");
}

// POST /pen/test?pos=up|down&pct=N — move o servo para a posição sem salvar no ESP32
void rotaPenTest() {
  const char *m = "";
  int c = testaCaneta(server.arg("pos") == "up", server.hasArg("pct") ? server.arg("pct").toInt() : -1, m);
  respondeAcao(c, m);
}

// POST /arm/zero — a posição atual do braço vira o centro
void rotaArmZero() {
  const char *m = "";
  int c = zeraBraco(m);
  respondeAcao(c, m);
}

// Qualquer outra rota
void rotaNaoEncontrada() {
  server.send(404, "text/plain", "Nao encontrado");
}

unsigned long inicioMs = 0;
bool versaoPedida = false;

void setup() {
  Serial.begin(115200);
  Serial2.begin(BAUD_UNO, SERIAL_8N1, PINO_RX_UNO, PINO_TX_UNO);
  iniciaCrc();
  carregaConfig();  // antes de validar o arquivo guardado (limites e velocidades)

  // Rede própria (access point), sem roteador e sem internet
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));

  // Nome da rede: "EggBot-" + 4 últimos dígitos hexadecimais do MAC
  uint8_t mac[6];
  WiFi.softAPmacAddress(mac);
  snprintf(ssid, sizeof(ssid), "EggBot-%02X%02X", mac[4], mac[5]);

  WiFi.softAP(ssid, SENHA_WIFI, 6, 0, 4);  // canal 6, rede visível, até 4 clientes

  server.on("/", HTTP_GET, rotaPagina);
  server.on("/status", HTTP_GET, rotaStatus);
  server.on("/cmd", HTTP_POST, rotaCmd);
  server.on("/job", HTTP_POST, aoTerminar, aoReceberPedaco);
  server.on("/job/raw", HTTP_GET, rotaJobRaw);
  server.on("/job/delete", HTTP_POST, rotaJobDelete);
  server.on("/print/start", HTTP_POST, rotaPrintStart);
  server.on("/print/pause", HTTP_POST, rotaPrintPause);
  server.on("/print/resume", HTTP_POST, rotaPrintResume);
  server.on("/print/cancel", HTTP_POST, rotaPrintCancel);
  server.on("/config", HTTP_POST, rotaConfig);
  server.on("/pen/test", HTTP_POST, rotaPenTest);
  server.on("/arm/zero", HTTP_POST, rotaArmZero);
  server.onNotFound(rotaNaoEncontrada);
  server.begin();

  Serial.println();
  Serial.print("Rede: ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Abra http://192.168.4.1");

  // Flash (LittleFS); formata se a partição estiver vazia
  fsOk = LittleFS.begin(true);
  if (!fsOk) {
    Serial.println("ERRO: LittleFS não montou (confira Tools → Partition Scheme). Rotas de arquivo vão responder 503.");
  } else {
    Serial.printf("LittleFS: %lu de %lu B usados\n",
                  (unsigned long)LittleFS.usedBytes(), (unsigned long)LittleFS.totalBytes());
    if (LittleFS.exists(ARQ_TMP)) LittleFS.remove(ARQ_TMP);  // sobra de um envio interrompido

    // O arquivo "sobrevive" ao reinício: valida de novo para preencher as estatísticas
    if (LittleFS.exists(ARQ_JOB)) {
      Resumo r = {};
      Conversor conv;
      char erro[128];
      if (processa(ARQ_JOB, r, conv, erro, sizeof(erro))) {
        r.present = true;
        r.upload_ms = 0;
        job = r;
        Serial.printf("Arquivo recuperado: \"%s\", %lu B, CRC %08lX\n", job.name, job.bytes, job.crc);
      } else {
        Serial.printf("Arquivo salvo é inválido (%s); ignorado\n", erro);
      }
    }
  }
  inicioMs = millis();
}

void loop() {
  server.handleClient();

  // ~2 s depois de subir a rede, pede a versão para a página já abrir mostrando
  if (!versaoPedida && millis() - inicioMs >= 2000) {
    versaoPedida = true;
    if (!temPendente) enfileira("v");
  }

  atendeUno();
  atendeImpressao();
  atendeConfigUno();
}
