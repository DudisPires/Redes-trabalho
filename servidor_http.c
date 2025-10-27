#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORTA 8080
#define BUFFER_SIZE 8192

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

    // Cabeçalho básico (melhorar futuramente com Content-Type real)
    char header[128];
    sprintf(header, "HTTP/1.0 200 OK\r\n\r\n");
    write(client_sock, header, strlen(header));

    char buffer[BUFFER_SIZE];
    int n;
    while ((n = fread(buffer, 1, BUFFER_SIZE, f)) > 0)
        write(client_sock, buffer, n);

    fclose(f);
}

void lista_diretorio(int client_sock, const char *dir_path, const char *url_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        char *erro500 = "HTTP/1.0 500 Internal Server Error\r\n\r\nErro ao abrir diretório\n";
        write(client_sock, erro500, strlen(erro500));
        return;
    }

    // Cabeçalho HTTP
    char resposta[BUFFER_SIZE * 2];
    snprintf(resposta, sizeof(resposta),
        "HTTP/1.0 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
        "<!DOCTYPE html><html lang='pt-br'><head>"
        "<meta charset='UTF-8'>"
        "<title>Listagem de arquivos</title>"
        "<style>"
        "body { background-color: #111; color: #eee; font-family: Arial, sans-serif; text-align: center; }"
        "h2 { margin-top: 20px; color: #fff; }"
        "ul { list-style: none; padding: 0; max-width: 500px; margin: 0 auto; }"
        "li { margin: 10px 0; padding: 10px; background: #222; border-radius: 8px; }"
        "a { color: #66f; text-decoration: none; font-weight: bold; }"
        "a:hover { color: #fff; text-decoration: underline; }"
        "</style></head><body>"
        "<h2>Arquivos disponíveis em %s</h2><ul>",
        url_path);

    write(client_sock, resposta, strlen(resposta));

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
            continue;

        char linha[1024];
        snprintf(linha, sizeof(linha),
            "<li><a href=\"%s%s%s\">%s</a></li>",
            url_path,
            (url_path[strlen(url_path) - 1] == '/') ? "" : "/",
            ent->d_name,
            ent->d_name);
        write(client_sock, linha, strlen(linha));
    }

    const char *fim =
        "</ul><p style='margin-top:40px;color:#888;font-size:0.9em;'>Servidor C - Porta 8080</p></body></html>";
    write(client_sock, fim, strlen(fim));

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
                    lista_diretorio(client_sock, arquivo, caminho);
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
