#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fat.h"

void init(){
    FILE *f = fopen("fat.part", "wb");
    if(f == NULL){
        perror("Erro ao criar o arquivo fat.part");
        return;
    }

    uint8_t boot_block_data[TAM_CLUSTER];
    memset(boot_block_data, 0xbb, TAM_CLUSTER);
    fwrite(boot_block_data, TAM_CLUSTER, 1, f);

    fat[0] = FAT_BOOT_BLOCK;
    for(int i = 1; i <= CLUSTER_FAT; i++){
        fat[i] = FAT_CLUSTER_RESERVADO;
    }

    fat[9] = FAT_EOF;
    for(int i = 10; i < NUM_CLUSTERS; i++){
        fat[i] = FAT_CLUSTER_LIVRE;
    }

    fwrite(fat, sizeof(fat), 1, f);

    uint8_t root_dir_data[TAM_CLUSTER];
    memset(root_dir_data, 0x00, TAM_CLUSTER);
    fwrite(root_dir_data, TAM_CLUSTER, 1, f);

    uint8_t empty_cluster[TAM_CLUSTER];
    memset(empty_cluster, 0x00, TAM_CLUSTER);
    for (int i = 0; i < CLUSTER_DATA; i++) {
        fwrite(empty_cluster, TAM_CLUSTER, 1, f);
    }

    fclose(f);
}

int main(){
    init();
    printf("Sistema de arquivos FAT inicializado com sucesso.\n");
    return 0;
}
