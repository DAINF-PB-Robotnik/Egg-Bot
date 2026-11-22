# Egg Studio — React + Tailwind

Interface web do Egg-Bot, com um estúdio de desenho, prévia giratória do ovo, controle de impressão e configuração da máquina. A aplicação está isolada nesta pasta; os firmwares não foram alterados.

## Rodar localmente

Use Node.js 22.12+ ou uma versão compatível com o Vite instalado. As versões exatas das dependências estão no `package-lock.json`.

```bash
cd interface_web
npm ci
npm run dev
```

Abra o endereço mostrado pelo Vite, normalmente **http://localhost:5173**.

O modo padrão é **demonstração**: os controles conversam com um simulador no navegador. Nenhum comando é enviado para placas. O simulador foi extraído da prévia original e mantém suas simplificações e movimentos acelerados em 4×.

Para visualizar pelo celular na mesma rede do computador:

```bash
npm run dev -- --host 0.0.0.0
```

Abra no celular o endereço de rede exibido pelo Vite. Não é necessário ter o ESP32 conectado para usar a demonstração.

## O que está implementado

- Interface responsiva com React, Tailwind CSS, ícones Lucide e fonte Manrope empacotada localmente.
- Desenho por mouse/toque, desfazer/refazer, grade, zoom e navegação na área de rotação.
- Rascunho salvo automaticamente no navegador, incluindo o nome do desenho.
- Padrões geométricos existentes e um exemplo adicional, Flora.
- Importação de contornos SVG com remoção de elementos/atributos ativos antes da medição.
- Geração, inspeção e download de arquivos `EGGJOB 1` com CRC-32.
- Prévia do arquivo projetada sobre um ovo, com rotação e opção de mostrar viagens da caneta.
- Upload, início, pausa, retomada, cancelamento e remoção de trabalhos pela API existente.
- Posições e teste do servo, calibração manual do braço, velocidades e console de diagnóstico.
- Avisos de erro, estados de conexão, bloqueios de operação e confirmação de ações destrutivas.

A prévia é ilustrativa e não implementa um novo modelo físico do ovo. O progresso indica comandos aceitos. Os limites e comportamentos do firmware original continuam aplicáveis.

O arquivo `.egg` baixado é texto no formato `EGGJOB 1`; o upload para o ESP32 usa o campo multipart `file` e o nome `job.txt`, conforme a implementação existente.

## Estrutura para editar

```text
interface_web/
├── index.html
├── package.json / package-lock.json
├── vite.config.js
├── .env.example
├── public/egg.svg
├── src/
│   ├── main.jsx
│   ├── App.jsx                       Navegação e estado do desenho
│   ├── styles.css                    Tailwind, tema e estilos dos componentes
│   ├── components/
│   │   ├── DrawingCanvas.jsx         Editor com mouse e toque
│   │   ├── EggPreview.jsx            Projeção ilustrativa do desenho
│   │   ├── PrintPanel.jsx            Trabalho e controles de impressão
│   │   ├── SettingsPanel.jsx         Configuração e calibração
│   │   └── ui.jsx                    Botões, modal, marca e avisos
│   ├── hooks/useMachine.js           Transporte, consulta de estado e ações
│   └── lib/
│       ├── drawing.js                Padrões, simplificação e EGGJOB/CRC
│       ├── svg.js                    Importação de geometria SVG
│       └── demo.js                   Simulador adaptado da prévia original
├── tests/
│   ├── drawing.test.js               Contratos de dados
│   └── studio.spec.js                Fluxos no navegador
├── playwright.config.js
└── legacy/previa-pagina-etapa5.html  Prévia original preservada
```

Para mudar o visual, comece por `src/styles.css` e pelos componentes. As cores de tema estão no bloco `@theme`; a lógica de arquivo e comunicação está separada em `lib/` e `hooks/`.

A [página embarcada original](../firmware/esp32/eggbot_etapa5/pagina.h) e a [prévia HTML](legacy/previa-pagina-etapa5.html) foram mantidas como referência, com o conteúdo original. Eles não são atualizados automaticamente a partir dos componentes React. A aplicação nova inicia em `index.html`.

## Conexão com o ESP32

O modo equipamento é opcional e **não foi validado em hardware nesta migração**. A aplicação usa as rotas já existentes: `/status`, `/job`, `/job/raw`, `/job/delete`, `/print/*`, `/config`, `/pen/test`, `/arm/zero` e `/cmd`.

Para usar o servidor de desenvolvimento como ponte, copie `.env.example` para `.env.local` e configure:

```dotenv
VITE_DEMO=false
VITE_API_BASE=/api
EGGBOT_TARGET=http://192.168.4.1
```

Reinicie o Vite após mudar as variáveis. O computador que executa o Vite precisa conseguir alcançar o ESP32. O Vite encaminha `/api/...` ao dispositivo removendo o prefixo; o navegador continua usando a mesma origem, evitando depender de CORS no firmware.

Nesse modo, iniciar uma impressão ou usar os controles manuais envia solicitações reais. Não ocorre troca automática para demonstração em caso de falha da conexão: a interface mostra que o equipamento está indisponível.

## Build e distribuição

```bash
npm run build
npm run preview
```

O build gera `dist/`, com os arquivos estáticos e fontes locais. `npm run preview` serve para conferir o build; o proxy configurado nesta base é destinado ao desenvolvimento.

O build **não instala a interface no ESP32**. O firmware existente só serve `PAGINA` em `pagina.h`. Hospedar o bundle React na placa exigirá uma etapa posterior de empacotamento e suporte aos arquivos estáticos, que ficou fora desta migração da interface.

Para um servidor externo de produção em modo equipamento, é necessário encaminhar o prefixo `/api` para as rotas do ESP32, ou fornecer uma API de mesma origem. Se a aplicação e a API forem servidas juntas na própria máquina, `VITE_API_BASE` poderá ser vazio, mediante suporte do servidor. Não expor uma conexão direta do navegador a outro host sem tratar a política de origem.

## Verificação

```bash
npm test
npx playwright install chromium
npm run test:e2e
npm run build
```

Os testes de dados cobrem CRC, formato do arquivo, separação dos traços, limites e padrões. Os testes de navegador cobrem desenho, desfazer/refazer, persistência do rascunho, download, importação de SVG, impressão simulada, configuração e layout móvel.

Não validam o movimento físico nem corrigem as limitações de execução descritas no levantamento de arquitetura. Em particular, o simulador mantém o comportamento histórico de espera entre comandos, que difere do Uno para alguns comandos.

A simplificação geométrica original foi preservada nesta migração; a limitação de ida e volta colinear continua pendente. O importador mantém a amostragem e separação heurística de subcaminhos, aceitando um subconjunto geométrico de SVG. Recursos de estilo, referências externas, texto, imagens e preenchimentos não são convertidos em trajetórias.

## Referências de configuração

A configuração usa o [plugin oficial do Tailwind para Vite](https://tailwindcss.com/docs/installation/using-vite). Os comandos de desenvolvimento e build seguem o [guia do Vite](https://vite.dev/guide/).
