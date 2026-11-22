# Pré-Plotter EggBot

Ferramenta opcional de diagnóstico por USB para controlar o Arduino Uno com EggDuino. Não é necessária para a interface React nem para o fluxo ESP32 → Uno.

Execute os comandos abaixo na **raiz do repositório**.

## Rápido Começo

### Instalação

```bash
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r requirements.txt
```

### Uso Básico

Conferir a CLI sem abrir a serial (não calcula trajetórias nem gera prévia):
```bash
python3 -m preplotter.main --port /dev/ttyACM0
```

Teste de movimento real, com a máquina conectada e preparada:
```bash
python3 -m preplotter.main --port /dev/ttyACM0 --run --test reference
```

## Módulos

- **planner.py** — Cálculo automático de movimento (distância euclidiana, duração)
- **eggduino.py** — Comunicação serial com Arduino (EggDuino)
- **main.py** — Interface CLI e coordenação dos testes

## Testes Integrados

```bash
# Teste 1: Referência (SM,2000,0,800)
python3 -m preplotter.main --port /dev/ttyACM0 --run --test reference

# Teste 2: Retorno
python3 -m preplotter.main --port /dev/ttyACM0 --run --test return

# Teste 3: Eixo do braço
python3 -m preplotter.main --port /dev/ttyACM0 --run --test arm

# Teste 4: Diagonal (ambos eixos)
python3 -m preplotter.main --port /dev/ttyACM0 --run --test diagonal

# Todos os testes
python3 -m preplotter.main --port /dev/ttyACM0 --run --test all
```

## Movimento Customizado

```bash
python3 -m preplotter.main \
  --port /dev/ttyACM0 \
  --run \
  --start-rotation 0 \
  --start-arm 0 \
  --end-rotation 500 \
  --end-arm 300 \
  --speed 200
```

## Documentação Completa

Veja o [guia técnico para GPT](../docs/README_GPT.md), com o mapa do código, entradas e saídas, protocolo serial e proposta de integração com o celular.

O guia técnico é um levantamento histórico. A organização atual está no [README principal](../README.md) e os firmwares estão em [firmware/](../firmware/README.md).
