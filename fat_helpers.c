#include "fat.h"

uint16_t fat[NUM_CLUSTERS];
data_cluster_t data_cluster;
int sistema_carregado = 0;

int find_entry_idx_by_name(const char* name, data_cluster_t* cluster_data){ // Busca o índice da entrada pelo nome
    int entradas_por_cluster = CLUSTER_SIZE / sizeof(dir_entry_t);
    for(int i = 0; i < entradas_por_cluster; i++) {
        if(cluster_data->dir[i].filename[0] != 0x00 &&
            strcmp((char*)cluster_data->dir[i].filename, name) == 0){
            return i; // Retorna o índice
        }
    }
    return -1; // Retorna -1 se não encontrar a entrada
}


int find_free_entry_idx(data_cluster_t* cluster_data){ // Busca a primeira entrada livre no cluster
    int entradas_por_cluster = CLUSTER_SIZE / sizeof(dir_entry_t);
    for(int i = 0; i < entradas_por_cluster; i++){
        if(cluster_data->dir[i].filename[0] == 0x00){
            return i; // Retorna índice da primeira entrada livre
        }
    }
    return -1; // retorna -1 se estiver cheio
}

int find_free_cluster(){ // Busca o primeiro cluster livre
    for(int i = 10; i < NUM_CLUSTERS; i++){
        if(fat[i] == FAT_CLUSTER_LIVRE){
            return i; // Retorna o índice do primeiro cluster livre
        }
    }
    return -1; // Retorna -1 se não encontrar cluster livre
}

void write_fat(FILE *f){ // recebe o ponteiro do arquivo e escreve a FAT no disco
    fseek(f, CLUSTER_SIZE, SEEK_SET);
    fwrite(fat, sizeof(fat), 1, f);
}

find_result_t find_entry_by_path(const char* path){ // Quebra o caminho para caminhar pela FAT
    find_result_t result;
    result.found = 0; 

    char path_copy[256];
    strncpy(path_copy, path, 255);
    path_copy[255] = '\0';

    uint16_t current_cluster_idx = 9;
    data_cluster_t current_cluster_data;

    if(strcmp(path, "/") == 0){
        result.found = 1;
        strcpy((char*)result.entry.filename, "/");
        result.entry.attributes = 1; 
        result.entry.first_block = 9;
        result.parent_cluster_idx = 0; 
        return result;
    }

    char *token = strtok(path_copy, "/");
    
    while(token != NULL){
        FILE *f = fopen("fat.part", "rb");
        fseek(f, current_cluster_idx * CLUSTER_SIZE, SEEK_SET);
        fread(&current_cluster_data, CLUSTER_SIZE, 1, f);
        fclose(f);

        int entry_idx = find_entry_idx_by_name(token, &current_cluster_data);

        if(entry_idx == -1){ // Não encontrou
            result.found = 0;
            return result;
        }

        dir_entry_t found_entry = current_cluster_data.dir[entry_idx];
        char *next_token = strtok(NULL, "/");

        if(next_token == NULL){ // Encontrou
            result.found = 1;
            result.entry = found_entry;
            result.parent_cluster_idx = current_cluster_idx;
            result.entry_idx_in_parent = entry_idx;
            return result;
        }else{ 
            if(found_entry.attributes != 1){ 
                result.found = 0;
                return result;
            }
            current_cluster_idx = found_entry.first_block;
            token = next_token;
        }
    }
    return result;
}

void separate_path(const char* full_path, char* parent_path, char* child_name){ // Separa o que é caminho e o que é nome do arquivo ou diretório
    const char *last_slash = strrchr(full_path, '/');
    
    if(last_slash == NULL){ //É um arquivo/dir na raiz
        strcpy(parent_path, "/");
        strcpy(child_name, full_path);
    }else if(last_slash == full_path){ // O caminho é "/arquivo"
        strcpy(parent_path, "/");
        strcpy(child_name, last_slash + 1);
    }else{ // O caminho é "/pasta/arquivo"
        strncpy(parent_path, full_path, last_slash - full_path);
        parent_path[last_slash - full_path] = '\0';
        strcpy(child_name, last_slash + 1);
    }
}
