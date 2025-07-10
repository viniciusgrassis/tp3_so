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

int main(){
    init();
    printf("Sistema de arquivos FAT inicializado com sucesso.\n");
    return 0;
}
