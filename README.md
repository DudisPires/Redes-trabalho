# 🖥️ Trabalho de Redes — Cliente e Servidor HTTP em C  

---

## 📘 Descrição  

Este projeto implementa dois programas em **C** que simulam o funcionamento básico da web:  

1. **Cliente HTTP (`meu_navegador`)**  
   - Realiza requisições HTTP para servidores.  
   - Recebe e salva os arquivos localmente.  
   - Exemplo de uso:  
     ```
     ./meu_navegador http://localhost:8080/image.png
     ```

2. **Servidor HTTP (`meu_servidor`)**  
   - Fornece arquivos de um diretório local via protocolo HTTP.  
   - Se existir um `index.html`, ele é retornado.  
   - Caso contrário, o servidor gera uma listagem automática dos arquivos disponíveis.  
   - Exemplo de uso:  
     ```
     ./meu_servidor site-teste
     ```
  

