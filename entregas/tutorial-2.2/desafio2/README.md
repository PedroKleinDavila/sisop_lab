# Desafio 2: mensagem no log do kernel

Fontes: `log_message.c` (kernel) e `log_message_test.c` (usuário).
A chamada i386 387 recebe uma string terminada em NUL de no máximo
255 caracteres. Devolve o comprimento escrito no log; entradas
inválidas geram `EFAULT`, `EINVAL` ou `E2BIG`.

Depois de `./scripts/lab-build.sh`, inicialize `./scripts/lab-qemu.sh`,
execute `log_message_test` e confira
`dmesg | grep 'SISOP user message'`. O teste cobre o limite,
uma mensagem longa demais, uma vazia e um ponteiro inválido.
O arquivo `kernel-integration.patch` na pasta pai contém a
integração desta syscall ao kernel.
