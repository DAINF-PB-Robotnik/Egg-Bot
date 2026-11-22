# Etapa 5 — Configuração, calibração e interface final

> **Para quem lê (Claude / Claude Code):** esta SPEC descreve o código que está em
> `Projeto_web/eggbot_etapa5/`. **Não altere `eggbot_etapa1/` a `eggbot_etapa4/`.** O código parte da
> etapa 4 (desenho → arquivo → impressão pela serial) e acrescenta o que está abaixo. Não implemente
> nada da seção "Fora do escopo".

## Objetivo

Deixar o EggBot ajustável e com a interface final:
1. **configuração da máquina** gravada no ESP32: posições da caneta, tempo do servo, velocidades e
   limites do braço;
2. **calibração guiada**: o ESP32 acompanha a posição do braço, define o centro e marca os limites;
3. **abas funcionando**: Criar / Imprimir / Config.;
4. **padrões prontos** (os mesmos do `preplotter/patterns.py` do grupo) e **importação de SVG**.

## Arquivos

```
Projeto_web/eggbot_etapa5/
├── SPEC-etapa5-configuracao-interface.md   (este arquivo)
├── eggbot_etapa5.ino                       (a Arduino IDE exige o mesmo nome da pasta)
├── pagina.h                                (a página; o mesmo HTML da prévia, sem o simulador)
└── previa-pagina-etapa5.html               (prévia com ESP32 + Uno simulados — só para ver)
```

Ambiente igual ao da etapa 4 (IDE 2, esp32 3.3.x, ESP32 Dev Module, "Default 4MB with spiffs").
Bibliotecas do pacote: `WiFi.h`, `WebServer.h`, `FS.h`, `LittleFS.h`, **`Preferences.h`** (NVS).

## Configuração

| Campo | Padrão | Faixa | Onde é usado |
|---|---|---|---|
| `pen_up` | 60 % | 0–100 | servo com a caneta em cima (`SC,5`) |
| `pen_down` | 40 % | 0–100 | servo com a caneta embaixo (`SC,4`) |
| `pen_ms` | 500 ms | 100–2000 | espera de cada `SP` (antes era 500 fixo) |
| `speed_down` | 300 passos/s | 50–1000 | velocidade com a caneta abaixada |
| `speed_up` | 800 passos/s | 50–1000 | velocidade com a caneta levantada |
| `arm_min` | −400 | −1600 a −10 | limite inferior do braço (validação do arquivo e área de desenho) |
| `arm_max` | 400 | 10 a 1600 | limite superior do braço |

- Gravada com `Preferences` (namespace `eggbot`), lida no `setup()` **antes** de validar o
  `/job.txt` guardado. Se o que estiver gravado for inválido, volta ao padrão.
