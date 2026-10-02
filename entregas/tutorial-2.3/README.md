# Desafio do Tutorial 2.3 — XTEA

Este driver foi adaptado do `simple_driver.tar.gz` fornecido na disciplina.
Ele cria `/dev/xtea_driver`. As quatro palavras de 32 bits são passadas
na carga do módulo; cada parâmetro exige exatamente oito dígitos hexadecimais.
Há valores padrão do exemplo do tutorial.

Com a distribuição construída por `./scripts/lab-build.sh`, inicie o QEMU,
faça login como root e rode:

```sh
modprobe xtea_driver key0=f0e1d2c3 key1=b4a59687 key2=78695a4b key3=3c2d1e0f
ls -l /dev/xtea_driver
xtea_test
dmesg | tail
rmmod xtea_driver
```

O protocolo é uma linha ASCII escrita no dispositivo:
`enc <quantidade-de-bytes> <dados-hex>` ou
`dec <quantidade-de-bytes> <dados-hex>`. A quantidade deve ser múltipla
de 8, de 8 a 256 bytes; os dados têm exatamente dois dígitos hex por
byte. A leitura retorna os bytes transformados em hex e uma quebra de
linha. Cada descritor aberto mantém sua própria resposta. A ordem dos
bytes dentro de cada palavra XTEA é big-endian.

O ZIP desta entrega inclui `xtea_driver.c`, `xtea_test.c`,
`Makefile` e esta instrução. O Buildroot descobre o compilador cruzado
por sua infraestrutura, sem caminho fixo do tutorial.
