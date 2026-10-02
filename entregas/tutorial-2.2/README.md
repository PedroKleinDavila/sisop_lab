# Tutorial 2.2

O kernel 4.13.9 recebe as chamadas i386 385 (exemplo),
386 (desafio 1) e 387 (desafio 2). `kernel-integration.patch`
altera a tabela de syscalls, os protótipos e o Makefile do kernel.
`scripts/lab-setup.sh` aplica o patch e copia os três fontes
de syscall para a árvore local. O Buildroot compila os três
programas de teste para o rootfs.

No QEMU: `process_info_test`, `sleeping_test`,
`log_message_test` e `dmesg | grep 'SISOP user message'`.
Cada desafio possui seus próprios fontes e instruções no ZIP
`GrupoJ.zip` desta atividade.
