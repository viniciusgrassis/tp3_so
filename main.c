#include "fat.h"

int main(){

    char linha_inteira[200];
    char comando[100];
    char argumento[100];

    while(1){
        printf("> ");

        if(fgets(linha_inteira, sizeof(linha_inteira), stdin) == NULL){
            break;
        }

        memset(comando, 0, sizeof(comando));
        memset(argumento, 0, sizeof(argumento));

        sscanf(linha_inteira, "%s %s", comando, argumento);

        if(strcmp(comando, "init") == 0){
            init();
        }else if(strcmp(comando, "load") == 0){
            load();
        }else if(strcmp(comando, "ls") == 0){
            ls();
        }else if(strcmp(comando, "create") == 0){
            if(strlen(argumento) == 0){
                printf("O comando create requer um nome de arquivo!\n");
            }else{
                create(argumento);
            }
        }else if(strcmp(comando, "mkdir") == 0){
            if(strlen(argumento) == 0){
                printf("O comando mkdir requer um nome de diretório!\n");
            }else{
                mkdir(argumento);
            }
        }else if(strcmp(comando, "unlink") == 0){
            if(strlen(argumento) == 0){
                printf("O comando unlink requer um nome de arquivo ou diretório!\n");
            }else{
                unlink(argumento);
            }
        }else if(strcmp(comando, "exit") == 0){
            printf("Saindo...\n");
            break; 
        }else if(strlen(comando) > 0){
            printf("Comando desconhecido: %s\n", comando);
        }
    }
    return 0;
}
