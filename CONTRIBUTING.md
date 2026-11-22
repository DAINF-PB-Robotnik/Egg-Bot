# Desenvolvimento

## Preparação

Use `npm ci` em `interface_web/` para instalar as versões do lockfile. O arquivo `.nvmrc` seleciona Node 22; o mínimo dessa linha é 22.12. Para o diagnóstico Python, crie um ambiente virtual e instale `requirements.txt` na raiz.

O frontend abre em demonstração por padrão. Configurações individuais ficam em `.env.local`, e a senha do firmware fica em `senha_wifi.h`; esses arquivos são ignorados. Os exemplos de configuração não contêm credenciais privadas.

## Antes de enviar uma alteração

```bash
cd interface_web
npm test
npm run test:e2e
npm run build
cd ..
python3 scripts/check_docs.py
git diff --check
git status --short
```

Na primeira execução dos testes de navegador, instale o Chromium com `npx playwright install chromium`. Em Linux sem as bibliotecas de sistema necessárias, use `npx playwright install --with-deps chromium`.

Descreva o problema resolvido e os testes executados no commit ou pull request. Separe resultados do simulador de resultados obtidos no hardware. Mudanças no protocolo, nas unidades ou no estado da impressão precisam de testes compatíveis com o Uno real.

## Organização

- `interface_web/src/`: interface, dados do desenho e adaptadores de comunicação.
- `firmware/`: sketches e fontes por placa; mantenha o nome da pasta igual ao sketch principal.
- `preplotter/`: diagnóstico Python opcional, executado a partir da raiz com `python3 -m preplotter.main`.
- `docs/`: análises históricas, imagens e documentos acadêmicos.

Não versione dependências instaladas, ambientes virtuais, bytecode, builds, resultados de testes ou configuração privada. Arquivos `.egg` de exemplo são permitidos: neste projeto representam trabalhos `EGGJOB`, não pacotes Python.

Preserve as atribuições e cabeçalhos dos códigos externos. Não altere o `pagina.h` do firmware ao experimentar o design React: o bundle novo ainda não é integrado automaticamente ao ESP32.
