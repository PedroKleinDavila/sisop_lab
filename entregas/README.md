# TP2 no Codespace

Execute na raiz do repositório, em um Codespace Ubuntu ou Debian x86_64:

```sh
./scripts/lab-build.sh
./scripts/lab-qemu.sh
```

O primeiro comando verifica recursos, instala dependências ausentes, baixa e
confere o Linux 4.13.9, configura o Buildroot e compila. Em compilações
seguintes, sincroniza o kernel local e reconstrói os pacotes do laboratório.
Os arquivos grandes ficam em `.lab/` e `output-lab/`, ignorados pelo Git.

No QEMU, faça login como `root`. Antes dos testes de disco, confira
`cat /proc/version`, `ls -l /dev/sda /dev/sdb` e
`cat /sys/block/sdb/size`. O tamanho esperado de `/dev/sdb` é 2097152
setores de 512 bytes. Se não coincidir, não rode os testes de disco.
Encerre o QEMU com `Ctrl-A X`.

As entregas ficam em três diretórios: `tutorial-2.2/`, `tutorial-2.3/`
e `sstf/`. Os ZIPs com o mesmo nome `GrupoJ.zip` permanecem em
subdiretórios distintos.
