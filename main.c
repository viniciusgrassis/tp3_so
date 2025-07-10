#include "fat.h"

int main(){
    char comando[100];
    char argumento[100];
    while(1){
        printf("\n> ");
        scanf("%s", comando);
        if(strcmp(comando, "init") == 0){
            init();
        }else if(strcmp(comando, "load") == 0){
            load();
        }else if(strcmp(comando, "ls") == 0){
            ls();
        }else if(strcmp(comando, "exit") == 0){
            printf("Saindo...\n");
            break; 
        }else{
            printf("Comando desconhecido: %s\n", comando);
        }
        while(getchar() != '\n');
    }
    return 0;
}
