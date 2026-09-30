# Trabalho Prático 2 — SSTF

## O que foi implementado

`sstf_iosched.c` registra um escalonador de fila simples para Linux 4.13.9.
Cada despacho seleciona a requisição pendente mais próxima da posição final
da anterior; empate mantém a mais antiga. `sector_read.c` produz leituras
concorrentes e alinhadas em `/dev/sdb`; `run_sstf.sh` seleciona o módulo,
desabilita mesclagem/read-ahead, esvazia caches e guarda o log.

## Execução no Codespace

Na raiz do repositório:

```sh
./scripts/lab-build.sh
./scripts/lab-qemu.sh
```

No convidado QEMU, como root:

```sh
cat /proc/version
cat /sys/block/sdb/size
run_sstf 16 16
cat /sys/block/sdb/queue/scheduler
head /mnt/results/sstf.log
```

O script exige exatamente 2097152 setores em `/dev/sdb`, nunca escreve
nesse dispositivo, e salva o log no disco de resultados `/dev/sdc`.
Saia do QEMU com `Ctrl-A X`. No Codespace:

```sh
./scripts/lab-collect.sh
```

Isso gera `report/SSTF.pdf` e `report/SSTF-ordens.csv` com os
dados **reais**. `report/SSTF-referencia.pdf` usa apenas os 50 setores
do enunciado para mostrar o método; ele está marcado como referência
analítica e não substitui a medição da VM.

## Apresentação ao professor

1. Mostrar o código de `sstf_dispatch`, a variável da posição da
   cabeça e a regra de desempate.
2. Mostrar `modprobe sstf_iosched` e `[sstf]` em
   `/sys/block/sdb/queue/scheduler`.
3. Explicar o papel de `O_DIRECT`, dos filhos concorrentes,
   da cache e da mesclagem.
4. Abrir o PDF experimental, explicar a soma das distâncias e
   a limitação do QEMU para medir tempo real.
5. Reconhecer que SSTF pode causar starvation de setores distantes.
