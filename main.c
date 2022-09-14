#include "json-parser.h"
#include "json-parser-util.h"

#include <sys/time.h>

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

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
    if(node == NULL)
        return;

    dealloc_count++;
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

    if(doc.first->type != JsonNodeType_array) {
        printf("top-level entry is not an array type\n");
        exit(1);
    }

    JsonArrIter_t arr;
    if(!JsonArrIter_init(&arr, doc.first)) {
        printf("error initializing array iterator\n");
        exit(1);
    }

    printf("for every entry in top-level array, print payload/commits[0]/message if it exists\n\n");

    JsonNode_t* entry = JsonArrIter_current(&arr);
    for(; entry != NULL; JsonArrIter_next(&arr), entry = JsonArrIter_current(&arr)) {

        // print every payload/commits[0]/message

        JsonNode_t* p = JsonObj_field_by_name(json, entry, "payload");
        if(p == NULL) {
            continue;
        }

        JsonNode_t* c = JsonObj_field_by_name(json, JsonObj_field_value(p), "commits");
        if(c == NULL) {
            continue;
        }

        JsonNode_t* c0 = JsonArr_index(JsonObj_field_value(c), 0);
        if(c0 == NULL) {
            continue;
        }

        JsonNode_t* m = JsonObj_field_by_name(json, c0, "message");
        if(m == NULL) {
            continue;
        }

        JsonString_t str;
        JsonString_init(&str, json, JsonObj_field_value(m));

        size_t strsz = JsonString_size(&str);
        char buf[strsz + 1];
        buf[strsz] = '\0';
        JsonString_copy(&str, buf);

        printf("message : %s\n", buf);
    }

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



