# SystemInfo

## Descrição

Servidor HTTP executado em uma distribuição Linux embarcada
gerada com Buildroot. O serviço coleta informações do sistema
através de /proc e /sys e as disponibiliza em JSON através do
endpoint GET /status na porta 8080.

## Execução

Servidor:
http://192.168.1.10:8080/status

Teste:
curl http://192.168.1.10:8080/status

## Informações coletadas

### datetime
Obtido combinando o campo btime de /proc/stat com o tempo
decorrido disponível em /proc/uptime.

### uptime_seconds
Obtido de /proc/uptime.

### CPU
Modelo e frequência obtidos de /proc/cpuinfo.
O uso da CPU é calculado comparando duas leituras de /proc/stat.

### Memória
Obtida de /proc/meminfo.

### Sistema operacional
Obtido de /proc/version.

### Processos
Os PIDs são identificados pelos diretórios numéricos de /proc.
O nome é lido de /proc/<pid>/comm.

### Discos
Obtidos de /sys/block.
O tamanho é calculado a partir de /sys/block/<device>/size.

### USB
Obtido de /sys/bus/usb/devices.

### Rede
As interfaces são obtidas de /sys/class/net.
Os endereços IPv4 são identificados através de
/proc/net/fib_trie e /proc/net/route.