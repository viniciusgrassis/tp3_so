#include "fat.h"

uint16_t fat[NUM_CLUSTERS];
data_cluster_t data_cluster;
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
    for (int i = 0; i < DATA_CLUSTERS; i++) {
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

void create(char *filename){
    if(!sistema_carregado){
        printf("O sistema de arquivos não foi carregado. Execute 'load'.\n");
        return;
    }

    if(strlen(filename) > 17){
        printf("Nome de arquivo muito longo! Máximo 17 caracteres.\n");
        return;
    }

    FILE *f = fopen("fat.part", "r+b");
    if(f == NULL){
        perror("Erro ao abrir o arquivo fat.part");
        return;
    }

    long int offset_raiz = 9*CLUSTER_SIZE;
    fseek(f, offset_raiz, SEEK_SET);
    fread(&data_cluster, CLUSTER_SIZE, 1, f);

    int entrada_livre_idx = -1;
    int entradas_por_cluster = CLUSTER_SIZE / sizeof(dir_entry_t);

    for(int i = 0; i < entradas_por_cluster; i++){
        if(data_cluster.dir[i].filename[0] != 0x00 && strcmp((char*)data_cluster.dir[i].filename, filename) == 0){
            printf("Erro: Arquivo ou diretório '%s' já existe!\n", filename);
            fclose(f);
            return;
        }
        if(data_cluster.dir[i].filename[0] == 0x00 && entrada_livre_idx == -1){
            entrada_livre_idx = i;
        }
    }

    if(entrada_livre_idx == -1){
        printf("Erro: Diretório raiz cheio!\n");
        fclose(f);
        return;
    }

    dir_entry_t *nova_entrada = &data_cluster.dir[entrada_livre_idx];
    strcpy((char*)nova_entrada->filename, filename);
    nova_entrada->attributes = 0;
    nova_entrada->size = 0;
    nova_entrada->first_block = 0;

    fseek(f, offset_raiz, SEEK_SET);
    fwrite(&data_cluster, CLUSTER_SIZE, 1, f);

    fclose(f);
    printf("Arquivo '%s' criado com sucesso.\n", filename);
}

int find_free_cluster(){
    for(int i = 10; i < NUM_CLUSTERS; i++){
        if(fat[i] == FAT_CLUSTER_LIVRE){
            return i;
        }
    }
    return -1; // Nenhum cluster livre encontrado
}

void write_fat(){
    FILE *f = fopen("fat.part", "r+b");
    if(f == NULL){
        perror("Erro ao abrir o arquivo fat.part");
        return;
    }
    fseek(f, CLUSTER_SIZE, SEEK_SET);
    fwrite(fat, sizeof(fat), 1, f);
    fclose(f);
}

void mkdir(char *dirname){
    if(!sistema_carregado){
        printf("O sistema de arquivos não foi carregado. Execute 'load'.\n");
        return;
    }

    if(strlen(dirname) > 17){
        printf("Nome de diretório muito longo! Máximo 17 caracteres.\n");
        return;
    }

    int free_cluster_idx = find_free_cluster();
    if(free_cluster_idx == -1){
        printf("Erro: Não há clusters livres disponíveis.\n");
        return;
    }

    FILE *f = fopen("fat.part", "r+b");
    if(f == NULL){
        perror("Erro ao abrir o arquivo fat.part");
        return;
    }

    long int offset_raiz = 9 * CLUSTER_SIZE;
    fseek(f, offset_raiz, SEEK_SET);
    fread(&data_cluster, CLUSTER_SIZE, 1, f);

    int entrada_livre_idx = -1;
    int entradas_por_cluster = CLUSTER_SIZE / sizeof(dir_entry_t);

    for(int i = 0; i < entradas_por_cluster; i++){
        if(strcmp((char*)data_cluster.dir[i].filename, dirname) == 0){
            printf("Erro: Diretório '%s' já existe!\n", dirname);
            fclose(f);
            return;
        }
        if(data_cluster.dir[i].filename[0] == 0x00 && entrada_livre_idx == -1){
            entrada_livre_idx = i;
        }
    }
    if(entrada_livre_idx == -1){
        printf("Erro: Diretório raiz cheio!\n");
        fclose(f);
        return;
    }
    fat[free_cluster_idx] = FAT_EOF;
    write_fat();

    dir_entry_t *nova_entrada = &data_cluster.dir[entrada_livre_idx];
    strcpy((char*)nova_entrada->filename, dirname);
    nova_entrada->attributes = 1; // Diretório
    nova_entrada->size = 0;
    nova_entrada->first_block = (uint16_t)free_cluster_idx;

    fseek(f, offset_raiz, SEEK_SET);
    fwrite(&data_cluster, CLUSTER_SIZE, 1, f);

    uint8_t empty_cluster[CLUSTER_SIZE];
    memset(empty_cluster, 0x00, CLUSTER_SIZE);
    fseek(f, free_cluster_idx * CLUSTER_SIZE, SEEK_SET);
    fwrite(empty_cluster, CLUSTER_SIZE, 1, f);

    fclose(f);
    printf("Diretório '%s' criado com sucesso.\n", dirname);
}

// Substitua sua função unlink inteira por esta
void unlink(char *name){
    if(!sistema_carregado){
        printf("Sistema de arquivos não carregado. Execute 'load' primeiro.\n");
        return;
    }

    FILE *f = fopen("fat.part", "r+b");
    if(f == NULL){
        perror("Erro ao abrir o arquivo fat.part");
        return;
    }

    long int offset_raiz = 9 * CLUSTER_SIZE;
    fseek(f, offset_raiz, SEEK_SET);
    fread(&data_cluster, CLUSTER_SIZE, 1, f);

    int target_idx = -1;
    int entradas_por_cluster = CLUSTER_SIZE / sizeof(dir_entry_t);
    for(int i = 0; i < entradas_por_cluster; i++){
        if(data_cluster.dir[i].filename[0] != 0x00 &&
            strcmp((char*)data_cluster.dir[i].filename, name) == 0){
            target_idx = i;
            break;
        }
    }

    if(target_idx == -1){
        printf("Erro: Arquivo ou diretorio '%s' nao encontrado.\n", name);
        fclose(f);
        return;
    }

    dir_entry_t *entry = &data_cluster.dir[target_idx];
    
    if(entry->attributes == 0){ 
        uint16_t current_cluster = entry->first_block;
        while(current_cluster != 0 && current_cluster != FAT_EOF){
            uint16_t next_cluster = fat[current_cluster];
            fat[current_cluster] = FAT_CLUSTER_LIVRE;
            current_cluster = next_cluster;
        }
        write_fat(); 
        printf("Arquivo '%s' apagado.\n", name);

    }else{ 
        uint8_t temp_dir_cluster[CLUSTER_SIZE];
        dir_entry_t *dir_content = (dir_entry_t*) temp_dir_cluster; 

        fseek(f, entry->first_block * CLUSTER_SIZE, SEEK_SET);
        fread(temp_dir_cluster, CLUSTER_SIZE, 1, f);
        
        int not_empty = 0;
        for(int i = 0; i < entradas_por_cluster; i++){
            if(dir_content[i].filename[0] != 0x00){ 
                not_empty = 1;
                break;
            }
        }

        if(not_empty){
            printf("Erro: O diretorio '%s' nao esta vazio.\n", name);
            fclose(f);
            return;
        }
        
        fat[entry->first_block] = FAT_CLUSTER_LIVRE;
        write_fat();
        printf("Diretorio '%s' apagado.\n", name);
    }

    memset(entry, 0, sizeof(dir_entry_t));

    fseek(f, offset_raiz, SEEK_SET);
    fwrite(&data_cluster, CLUSTER_SIZE, 1, f);

    fclose(f);
}
