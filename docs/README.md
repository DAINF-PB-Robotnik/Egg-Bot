# Documentação do Egg-Bot

Para a estrutura e execução atuais, comece pelo [README principal](../README.md), pelo [guia da interface](../interface_web/README.md) e pelo [guia dos firmwares](../firmware/README.md).

## Levantamentos históricos

- [Análise da arquitetura — novembro de 2026](ANALISE_ARQUITETURA.md): revisão dos códigos Python, ESP32, Uno e interface anterior, com problemas e propostas. Foi produzida antes da migração React e da organização para o GitHub.
- [Contexto para GPT — setembro de 2026](README_GPT.md): descrição do pré-plotter e do driver Inkscape antes da chegada do código web.

Esses documentos preservam o contexto da época, inclusive nomes antigos de pastas e problemas de instalação já tratados. Seus links foram ajustados para as localizações atuais. Não use as árvores históricas como guia de instalação.

As limitações de firmware apontadas na análise não foram corrigidas pela migração visual ou pela reorganização dos arquivos.

## Projeto acadêmico

- [Proposta de projeto em LaTeX](<propostas de projeto.tex>)
- [Relatório em PDF](Relatório_Final___Oficina.pdf): versão recebida, ainda com seções em elaboração.
- [Foto da montagem](imagem02.jpeg) e [imagem de capa](capa.jpeg)

## Localizações atuais

| Conteúdo citado nos levantamentos | Localização atual |
| --- | --- |
| `EggDuino-master/` | [firmware/uno/EggDuino/](../firmware/uno/EggDuino/EggDuino.ino) |
| `eggbot_web/` e `eggbot_etapa5_web/eggbot_etapa5/` | [firmware/esp32/eggbot_etapa5/](../firmware/esp32/eggbot_etapa5/eggbot_etapa5.ino) |
| Prévia HTML anterior | [interface_web/legacy/](../interface_web/legacy/previa-pagina-etapa5.html) |
| Interface React atual | [interface_web/](../interface_web/README.md) |
