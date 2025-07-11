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
    for (int i = 0; i < DATA_CLUSTERS; i++) {
        fwrite(empty_cluster, CLUSTER_SIZE, 1, f);
    }
    fclose(f);
    printf("Sistema de arquivos FAT inicializado com sucesso.\n");
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

void ls(const char* path){
    if(!sistema_carregado){
        printf("Sistema de arquivos não carregado. Execute 'load' primeiro.\n");
        return;
    }
    find_result_t result = find_entry_by_path(path);
    if(!result.found){
        printf("Erro: Caminho '%s' não encontrado.\n", path);
        return;
    }

    // Se for um arquivo, apenas exibe suas informações
    if(result.entry.attributes == 0) {
        printf("Attr| Tamanho | Nome\n");
        printf("----|---------|----\n");
        printf(" %c   | %-7u | %s\n",
                'A',
                result.entry.size,
                result.entry.filename);
        return;
    }

    // Se for um diretório, lista seu conteúdo
    uint16_t cluster_idx = result.entry.first_block;
    data_cluster_t dir_content;
    FILE *f = fopen("fat.part", "rb");
    if(f == NULL){
        perror("Erro ao abrir o arquivo fat.part");
        return;
    }
    fseek(f, cluster_idx * CLUSTER_SIZE, SEEK_SET);
    fread(&dir_content, CLUSTER_SIZE, 1, f);
    fclose(f);

    printf("Conteúdo do diretório '%s':\n", path);
    printf("Attr| Tamanho | Nome\n");
    printf("----|---------|----\n");
    int entradas_por_cluster = CLUSTER_SIZE / sizeof(dir_entry_t);
    for(int i = 0; i < entradas_por_cluster; i++){
        if(dir_content.dir[i].filename[0] != 0x00){
            printf(" %c   | %-7u | %s\n",
                (dir_content.dir[i].attributes == 1) ? 'D' : 'A',
                dir_content.dir[i].size,
                dir_content.dir[i].filename);
        }
    }
}

void create(char *full_path){
    if(!sistema_carregado){ /*...*/ return; }

    char parent_path[256];
    char new_filename[18];
    separate_path(full_path, parent_path, new_filename);

    if(strlen(new_filename) > 17){
        printf("Nome de arquivo muito longo! Máximo 17 caracteres.\n");
        return;
    }

    find_result_t parent_dir_info = find_entry_by_path(parent_path);
    if (!parent_dir_info.found || parent_dir_info.entry.attributes != 1) {
        printf("Erro: O caminho '%s' não é um diretório válido.\n", parent_path);
        return;
    }

    FILE *f = fopen("fat.part", "r+b");
    uint16_t parent_cluster_idx = parent_dir_info.entry.first_block;
    fseek(f, parent_cluster_idx * CLUSTER_SIZE, SEEK_SET);
    fread(&data_cluster, CLUSTER_SIZE, 1, f);

    if (find_entry_idx_by_name(new_filename, &data_cluster) != -1) {
        printf("Erro: Arquivo ou diretório '%s' já existe neste caminho.\n", new_filename);
        fclose(f);
        return;
    }

    int free_entry_idx = find_free_entry_idx(&data_cluster);
    if (free_entry_idx == -1) {
        printf("Erro: Não há espaço livre no diretório '%s'.\n", parent_path);
        fclose(f);
        return;
    }

    dir_entry_t *nova_entrada = &data_cluster.dir[free_entry_idx];
    strcpy((char*)nova_entrada->filename, new_filename);
    nova_entrada->attributes = 0;
    nova_entrada->size = 0;
    nova_entrada->first_block = 0;

    fseek(f, parent_cluster_idx * CLUSTER_SIZE, SEEK_SET);
    fwrite(&data_cluster, CLUSTER_SIZE, 1, f);
    fclose(f);
    printf("Arquivo '%s' criado com sucesso.\n", full_path);
}

