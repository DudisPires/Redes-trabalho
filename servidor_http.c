#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORTA 8080
#define BUFFER_SIZE 4096

void erro(const char *msg) {
    perror(msg);
    exit(1);
}

void envia_arquivo(int client_sock, const char *caminho) {
    FILE *f = fopen(caminho, "rb");
    if (!f) {
        char *erro404 = "HTTP/1.0 404 Not Found\r\nContent-Type: text/plain\r\n\r\nArquivo não encontrado\n";
        write(client_sock, erro404, strlen(erro404));
        return;
    }

    char header[128];
    sprintf(header, "HTTP/1.0 200 OK\r\n\r\n");
    write(client_sock, header, strlen(header));

    char buffer[BUFFER_SIZE];
    int n;
    while ((n = fread(buffer, 1, BUFFER_SIZE, f)) > 0) {
        write(client_sock, buffer, n);
    }

    fclose(f);
}

void lista_diretorio(int client_sock, const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        char *erro500 = "HTTP/1.0 500 Internal Server Error\r\n\r\nErro ao abrir diretório\n";
        write(client_sock, erro500, strlen(erro500));
        return;
    }

    char resposta[BUFFER_SIZE];
    strcpy(resposta, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n");
    strcat(resposta, "<html><body><h2>Arquivos disponíveis:</h2><ul>");

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) {
            strcat(resposta, "<li><a href=\"");
            strcat(resposta, ent->d_name);
            strcat(resposta, "\">");
            strcat(resposta, ent->d_name);
            strcat(resposta, "</a></li>");
        }
    }
    strcat(resposta, "</ul></body></html>");

    write(client_sock, resposta, strlen(resposta));
    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <diretorio_base>\n", argv[0]);
        exit(1);
    }

    char *base_dir = argv[1];

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
        erro("Erro ao criar socket");

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORTA);

    if (bind(server_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        erro("Erro no bind");

    listen(server_sock, 5);
    printf("Servidor rodando na porta %d. Servindo %s\n", PORTA, base_dir);

    while (1) {
        int client_sock = accept(server_sock, NULL, NULL);
        if (client_sock < 0)
            erro("Erro ao aceitar conexão");

        char buffer[BUFFER_SIZE];
        int n = read(client_sock, buffer, BUFFER_SIZE - 1);
        if (n <= 0) {
            close(client_sock);
            continue;
        }

        buffer[n] = '\0';
        char metodo[8], caminho[512];
        sscanf(buffer, "%s %s", metodo, caminho);

        if (strcmp(metodo, "GET") != 0) {
            char *erro405 = "HTTP/1.0 405 Method Not Allowed\r\n\r\n";
            write(client_sock, erro405, strlen(erro405));
            close(client_sock);
            continue;
        }

        char arquivo[1024];
        snprintf(arquivo, sizeof(arquivo), "%s%s", base_dir, caminho);

        struct stat st;
        if (stat(arquivo, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                char index_path[1024];
                snprintf(index_path, sizeof(index_path), "%s/index.html", arquivo);
                if (stat(index_path, &st) == 0)
                    envia_arquivo(client_sock, index_path);
                else
                    lista_diretorio(client_sock, arquivo);
            } else {
                envia_arquivo(client_sock, arquivo);
            }
        } else {
            char *erro404 = "HTTP/1.0 404 Not Found\r\n\r\nArquivo não encontrado\n";
            write(client_sock, erro404, strlen(erro404));
        }

        close(client_sock);
    }

    close(server_sock);
    return 0;
}
