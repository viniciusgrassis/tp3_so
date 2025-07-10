#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fat.h"

int sistema_carregado = 0;

void init(){
    FILE *f = fopen("fat.part", "wb");
    if(f == NULL){
        perror("Erro ao criar o arquivo fat.part");
        return;
    }

    uint8_t boot_block_data[CLUSTER_SIZE];
    memset(boot_block_data, 0xbb, CLUSTER_SIZE);
    fwrite(boot_block_data, CLUSTER_SIZE, 1, f);

    fat[0] = FAT_BOOT_BLOCK;
    for(int i = 1; i <= CLUSTER_FAT; i++){
        fat[i] = FAT_CLUSTER_RESERVADO;
    }

    fat[9] = FAT_EOF;
    for(int i = 10; i < NUM_CLUSTERS; i++){
        fat[i] = FAT_CLUSTER_LIVRE;
    }

    fwrite(fat, sizeof(fat), 1, f);

    uint8_t root_dir_data[CLUSTER_SIZE];
    memset(root_dir_data, 0x00, CLUSTER_SIZE);
    fwrite(root_dir_data, CLUSTER_SIZE, 1, f);

    uint8_t empty_cluster[CLUSTER_SIZE];
    memset(empty_cluster, 0x00, CLUSTER_SIZE);
    for (int i = 0; i < CLUSTER_SIZE; i++) {
        fwrite(empty_cluster, CLUSTER_SIZE, 1, f);
    }

    fclose(f);
}

void load(){
    FILE *f = fopen("fat.part", "rb");
    if(f == NULL){
        printf("Erro! 'fat.part' não encontrado!\n");
        printf("Execute 'init' primeiro.\n");
        return;
    }
    fseek(f, CLUSTER_SIZE, SEEK_SET);

    size_t bytes_lidos = fread(fat, sizeof(fat), 1, f);

    if(bytes_lidos < 1){
        printf("Erro ao ler a FAT.\n");
        fclose(f);
        return;
    }

    fclose(f);
    sistema_carregado = 1;
    printf("Sistema de arquivos FAT carregado com sucesso.\n");
}

void ls(){
    if(!sistema_carregado){
        printf("Sistema de arquivos não carregado. Execute 'load' primeiro.\n");
        return;
    }

    FILE *f = fopen("fat.part", "rb");
    if(f == NULL){
        perror("Erro ao abrir o arquivo fat.part");
        return;
    }

    long int offset_raiz = 9*CLUSTER_SIZE;
    fseek(f, offset_raiz, SEEK_SET);

    fread(&data_cluster, CLUSTER_SIZE, 1, f);
    fclose(f);

    printf("Conteúdo do diretório raiz:\n");
    printf("Tipo\tTamanho\tNome\n");
    printf("----\t------\t----\n");

    int entradas_por_cluster = CLUSTER_SIZE / sizeof(dir_entry_t);

    for(int i = 0; i < entradas_por_cluster; i++){
        if(data_cluster.dir[i].filename[0] != 0x00){
            if(data_cluster.dir[i].attributes == 1){
                printf("[D]  | ");
            }else{
                printf("[A]  | ");
            }
            printf("%-15u | %s\n", data_cluster.dir[i].size, data_cluster.dir[i].filename);
        }
    }
}

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
