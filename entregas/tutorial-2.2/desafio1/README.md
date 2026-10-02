# Desafio 1: processos dormindo

Fontes: `list_sleeping.c` (kernel) e `sleeping_test.c` (usuário).
A chamada i386 386 recebe `pid_t *out, size_t capacity` e devolve
o número de PIDs. `ENOSPC` indica vetor pequeno; `EFAULT`, endereço
inválido. Limite: 4096 entradas. A lista é um retrato momentâneo.

Depois de `./scripts/lab-build.sh`, inicialize `./scripts/lab-qemu.sh`
e execute `sleeping_test` como root. O teste cria um filho bloqueado
em `read()`, verifica que seu PID está na lista e testa `EFAULT`.
O arquivo `kernel-integration.patch` na pasta pai integra esta
syscall, a de exemplo e a do desafio 2 ao kernel.
