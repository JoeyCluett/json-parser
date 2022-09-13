#include "json-parser.h"
#include "json-parser-util.h"

#include <sys/time.h>

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

extern char *strdup(const char *s);

#define USE_CUSTOM_ALLOC

#define CHUNK_SIZE 4096

typedef struct node_chunk {
    JsonNode_t* current_node;
    JsonNode_t* max_node;
    struct node_chunk* next;
    JsonNode_t nodes[CHUNK_SIZE];
} node_chunk_t;

typedef struct {
    node_chunk_t* first;
    node_chunk_t* last;
} bump_allocator_t;

size_t node_count    = 0ul;
size_t dealloc_count = 0ul;
size_t chunk_count   = 0ul;

node_chunk_t* new_node_chunk(void) {

    chunk_count++;    

    node_chunk_t* chunkptr = (node_chunk_t*)malloc(sizeof(node_chunk_t));
    chunkptr->next = NULL;
    chunkptr->current_node = chunkptr->nodes;
    chunkptr->max_node = chunkptr->nodes + CHUNK_SIZE;
    return chunkptr;
}

JsonNode_t* alloc_custom(void* const alloc_data);

void dealloc_custom(void* const alloc_data, JsonNode_t* node) {
    //printf("dealloc called\n");
    //fflush(stdout);

    if(node == NULL)
        return;

    dealloc_count++;
    //printf("dealloc count : %lu\n", dealloc_count);
}

size_t timestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000ul + tv.tv_usec;
}

int main(int argc, char** argv) {

    if(argc != 2) {
        printf("usage:\n    ./main <JSON file>\n\n");
        fflush(stdout);
        exit(EXIT_SUCCESS);
    }

    bump_allocator_t bump;

    JsonDocument_t doc;

    JsonParser_init_document(&doc,
            dealloc_custom,
            alloc_custom, &bump);

    char* json;

    {
        FILE* fptr = fopen(argv[1], "rb");
        if(fptr == NULL)
            return 1;

        fseek(fptr, 0L, SEEK_END);
        size_t l_size = (size_t)ftell(fptr);

        printf("file size : %lu bytes\n", l_size);

        rewind(fptr);
        json = (char*)malloc(l_size + 1);

        size_t rd_size = 0ul;

        while(l_size) {
            size_t chunk_size = fread(json + rd_size, 1, l_size, fptr);
            rd_size += chunk_size;
            l_size  -= chunk_size;
        }

        fclose(fptr);
        json[rd_size] = '\0';
    }

    size_t num_iters = 1;
    size_t total_time = 0ul;

    bump.first = new_node_chunk();
    bump.last = bump.first;

    JsonParseCode_t code = JsonParser_parse_document(
        &doc,
        json
    );

    JsonObjIter_t iter;
    if(!JsonObjIter_init(&iter, doc.first)) {
        printf("top-level entity is not a JSON object\n");
        exit(1);
    }

    JsonNode_t* node = JsonObj_field_by_name(json, doc.first, "dataStoreUrl");
    if(node == NULL) {
        printf("field name 'dataStoreUrl' not found in JSON document\n");
        exit(1);
    }

    JsonNode_t* value = node->pair.value;
    if(value->type != JsonNodeType_string) {
        printf("value is not string type\n");
        exit(1);
    }

    char buf[128];

    JsonString_t str;
    JsonString_init(&str, json, value);
    unsigned long strsz = JsonString_size(&str);
    buf[strsz] = '\0';
    JsonString_copy(&str, buf);

    printf("value : %s\n", buf);
    printf("input : jdbc:microsoft:sqlserver://LOCALHOST:1433;DatabaseName=goon\n");

    JsonParser_delete_document(&doc);

    // remove blocks belonging to bump allocator
    node_chunk_t* chunk_iter = bump.first;
    while(chunk_iter) {
        node_chunk_t* tmp = chunk_iter->next;
        free(chunk_iter);
        chunk_iter = tmp;
    }

    // remove space allocated for source
    free(json);

    return 0;
}

JsonNode_t* alloc_custom(void* const alloc_data) {
    bump_allocator_t* const bump = (bump_allocator_t* const)alloc_data;
    node_chunk_t* nodes = bump->last;

    node_count++;

    if(nodes->current_node == nodes->max_node) {
        bump->last->next = new_node_chunk();
        bump->last = bump->last->next;
        nodes = bump->last;
    }

    JsonNode_t* node = nodes->current_node++;
    node->type = JsonNodeType_none;
    node->pair.key_start = 0;
    node->pair.key_end   = 0;
    node->pair.next      = NULL;
    node->pair.value     = NULL;
    return node;
}



