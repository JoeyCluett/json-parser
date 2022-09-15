#pragma once

/*
Copyright (C) 2022  Joe Cluett
This file is part of json-parser.

json-parser is free software: you can redistribute it and/or modify it under 
the terms of the GNU General Public License as published by the 
Free Software Foundation, either version 3 of the License, or (at your option) any later version.
json-parser is distributed in the hope that it will be useful, but WITHOUT 
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along 
with json-parser. If not, see <https://www.gnu.org/licenses/>.
*/

#include "json-parser-config.h"

typedef enum {
    JsonNodeType_none,

    JsonNodeType_object,
    JsonNodeType_pair,

    JsonNodeType_array,
    JsonNodeType_element, // follow-up for array types

    JsonNodeType_string,
    JsonNodeType_number,

    JsonNodeType_true,
    JsonNodeType_false,
    JsonNodeType_null,
} JsonNodeType_t;

//
// get printable string for node type
//
const char* JsonNodeType_as_string(JsonNodeType_t node_type);

typedef struct JsonNode {
    JsonNodeType_t type;

    struct JsonNode* parent; // not used by all node types

    union {
        struct {
            struct JsonNode* first;
            struct JsonNode* last;
        } obj;

        struct {
            unsigned int key_start;
            unsigned int key_end;
            struct JsonNode* value;
            struct JsonNode* next;
        } pair;

        struct {
            unsigned int start;
            unsigned int end;
        } str;

        struct {
            unsigned int start;
            unsigned int end;
        } num;

        struct {
            struct JsonNode* first;
            //struct JsonNode* next;
            struct JsonNode* last;
        } arr;

        struct {
            struct JsonNode* item;
            struct JsonNode* next;
        } elem;
    };

} JsonNode_t;

typedef JsonNode_t*(*JsonParser_alloc_callback)(void* const alloc_data);
typedef void(*JsonParser_dealloc_callback)(void* const alloc_data, JsonNode_t* node);

typedef struct {
    JsonNode_t* first;
    JsonParser_alloc_callback allocate_node_cb;
    JsonParser_dealloc_callback dealloc_node_cb;
    void* alloc_data;
} JsonDocument_t;

#ifdef JSONPARSER_HAS_MALLOC

//
// default callback for deleting nodes
//
void JsonParser_default_delete_node(void* const alloc_data, JsonNode_t* node);

//
// default callback for allocating nodes
//
JsonNode_t* JsonParser_default_allocate_node(void* const alloc_data);

#endif

//
// initialize documents
//
void JsonParser_init_document(
        JsonDocument_t* doc,
        JsonParser_dealloc_callback delete_node_cb,
        JsonParser_alloc_callback allocate_node_cb,
        void* alloc_data);

//
// destroy nodes used for JSON parsing
//
void JsonParser_delete_document(JsonDocument_t* doc);

typedef enum {
    JsonParseCode_success = 0,

    JsonParseCode_malformed_source,
    JsonParseCode_malformed_object,
    JsonParseCode_malformed_array,
    JsonParseCode_malformed_string,

    JsonParseCode_number_invalid_char,

    JsonParseCode_invalid_array_ending,
    JsonParseCode_invalid_object_ending,

    JsonParseCode_stack_error,
    JsonParseCode_allocation_failure,
    JsonParseCode_unknown_internal_error,

    JsonParseCode_empty_source,
} JsonParseCode_t;

//
// get printable string for parse code
//
const char* JsonParseCode_as_string(JsonParseCode_t c);

//
// meat of the library
//
JsonParseCode_t JsonParser_parse_document(JsonDocument_t* doc, const char* str);