void mkdir(char *full_path){
    if(!sistema_carregado){
        printf("O sistema de arquivos não foi carregado. Execute 'load'.\n");
        return;
    }

    char parent_path[256];
    char new_dirname[18];
    separate_path(full_path, parent_path, new_dirname);

    if(strlen(new_dirname) > 17){
        printf("Nome de diretório muito longo! Máximo 17 caracteres.\n");
        return;
    }

    // Encontra o diretório pai
    find_result_t parent_dir_info = find_entry_by_path(parent_path);
    if (!parent_dir_info.found || parent_dir_info.entry.attributes != 1) {
        printf("Erro: O caminho '%s' não é um diretório válido.\n", parent_path);
        return;
    }

    FILE *f = fopen("fat.part", "r+b");
    if(f == NULL){
        perror("Erro ao abrir o arquivo fat.part");
        return;
    }

    // Carrega os dados do diretório pai
    uint16_t parent_cluster_idx = parent_dir_info.entry.first_block;
    fseek(f, parent_cluster_idx * CLUSTER_SIZE, SEEK_SET);
    fread(&data_cluster, CLUSTER_SIZE, 1, f);

    // Verifica se já existe uma entrada com o mesmo nome no diretório pai
    if (find_entry_idx_by_name(new_dirname, &data_cluster) != -1) {
        printf("Erro: Diretório '%s' já existe neste caminho.\n", new_dirname);
        fclose(f);
        return;
    }

    // Encontra uma entrada livre no diretório pai
    int free_entry_idx = find_free_entry_idx(&data_cluster);
    if (free_entry_idx == -1) {
        printf("Erro: Não há espaço livre no diretório '%s'.\n", parent_path);
        fclose(f);
        return;
    }
    
    // Encontra um cluster de dados livre para o novo diretório
    int free_cluster_idx = find_free_cluster();
    if(free_cluster_idx == -1){
        printf("Erro: Não há clusters livres disponíveis.\n");
        fclose(f);
        return;
    }

    // Atualiza a FAT e salva no disco
    fat[free_cluster_idx] = FAT_EOF;
    write_fat(f);

    // Preenche a entrada de diretório no pai
    dir_entry_t *nova_entrada = &data_cluster.dir[free_entry_idx];
    strcpy((char*)nova_entrada->filename, new_dirname);
    nova_entrada->attributes = 1; // Atributo 1 para DIRETÓRIO
    nova_entrada->size = 0;
    nova_entrada->first_block = (uint16_t)free_cluster_idx;
    
    // Salva o diretório pai modificado
    fseek(f, parent_cluster_idx * CLUSTER_SIZE, SEEK_SET);
    fwrite(&data_cluster, CLUSTER_SIZE, 1, f);

    // Limpa o cluster do novo diretório (inicializa com zeros)
    uint8_t empty_cluster[CLUSTER_SIZE];
    memset(empty_cluster, 0x00, CLUSTER_SIZE);
    fseek(f, free_cluster_idx * CLUSTER_SIZE, SEEK_SET);
    fwrite(empty_cluster, CLUSTER_SIZE, 1, f);
    
    fclose(f);
    printf("Diretório '%s' criado com sucesso.\n", full_path);
}

void unlink(char *full_path){
    if(!sistema_carregado){
        printf("Sistema de arquivos não carregado. Execute 'load' primeiro.\n");
        return;
    }

    // Usa nossa função mestra para encontrar o alvo
    find_result_t result = find_entry_by_path(full_path);

    if(!result.found){
        printf("Erro: Arquivo ou diretorio '%s' nao encontrado.\n", full_path);
        return;
    }

    FILE *f = fopen("fat.part", "r+b");
    if(f == NULL){
        perror("Erro ao abrir o arquivo fat.part");
        return;
    }
    
    // Carrega o diretório PAI do alvo
    fseek(f, result.parent_cluster_idx * CLUSTER_SIZE, SEEK_SET);
    fread(&data_cluster, CLUSTER_SIZE, 1, f);

    // Pega um ponteiro para a entrada que será apagada
    dir_entry_t *entry = &data_cluster.dir[result.entry_idx_in_parent];
    
    if(entry->attributes == 0){ // É um arquivo
        uint16_t current_cluster = entry->first_block;
        while(current_cluster != 0 && current_cluster != FAT_EOF){
            uint16_t next_cluster = fat[current_cluster];
            fat[current_cluster] = FAT_CLUSTER_LIVRE;
            current_cluster = next_cluster;
        }
        write_fat(f); 
        printf("Arquivo '%s' apagado.\n", full_path);

    } else { // É um diretório
        data_cluster_t dir_content;
        fseek(f, entry->first_block * CLUSTER_SIZE, SEEK_SET);
        fread(&dir_content, CLUSTER_SIZE, 1, f);
        int not_empty = 0;
        int entradas_por_cluster = CLUSTER_SIZE / sizeof(dir_entry_t);
        for(int i = 0; i < entradas_por_cluster; i++){
            if(dir_content.dir[i].filename[0] != 0x00){ 
                not_empty = 1;
                break;
            }
        }
        if(not_empty){
            printf("Erro: O diretorio '%s' nao esta vazio.\n", full_path);
            fclose(f);
            return;
        }
        fat[entry->first_block] = FAT_CLUSTER_LIVRE;
        write_fat(f);
        printf("Diretorio '%s' apagado.\n", full_path);
    }

    // Zera a entrada no diretório pai e a salva no disco
    memset(entry, 0, sizeof(dir_entry_t));
    fseek(f, result.parent_cluster_idx * CLUSTER_SIZE, SEEK_SET);
    fwrite(&data_cluster, CLUSTER_SIZE, 1, f);

    fclose(f);
}


