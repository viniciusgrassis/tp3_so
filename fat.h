// fat.h - VERSÃO FINAL E COMPLETA
#ifndef FAT_H
#define FAT_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// --- Constantes ---
#define TAM_SETOR 512 
#define CLUSTER_SIZE 1024 
#define NUM_CLUSTERS 4096 
#define DATA_CLUSTERS 4086 
#define CLUSTER_FAT 8 

#define FAT_CLUSTER_LIVRE 0x0000 
#define FAT_BOOT_BLOCK 0xFFFD 
#define FAT_CLUSTER_RESERVADO 0xFFFE 
#define FAT_EOF 0xFFFF 

// --- Estruturas e Tipos ---
typedef struct{
    uint8_t filename[18]; 
    uint8_t attributes; 
    uint8_t reserved[7]; 
    uint16_t first_block; 
    uint32_t size; 
}dir_entry_t; 

typedef union{
    dir_entry_t dir[CLUSTER_SIZE / sizeof(dir_entry_t)]; 
    uint8_t data[CLUSTER_SIZE]; 
}data_cluster_t;

typedef struct {
    int found;
    dir_entry_t entry;
    int parent_cluster_idx;
    int entry_idx_in_parent;
} find_result_t;

// --- Variáveis Globais ---
extern uint16_t fat[NUM_CLUSTERS];
extern data_cluster_t data_cluster;
extern int sistema_carregado;

// --- Protótipos das Funções ---

// Comandos do Shell (em fat_commands.c)
void init(); 
void load(); 
void ls(const char* path); 
void create(char *filename); 
void mkdir(char *dirname); 
void unlink(char *name); 
void write(char *filename, char *text);
void append(char *filename, char *text);
void read(char *filename);

// Funções Auxiliares (em fat_helpers.c)
void write_fat(FILE *f);
int find_free_cluster();
int find_entry_idx_by_name(const char* name, data_cluster_t* cluster_data);
int find_free_entry_idx(data_cluster_t* cluster_data);
void separate_path(const char* full_path, char* parent_path, char* child_name);
find_result_t find_entry_by_path(const char* path);

#endif