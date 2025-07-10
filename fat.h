#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define TAM_SETOR 512
#define TAM_CLUSTER 1024
#define NUM_CLUSTERS 4096

#define CLUSTER_BOOT_BLOCK 1
#define CLUSTER_FAT 8
#define CLUSTER_ROOT_DIR 1
#define CLUSTER_DATA 4086

#define FAT_CLUSTER_LIVRE 0x0000
#define FAT_BOOT_BLOCK 0xFFFD
#define FAT_CLUSTER_RESERVADO 0xFFFE
#define FAT_EOF 0xFFFF

typedef struct{
    uint8_t nome[18];
    uint8_t atributos;
    uint8_t reservado[7];
    uint16_t primeiroBloco;
    uint32_t tamanho;
}entrada_t;

uint16_t fat[NUM_CLUSTERS];

union cluster_data{
    uint8_t data[TAM_CLUSTER];
    entrada_t dir[TAM_CLUSTER / sizeof(entrada_t)];
};

void init();