void write(char *full_path, char *text){
    if(!sistema_carregado){
        printf("Sistema de arquivos não carregado. Execute 'load' primeiro.\n");
        return;
    }
    
    // Usa a função mestra para encontrar o arquivo
    find_result_t result = find_entry_by_path(full_path);

    if(!result.found){
        printf("Erro: Arquivo '%s' não encontrado.\n", full_path);
        return;
    }
    if(result.entry.attributes == 1){
        printf("Erro: '%s' é um diretório, não um arquivo.\n", full_path);
        return;
    }

    FILE *f = fopen("fat.part", "r+b");
    if(f == NULL){
        perror("Erro ao abrir o arquivo fat.part");
        return;
    }

    // Carrega o diretório PAI do arquivo
    uint16_t parent_cluster_idx = result.parent_cluster_idx;
    fseek(f, parent_cluster_idx * CLUSTER_SIZE, SEEK_SET);
    fread(&data_cluster, CLUSTER_SIZE, 1, f);

    // Pega o ponteiro para a entrada do arquivo dentro do diretório pai
    dir_entry_t *entry = &data_cluster.dir[result.entry_idx_in_parent];

    // O resto da lógica de 'write' (liberar clusters antigos, alocar novos, etc.) continua idêntica
    uint16_t current_cluster = entry->first_block;
    while(current_cluster != 0 && current_cluster != FAT_EOF){
        uint16_t next_cluster = fat[current_cluster];
        fat[current_cluster] = FAT_CLUSTER_LIVRE;
        current_cluster = next_cluster;
    }
    int text_len = strlen(text);
    if(text_len == 0){
        entry->first_block = 0;
        entry->size = 0;
    }else{
        int clusters_needed = (text_len + CLUSTER_SIZE - 1) / CLUSTER_SIZE;
        int *new_clusters = malloc(clusters_needed * sizeof(int));
        int allocated_count = 0;
        for(int i = 0; i < clusters_needed; i++){
            int free_cluster = find_free_cluster();
            if(free_cluster == -1){
                printf("Erro: Disco cheio, não foi possível escrever.\n");
                for(int j = 0; j < allocated_count; j++){ fat[new_clusters[j]] = FAT_CLUSTER_LIVRE; }
                free(new_clusters);
                fclose(f);
                return;
            }
            new_clusters[i] = free_cluster;
            fat[free_cluster] = FAT_EOF; 
            allocated_count++;
        }
        entry->first_block = new_clusters[0];
        entry->size = text_len;
        for(int i = 0; i < clusters_needed - 1; i++){
            fat[new_clusters[i]] = new_clusters[i+1];
        }
        fat[new_clusters[clusters_needed - 1]] = FAT_EOF;
        char *text_ptr = text;
        int remaining = text_len;
        for(int i = 0; i < clusters_needed; i++){
            fseek(f, new_clusters[i] * CLUSTER_SIZE, SEEK_SET);
            int to_write = (remaining > CLUSTER_SIZE) ? CLUSTER_SIZE : remaining;
            fwrite(text_ptr, 1, to_write, f);
            text_ptr += to_write;
            remaining -= to_write;
        }
        free(new_clusters);
    }

    // Salva as mudanças
    write_fat(f);
    fseek(f, parent_cluster_idx * CLUSTER_SIZE, SEEK_SET);
    fwrite(&data_cluster, CLUSTER_SIZE, 1, f);
    fclose(f);
    printf("Texto escrito no arquivo '%s' com sucesso.\n", full_path);
}

