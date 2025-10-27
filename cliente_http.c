#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFFER_SIZE 4096

void erro(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s http://host[:porta]/caminho\n", argv[0]);
        exit(1);
    }

    char url[1024];
    strcpy(url, argv[1]);

    if (strncmp(url, "http://", 7) != 0) {
        erro("URL deve começar com http://");
    }

    char *host = url + 7;
    char *path = strchr(host, '/');
    char caminho[512] = "/";
    if (path) {
        strcpy(caminho, path);
        *path = '\0';
    }

    char *port_str = strchr(host, ':');
    int port = 80;
    if (port_str) {
        *port_str = '\0';
        port = atoi(port_str + 1);
    }

    struct hostent *server = gethostbyname(host);
    if (!server)
        erro("Host inválido");

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        erro("Erro ao criar socket");

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        erro("Erro ao conectar");

    char request[1024];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", caminho, host);

    write(sock, request, strlen(request));

    FILE *output = fopen("arquivos/saida_http.png", "wb");
    if (!output)
        erro("Erro ao criar arquivo");

    char buffer[BUFFER_SIZE];
    int n;
    int cabecalho = 1;
    char *body;

    while ((n = read(sock, buffer, BUFFER_SIZE)) > 0) {
        if (cabecalho) {
            body = strstr(buffer, "\r\n\r\n");
            if (body) {
                body += 4;
                fwrite(body, 1, n - (body - buffer), output);
                cabecalho = 0;
            }
        } else {
            fwrite(buffer, 1, n, output);
        }
    }

    printf("Arquivo salvo como 'saida_http.png'\n");

    fclose(output);
    close(sock);
    return 0;
}
