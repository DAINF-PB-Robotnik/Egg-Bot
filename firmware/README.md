# Firmwares do Egg-Bot

Os fontes foram separados por placa, preservando o conteúdo recebido. Esta organização não altera o protocolo nem corrige as limitações de movimento descritas na [análise de arquitetura](../docs/ANALISE_ARQUITETURA.md).

## Arduino Uno

Abra [uno/EggDuino/EggDuino.ino](uno/EggDuino/EggDuino.ino) na Arduino IDE. A pasta e o sketch principal agora têm o mesmo nome. Os demais arquivos `.ino`, as bibliotecas locais AccelStepper/SerialCommand e `button.h` devem permanecer juntos.

Selecione a placa Arduino Uno e a porta correspondente. O sketch usa a biblioteca `Servo` e componentes AVR. As instruções históricas estão no [README original](uno/EggDuino/README.md).

## ESP32

Abra [esp32/eggbot_etapa5/eggbot_etapa5.ino](esp32/eggbot_etapa5/eggbot_etapa5.ino). A [SPEC recebida](esp32/eggbot_etapa5/SPEC-etapa5-configuracao-interface.md) registra ESP32 Dev Module, pacote esp32 3.3.x e partição “Default 4MB with spiffs”, usada pelo LittleFS.

Para personalizar a senha, copie `esp32/senha_wifi.exemplo.h` para `esp32/eggbot_etapa5/senha_wifi.h` e edite o arquivo local. Ele está excluído do versionamento. Sem esse arquivo, o firmware usa a senha pública de demonstração declarada no sketch.

O ESP32 serve a interface antiga de `pagina.h`. A aplicação React está em [interface_web/](../interface_web/README.md) e seu build não é instalado automaticamente na placa. A prévia antiga com simulador está em [interface_web/legacy/](../interface_web/legacy/previa-pagina-etapa5.html).

## Comunicação e verificação

Os dois firmwares usam UART a 9600 baud. O comando de movimento é `SM,duration_ms,arm_steps,rotation_steps\r`, com braço antes da rotação. Pinos e interpretação dos demais comandos estão documentados na análise de arquitetura.

Não houve compilação para as placas ou movimentação do equipamento nesta organização do repositório. A CI cobre a interface web, os documentos e a ferramenta Python; não valida estes firmwares em hardware.

Preserve os cabeçalhos de autoria e os arquivos de licença distribuídos com os fontes.