void append(char *full_path, char *text){
    if(!sistema_carregado){
        printf("Sistema de arquivos não carregado. Execute 'load' primeiro.\n");
        return;
    }
    
    // Usa a função mestra para encontrar o arquivo
    find_result_t result = find_entry_by_path(full_path);

    if(!result.found){
        printf("Erro: Arquivo '%s' não encontrado.\n", full_path);
        return;
    }
    if(result.entry.attributes == 1){
        printf("Erro: '%s' é um diretório, não um arquivo.\n", full_path);
        return;
    }

    FILE *f = fopen("fat.part", "r+b");
    if(f == NULL){
        perror("Erro ao abrir o arquivo fat.part");
        return;
    }

    // Carrega o diretório PAI do arquivo
    uint16_t parent_cluster_idx = result.parent_cluster_idx;
    fseek(f, parent_cluster_idx * CLUSTER_SIZE, SEEK_SET);
    fread(&data_cluster, CLUSTER_SIZE, 1, f);

    // Pega o ponteiro para a entrada do arquivo dentro do diretório pai
    dir_entry_t *entry = &data_cluster.dir[result.entry_idx_in_parent];
    
    // O resto da lógica de 'append' continua idêntica
    int text_len = strlen(text);
    if(text_len == 0){
        fclose(f);
        return;
    }
    int bytes_no_ultimo_cluster = entry->size % CLUSTER_SIZE;
    int espaco_livre_no_cluster = 0;
    if (entry->size > 0 && bytes_no_ultimo_cluster > 0) {
        espaco_livre_no_cluster = CLUSTER_SIZE - bytes_no_ultimo_cluster;
    }
    char *text_ptr = text;
    int bytes_restantes_para_escrever = text_len;
    uint16_t last_cluster = entry->first_block;
    if (last_cluster != 0) {
        while (fat[last_cluster] != FAT_EOF) {
            last_cluster = fat[last_cluster];
        }
    }
    if (espaco_livre_no_cluster > 0) {
        int bytes_para_preencher = (bytes_restantes_para_escrever < espaco_livre_no_cluster) ? bytes_restantes_para_escrever : espaco_livre_no_cluster;
        fseek(f, last_cluster * CLUSTER_SIZE + bytes_no_ultimo_cluster, SEEK_SET);
        fwrite(text_ptr, 1, bytes_para_preencher, f);
        text_ptr += bytes_para_preencher;
        bytes_restantes_para_escrever -= bytes_para_preencher;
    }
    if (bytes_restantes_para_escrever > 0) {
        int clusters_needed = (bytes_restantes_para_escrever + CLUSTER_SIZE - 1) / CLUSTER_SIZE;
        int *new_clusters = malloc(clusters_needed * sizeof(int));
        for (int i = 0; i < clusters_needed; i++) {
            new_clusters[i] = find_free_cluster();
            if (new_clusters[i] == -1) { /* ... */ }
            fat[new_clusters[i]] = FAT_EOF;
        }
        if (entry->first_block == 0) {
            entry->first_block = new_clusters[0];
        } else {
            fat[last_cluster] = new_clusters[0];
        }
        for (int i = 0; i < clusters_needed - 1; i++) {
            fat[new_clusters[i]] = new_clusters[i + 1];
        }
        int remaining = bytes_restantes_para_escrever;
        for (int i = 0; i < clusters_needed; i++) {
            fseek(f, new_clusters[i] * CLUSTER_SIZE, SEEK_SET);
            int to_write = (remaining > CLUSTER_SIZE) ? CLUSTER_SIZE : remaining;
            fwrite(text_ptr, 1, to_write, f);
            text_ptr += to_write;
            remaining -= to_write;
        }
        free(new_clusters);
    }
    
    entry->size += text_len;
    write_fat(f);
    fseek(f, parent_cluster_idx * CLUSTER_SIZE, SEEK_SET);
    fwrite(&data_cluster, CLUSTER_SIZE, 1, f);
    fclose(f);
    printf("Texto adicionado ao arquivo '%s' com sucesso.\n", full_path);
}

void read(char *full_path){
    if(!sistema_carregado){
        printf("Sistema de arquivos não carregado. Execute 'load' primeiro.\n");
        return;
    }

    // Usa a nossa função de navegação para encontrar o arquivo
    find_result_t result = find_entry_by_path(full_path);

    if(!result.found){
        printf("Erro: Arquivo '%s' não encontrado.\n", full_path);
        return;
    }

    if(result.entry.attributes == 1){
        printf("Erro: '%s' é um diretório, não é possível lê-lo com este comando.\n", full_path);
        return;
    }
    
    FILE *f = fopen("fat.part", "rb");
    if (f == NULL) {
        perror("Erro ao abrir o arquivo fat.part");
        return;
    }
    
    uint16_t current_cluster = result.entry.first_block;
    int bytes_restantes = result.entry.size;

    if(bytes_restantes == 0){
        fclose(f);
        return;
    }
    
    printf("Conteúdo do arquivo '%s':\n", full_path);
    char cluster_buffer[CLUSTER_SIZE];

    while(bytes_restantes > 0 && current_cluster != FAT_EOF && current_cluster != 0){
        fseek(f, current_cluster * CLUSTER_SIZE, SEEK_SET);
        fread(cluster_buffer, 1, CLUSTER_SIZE, f);

        int bytes_a_imprimir = (bytes_restantes < CLUSTER_SIZE) ? bytes_restantes : CLUSTER_SIZE;
        fwrite(cluster_buffer, 1, bytes_a_imprimir, stdout);

        bytes_restantes -= bytes_a_imprimir;
        current_cluster = fat[current_cluster];
    }
    printf("\n");
    fclose(f);
}