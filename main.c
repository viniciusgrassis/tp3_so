#include "fat.h"

int main(){

    char linha_inteira[200];
    char comando[100];
    char argumento1[100];
    char argumento2[100];
    
    while(1){
        printf("> ");

        if(fgets(linha_inteira, sizeof(linha_inteira), stdin) == NULL){
            break;
        }

        memset(comando, 0, sizeof(comando));
        memset(argumento1, 0, sizeof(argumento1));
        memset(argumento2, 0, sizeof(argumento2));

        sscanf(linha_inteira, "%s %s %s", comando, argumento1, argumento2);

        if(strcmp(comando, "init") == 0){
            init();
        }else if(strcmp(comando, "load") == 0){
            load();
        }else if(strcmp(comando, "ls") == 0){
            if(strlen(argumento1) == 0){
                ls("/"); 
            }else{
                ls(argumento1); 
            }
        }else if(strcmp(comando, "create") == 0){
            if(strlen(argumento1) == 0){
                printf("O comando create requer um nome de arquivo!\n");
            }else{
                create(argumento1);
            }
        }else if(strcmp(comando, "mkdir") == 0){
            if(strlen(argumento1) == 0){
                printf("O comando mkdir requer um nome de diretório!\n");
            }else{
                mkdir(argumento1);
            }
        }else if(strcmp(comando, "unlink") == 0){
            if(strlen(argumento1) == 0){
                printf("O comando unlink requer um nome de arquivo ou diretório!\n");
            }else{
                unlink(argumento1);
            }
        }else if(strcmp(comando, "write") == 0){ 
            if(strlen(argumento1) == 0 || strlen(argumento2) == 0){
                printf("O comando write requer um nome de arquivo e um texto.\n");
            }else{
                write(argumento1, argumento2);
            }
        }else if(strcmp(comando, "read") == 0){ 
            if(strlen(argumento1) == 0){
                printf("O comando read requer um nome de arquivo!\n");
            }else{
                read(argumento1);
            }
        }else if(strcmp(comando, "append") == 0){
            if(strlen(argumento1) == 0 || strlen(argumento2) == 0){
                printf("O comando append requer um nome de arquivo e um texto.\n");
            }else{
                append(argumento1, argumento2);
            }
        }
        else if(strcmp(comando, "exit") == 0){
            printf("Saindo...\n");
            break;
        }else if(strlen(comando) > 0){
            printf("Comando desconhecido: %s\n", comando);
        }
    }
    return 0;
}
