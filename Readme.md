# Rede P2P SCTP 
Trabalho do grau B - Rede P2P - Utilizando o protocolo SCTP

## Como executar:
1. Em um dispositivo linux, salvar o código fonte em um arquivo Peer.c
2. Abrir um terminal no local onde o arquivo foi salvo.
3. Executar o comando seguinte comando:
 > gcc -Wall -pthread -g peer.c -lsctp -o Peer
4. Alguns avisos deverão aparecer e um arquivo chamado Peer deve aparecer no diretório
5. Executar o seguinte comando para iniciar o programa:
 > ./Peer 