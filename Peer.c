#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/ioctl.h>

#define ECHOMAX 1024
#define QUANTPORTAS 4

char name[20];
int porta;
int portas[QUANTPORTAS];

void sending();
void receiving(int sock);
void *receive_thread(void *sock);

int main(void)
{
    //Cria lista de portas
    portas[0] = 3333;
    portas[1] = 4444;
    portas[2] = 5555;
    portas[3] = 6666;

    int sock;
    struct sockaddr_in server;

    // cria o socket
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) == -1)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    //Inicializando o endereco do servidor

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    
    for(int x = 0; x < QUANTPORTAS; x++){

        server.sin_port = htons(portas[x]);
        if(bind(sock, (struct sockaddr *)&server, sizeof(server)) >= 0){
            porta = portas[x];
            break;
        }
    }

    //Informacoes do servidor criado
    printf("IP server is: %s\n", inet_ntoa(server.sin_addr));
    printf("port is: %d\n", (int)ntohs(server.sin_port));

    
    if (listen(sock, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    //cria thread para receber simultaneamente
    pthread_t tid;
    pthread_create(&tid, NULL, &receive_thread, &sock); 
    
    //executa o envio
    while(1)
    {
        sending();
    }

    close(sock);

    return 0;
}

//Envio do comando
void sending()
{
    char linha[ECHOMAX];
    char linhaResposta[ECHOMAX] = {0};
    int sock = 0;
    struct sockaddr_in serv_addr;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    
    printf("Digite o comando:\n");
    gets(linha);
    if(linha[0] != "\n"){
        linha[strcspn(linha, "\n")] = 0;
        
        printf("Executado localmente:\n");
        system(linha);
        printf("\n");

        //tenta enviar para todas portas mapeadas e que nao sejam ele mesmo
        for(int x = 0; x < QUANTPORTAS; x++)
        {   
            sock = 0;
            if(portas[x] != porta)
            {
                if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
                {
                    printf("\n Erro ao criar socket \n");
                    return;
                }   
                serv_addr.sin_port = htons(portas[x]);
                //tenta criar conexao com cada porta
                if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
                {
                    printf("\nConexao falhou com %d \n", portas[x]);
                    continue;
                }
                //envia e recebe a resposta
                sctp_sendmsg(sock, (void *) linha, sizeof(linha), NULL, 0, 0, 0, 0, 0, 0 );
                printf("\nMensagem enviada na porta %d\n",portas[x]);
                sctp_recvmsg(sock, &linhaResposta, sizeof(linhaResposta), NULL, 0, 0, 0);
                printf("\nMensagem recebida de porta %d\n",portas[x]);
                printf("%s\n", linhaResposta);
                close(sock);
            }
        }
    }
}

//thread to receive
void *receive_thread(void *sock)
{
    int s_fd = *((int *)sock);
    while (1)
    {
        receiving(s_fd);
    }
}

//Receiving messages on our port
void receiving(int sock)
{
    struct sockaddr_in endereco;
    char buffer[ECHOMAX] = {0};
    int addrlen = sizeof(endereco);
    fd_set current_sockets, ready_sockets;
    
    FILE *pipe;
    char resultadoComando[ECHOMAX];
    char linhaComando[ECHOMAX];

    //Inicializa o conjunto de sockets
    FD_ZERO(&current_sockets);
    FD_SET(sock, &current_sockets);
    int count = 0;
    while (1)
    {
        count++;
        ready_sockets = current_sockets;

        if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0)
        {
            perror("Error");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++)
        {
            if (FD_ISSET(i, &ready_sockets))
            {

                if (i == sock)
                {
                    int client_socket;
                    //Aceita a conexao
                    if ((client_socket = accept(sock, (struct sockaddr *)&endereco,
                                                (socklen_t *)&addrlen)) < 0)
                    {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    FD_SET(client_socket, &current_sockets);
                }
                else
                {   
                    //Recebe e executa o comando
                    sctp_recvmsg(i, &buffer, sizeof(buffer), NULL, 0, 0, 0);
                    //printf("\nComando a executar:%s\n", buffer);
                    pipe = popen(buffer, "r");
                    if (pipe != NULL)
                    {
                        while (fgets(linhaComando, sizeof(linhaComando), pipe) != NULL){
                            strcat(resultadoComando, linhaComando);
                    }
                    //Envia o resultado do comando
                    sctp_sendmsg(i, (void *) resultadoComando, sizeof(resultadoComando), NULL, 0, 0, 0, 0, 0, 0 );
                    pclose(pipe);
                    resultadoComando[0] = '\0';
                    
                }
                    FD_CLR(i, &current_sockets);
                }
            }
        }

        if (count == (FD_SETSIZE * 2))
            break;
    }
}