- Máximo de 1000 passos/s: limite seguro da etapa 3 (o firmware perde sincronia acima de 2000).
- **Servo no EggDuino** (conferido no código do plex3r/EggDuino V1.6a): `SC,5,v` grava a posição
  que `SP,0` usa (em cima) e `SC,4,v` a que `SP,1` usa (embaixo). Os nomes internos do firmware são
  invertidos, mas o efeito é este. O ângulo é `(v − 6000) / 133,3`; usamos `v = 240 × (pct + 25)`,
  a mesma escala do driver do Inkscape ([evil-mad/EggBot](https://github.com/evil-mad/EggBot), `eggbot.py`): 0–100 % → 0–180°. Os valores
  ficam na EEPROM do Uno (resolve o "servo não se move" da tabela de problemas da etapa 3).
- **Mudou a configuração → revalida o arquivo guardado.** Velocidades e tempo mudam o tempo
  estimado. Se o arquivo ficar fora dos novos limites do braço, ele é apagado e a página avisa.

## Posição do braço e calibração

- O ESP32 soma o campo braço de **todo** `SM` que transmite (manual, impressão, calibração):
  `arm_pos` = passos a partir do centro. No boot vale 0 (supõe o braço no centro).
- **Definir centro** (`POST /arm/zero`): a posição atual vira 0.
- Os limites são marcados na página com a posição atual e salvos junto com a configuração.
- **A impressão começa de onde o braço está:** o conversor parte de `(0, arm_pos)` em vez de
  `(0, 0)`. O primeiro deslocamento (caneta levantada) leva o braço ao primeiro ponto, então as
  coordenadas do braço no arquivo são **absolutas em relação ao centro calibrado**. A rotação
  continua contando de onde o ovo está. No fim volta a `(0, 0)` (braço no centro) e `EM,0`. Ao
  iniciar, o total de comandos e o tempo são recalculados a partir dessa posição.
- Depois de `EM,0` o braço fica solto e pode se mover: a página avisa para definir o centro de novo.

## Fila de comandos (mudança em relação à etapa 4)

Com velocidade configurável até 50 passos/s, uma linha `L` pode gerar muito mais `SM`. No pior caso
(rotação 12800 + braço 3200 ≈ 13200 passos a 50 passos/s ≈ 264 s, em `SM` de ≤ 2 s) são 132
comandos. Por isso a fila tem **160** posições (eram 40). Os comandos de configuração (`SC`/`SP`
do teste) usam outra fila, mandada só quando a serial está livre e fora da impressão.

## Rotas (além das da etapa 4)

| Rota | Resposta |
|---|---|
| `POST /config?pen_up=&pen_down=&pen_ms=&speed_down=&speed_up=&arm_min=&arm_max=` | valida tudo; salva no NVS; se a caneta mudou, manda `SC,5`/`SC,4`; revalida o arquivo. `{"ok":true,"job_dropped":false}`. 400 (valor fora da faixa, com mensagem) / 409 (imprimindo). Campos ausentes ficam como estão |
| `POST /pen/test?pos=up\|down&pct=N` | manda `SC,5,v` + `SP,0,pen_ms` (ou `SC,4,v` + `SP,1,pen_ms`) **sem salvar no ESP32**. 200 / 400 / 409 |
| `POST /arm/zero` | posição atual do braço = 0. 200 / 409 (imprimindo ou serial ocupada) |

`/cmd` e `/print/start` também respondem 409 enquanto houver comandos de configuração na fila.

`GET /status` ganha:
```json
"cfg":{"pen_up":60,"pen_down":40,"pen_ms":500,"speed_down":300,"speed_up":800,"arm_min":-400,"arm_max":400},
"arm_pos":0,"cfg_busy":false
```
E `print.total`/`print.remaining_s` passam a ser os da impressão atual (que pode começar com o
braço fora do centro).

A validação do arquivo troca "braço Y fora de ±400" por
`linha K: braço Y fora dos limites (arm_min a arm_max)`.

## Página

Está em `pagina.h`. As abas trocam de painel no próprio navegador (a última aba aberta fica
lembrada no `localStorage`; sem ele, abre em Criar):

- **Criar:** desenho (a altura da área segue `arm_min`/`arm_max`, mantendo 1 passo = 1 passo; linha
  tracejada no centro), **padrões** (estrela, círculo, quadrado, senoide, 2 senoides, espiral de 2
  voltas: fórmulas do `preplotter/patterns.py` ajustadas aos limites), **Importar SVG** (meia volta ou
  volta inteira), prévia no ovo, arquivo gerado e envio. Pontos fora dos limites (por exemplo, depois
  de mudar a configuração) são ajustados e a página avisa quantos.
- **Imprimir:** card da impressão (da etapa 4) + estado do Arduino + posição do braço; a prévia no
  ovo mostra o arquivo que está no EggBot. A aba mostra o percentual durante a impressão.
- **Config.:** Caneta (sliders, "Testar", tempo), Braço (posição, ±10/±50, definir centro, ir para o
  centro, marcar limites), Velocidade, barra "Salvar/Descartar", e o card Arduino (estado, botões
  manuais, console EBB, log).

**Importar SVG:** o arquivo é colocado num container escondido e o próprio navegador mede a
geometria: cada `path`, `line`, `polyline`, `polygon`, `rect`, `circle` e `ellipse` é amostrado com
`getTotalLength`/`getPointAtLength` e transformado com `getScreenCTM` (pega `transform`, `viewBox`
etc.). Saltos maiores que 3× o passo de amostragem separam subcaminhos (o `M` no meio de um `path`).
O conjunto é escalado mantendo a proporção para caber na largura escolhida e em 95 % da altura do
braço, simplificado (RDP) e vira traços normais (Desfazer funciona). Só contornos: preenchimento é
ignorado, e texto precisa ser convertido em caminho (no Inkscape: Caminho → Objeto para caminho).

## Como testar

1. Gravar, conectar e abrir `http://192.168.4.1`. A página abre na aba Criar.
2. **Config. → Caneta:** ajustar "Em cima" e "Testar"; "Embaixo" e "Testar". O servo vai para cada
   posição. **Salvar**. Depois de "Testar" em cima, o console do card **Arduino** com `QP` responde `1`.
3. **Config. → Braço:** com ±10/±50, levar a caneta ao meio da altura do ovo → **Definir aqui como
   centro**. Subir até o limite seguro → **Marcar aqui** (superior); descer → **Marcar aqui**
   (inferior). **Salvar**. A área de desenho da aba Criar muda de altura.
4. Reiniciar o ESP32: a configuração continua a mesma.
5. **Criar:** inserir um padrão, importar um SVG simples (feito no Inkscape), conferir a prévia,
   **Enviar** → **Ir para Imprimir** → **Iniciar**. O desenho respeita os limites marcados.
6. Mudar a velocidade de desenho e salvar: o tempo estimado do arquivo no EggBot muda. Diminuir o
   limite superior abaixo do desenho e salvar: o arquivo é apagado e a página explica o motivo.
7. Durante a impressão, a configuração, os testes e o "definir centro" ficam bloqueados.

## Fora do escopo

DNS cativo (abrir a página sozinha), mDNS (`eggbot.local`), prévia 3D real (three.js/WebGL),
preenchimento de áreas (hachura) no SVG, imagens bitmap (PNG/JPG → vetor), PlatformIO.
