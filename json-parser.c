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

#include "json-parser.h"
#include "json-parser-config.h"

#ifdef JSONPARSER_HAS_MALLOC

#include <stdio.h>
#include <stdlib.h>

JsonNode_t* JsonParser_default_allocate_node(void* const _unused) {
    JsonNode_t* node = (JsonNode_t*)malloc(sizeof(JsonNode_t));
    node->type = JsonNodeType_none;
    node->pair.key_start = 0;
    node->pair.key_end   = 0;
    node->pair.next      = NULL;
    node->pair.value     = NULL;
    return node;
}

void JsonParser_default_delete_node(void* const alloc_data, JsonNode_t* node) {
    if(node == NULL) return;
    free(node);
}

#endif // JSONPARSER_HAS_MALLOC

void JsonParser_init_document(
        JsonDocument_t* doc,
        JsonParser_dealloc_callback delete_node_cb,
        JsonParser_alloc_callback allocate_node_cb,
        void* alloc_data) {

    doc->allocate_node_cb = allocate_node_cb;
    doc->dealloc_node_cb  = delete_node_cb;
    doc->first = NULL;
    doc->alloc_data = alloc_data;
}

void JsonParser_delete_document(JsonDocument_t* doc) {

    if(doc->first == NULL) return;

    JsonParser_dealloc_callback delete_node = doc->dealloc_node_cb;
    void* const alloc_data = doc->alloc_data;

    JsonNode_t* node_stack[JSONPARSER_MAX_DEPTH] = {NULL};

    if(doc->first->type == JsonNodeType_array || doc->first->type == JsonNodeType_object)
        node_stack[0] = doc->first;
    else
        return;

    long int stack_pointer = 0;

    while(stack_pointer >= 0) {

        JsonNode_t* top = node_stack[stack_pointer];

        if(top == NULL) {
            stack_pointer--;
            continue;
        }

        switch(top->type) {
        case JsonNodeType_none:
            return;

        case JsonNodeType_object: {
            JsonNode_t* tmp = top->obj.first;
            delete_node(alloc_data, top);
            node_stack[stack_pointer] = tmp;
            break;
        }
        case JsonNodeType_pair: {
            JsonNode_t* value = top->pair.value;
            JsonNode_t* next  = top->pair.next;
            delete_node(alloc_data, top);
            node_stack[stack_pointer]   = next;
            node_stack[++stack_pointer] = value; // value needs to be deleted first
            break;
        }
        case JsonNodeType_array: {
            JsonNode_t* tmp = top->arr.first;
            delete_node(alloc_data, top);
            node_stack[stack_pointer] = tmp;
            break;
        }
        case JsonNodeType_element: {
            JsonNode_t* item = top->elem.item;
            JsonNode_t* next = top->elem.next;
            delete_node(alloc_data, top);
            node_stack[stack_pointer]   = next;
            node_stack[++stack_pointer] = item;
            break;
        }
        case JsonNodeType_string:
        case JsonNodeType_number:
        case JsonNodeType_true:
        case JsonNodeType_false:
        case JsonNodeType_null:
            delete_node(alloc_data, top);
            stack_pointer--;
            break;
        }
    }
}

const char* JsonParseCode_as_string(JsonParseCode_t c) {
    switch(c) {
    case JsonParseCode_success:                return "success";
    case JsonParseCode_malformed_source:       return "malformed source";
    case JsonParseCode_malformed_object:       return "malformed object";
    case JsonParseCode_malformed_array:        return "malformed array";
    case JsonParseCode_malformed_string:       return "malformed string";
    case JsonParseCode_number_invalid_char:    return "invalid char in number";
    case JsonParseCode_invalid_array_ending:   return "invalid array ending";
    case JsonParseCode_invalid_object_ending:  return "invalid object seperator";
    case JsonParseCode_stack_error:            return "stack error";
    case JsonParseCode_allocation_failure:     return "allocation failure";
    case JsonParseCode_unknown_internal_error: return "unknown internal error";
    case JsonParseCode_empty_source:           return "empty source";
    default: return "UNKNOWN";
    }
}

const char* JsonNodeType_as_string(JsonNodeType_t node_type) {
    switch(node_type) {
    case JsonNodeType_none:    return "none";
    case JsonNodeType_object:  return "object";
    case JsonNodeType_pair:    return "pair";
    case JsonNodeType_array:   return "array";
    case JsonNodeType_element: return "element";
    case JsonNodeType_string:  return "string";
    case JsonNodeType_number:  return "number";
    case JsonNodeType_true:    return "true";
    case JsonNodeType_false:   return "false";
    case JsonNodeType_null:    return "null";
    default: return "UNKNOWN";
    }
}

static inline const int JsonParser_is_whitespace(const char c) {

    static const char table[32] = {
        0x00, 0x3E, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    return (int)(table[(c >> 3) & 0x1F] & (1 << (c & 0x07)));
}

static inline void JsonParser_array_add_element(JsonNode_t* arr, JsonNode_t* elem_node) {
    if(arr->arr.first == NULL) arr->arr.first = elem_node;
    else                       arr->arr.last->elem.next = elem_node;
    arr->arr.last = elem_node;
}

//
// seeks next non-whitespace character
//
static inline const char* JsonParser_seek(const char* str) {
    char c = *str;
    while(c) {
        if(JsonParser_is_whitespace(c)) {
            str++;
            c = *str;
            continue;
        } else {
            return str;
        }
    }
    return NULL;
}

static inline const char* JsonParser_consume_string(const char* str) {
    // assume str currently points at '"'
    
    str++;
    char c = *str;
    while(c) {
        if(c == '\\') {
            str += 2; // no error check
        } else if(c == '"') {
            return str;
        } else {
            str++;
        }
        c = *str;
    }
    return NULL;
}

static inline const int JsonParser_is_numeric(const char c) {
    return (c >= '0' && c <= '9');
}

static inline JsonNode_t* JsonParser_init_element_node(JsonNode_t* node, JsonNode_t* parent) {
    if(node == NULL) return NULL;
    node->type = JsonNodeType_element;
    node->parent = parent;
    return node;
}

static inline JsonNode_t* JsonParser_init_string_node(JsonNode_t* node, JsonNode_t* parent) {
    if(node == NULL) return NULL;
    node->type = JsonNodeType_string;
    node->parent = parent;
    return node;
}

static inline JsonNode_t* JsonParser_init_number_node(JsonNode_t* node, JsonNode_t* parent) {
    if(node == NULL) return NULL;
    node->type = JsonNodeType_number;
    node->parent = parent;
    return node;
}

static inline JsonNode_t* JsonParser_init_tfn_node(JsonNode_t* node, JsonNodeType_t type, JsonNode_t* parent) {
    if(node == NULL) return NULL;
    node->type = type;
    node->parent = parent;
    return node;
}

static inline const int JsonParser_valid_end_of_number(const char c) {
    return 
            JsonParser_is_whitespace(c) || 
            c == ',' || c == '}' || c == ']';
}

static const char* JsonParser_consume_number(const char* str, JsonParseCode_t* code) {

#define state_number_whole        0
#define state_number_fraction     1
#define state_number_exp_sign     2
#define state_number_exp_num      3

    int state_current;

    if(str[0] == '-' || JsonParser_is_numeric(str[0])) {
        str = (str[0] == '-') ? str + 1 : str;
        const char c = str[0];
        if(c == '0') {
            const char sec = str[1];
            if(sec == 'e' || sec == 'E') { // exponent sign part
                state_current = state_number_exp_sign;
                str += 2;
            } else if(sec == '.') { // fractional part
                state_current = state_number_fraction;
                str += 2;
            } else if(JsonParser_valid_end_of_number(sec)) {
                return str + 1;
            } else {
                *code = JsonParseCode_number_invalid_char;
                return NULL;
            }
        } else if(JsonParser_is_numeric(c)) {
            state_current = state_number_whole;
            str++;
        } else {
            *code = JsonParseCode_number_invalid_char;
            return NULL;
        }
    } else {
        *code = JsonParseCode_number_invalid_char;
        return NULL;
    }

    while(*str) {
        const char c = *str;

        if(
                state_current != state_number_exp_sign &&
                JsonParser_valid_end_of_number(c)) {
            return str;
        }

        switch(state_current) {
        case state_number_whole:
            if(JsonParser_is_numeric(c)) { ;
            } else if(c == '.') { // fractional part
                state_current = state_number_fraction;
            } else if(c == 'e' || c == 'E') { // exponent sign part
                state_current = state_number_exp_sign;
            } else {
                *code = JsonParseCode_number_invalid_char;
                return NULL;
            }
            str++;
            break;

        case state_number_fraction:
            if(JsonParser_is_numeric(c)) { ;
            } else if(c == 'e' || c == 'E') { // exponent sign part
                state_current = state_number_exp_sign;
            } else {
                *code = JsonParseCode_number_invalid_char;
                return NULL;
            }
            str++;
            break;

        case state_number_exp_sign:
            if(c == '-' || c == '+' || JsonParser_is_numeric(c)) {
                state_current = state_number_exp_num;
                str = (c == '-' || c == '+') ? str + 1 : str;
                break;
            } else {
                *code = JsonParseCode_number_invalid_char;
                return NULL;
            }

        case state_number_exp_num:
            if(JsonParser_is_numeric(c)) {
                str++;
                break;
            } else {
                *code = JsonParseCode_number_invalid_char;
                return NULL;
            }
        }
    }

    return NULL;
}

static inline const char* JsonParser_is_tfn(const char* str, JsonNodeType_t* nodetype) {

    if(str[0] == 't' && str[1] == 'r' && str[2] == 'u' && str[3] == 'e') {
        *nodetype = JsonNodeType_true;
        return str + 4;
    }
    else if(str[0] == 'f' && str[1] == 'a' && str[2] == 'l' && str[3] == 's' && str[4] == 'e') {
        *nodetype = JsonNodeType_false;
        return str + 5;
    }
    else if(str[0] == 'n' && str[1] == 'u' && str[2] == 'l' && str[3] == 'l') {
        *nodetype = JsonNodeType_null;
        return str + 4;
    }

    return NULL;
}

JsonParseCode_t JsonParser_parse_document(JsonDocument_t* doc, const char* str) {
    const char* const start_str = str;

    JsonNode_t* (*json_allocate_node)(void*) = doc->allocate_node_cb;
    void* const alloc_data = doc->alloc_data;

    JsonNode_t* top = NULL;

    const char* first_char = JsonParser_seek(str);
    if(first_char == NULL) return JsonParseCode_empty_source;

#define state_array     0
#define state_array_sep 1
#define state_key       2
#define state_value     3
#define state_pair_sep  4

    int state_stack[JSONPARSER_MAX_DEPTH+1];
    int* const state_stack_max = state_stack + JSONPARSER_MAX_DEPTH;
    int* state_pointer = state_stack; // points to current state

    if('[' == *first_char) {
        JsonNode_t* nodeptr = json_allocate_node(alloc_data);
        nodeptr->type       = JsonNodeType_array;
        nodeptr->parent     = top;
        top = nodeptr;
        state_stack[0] = state_array;
    }
    else if('{' == *first_char) {
        JsonNode_t* nodeptr = json_allocate_node(alloc_data);;
        nodeptr->type       = JsonNodeType_object;
        nodeptr->parent     = top;
        top = nodeptr;
        state_stack[0] = state_key;
    }
    else {
        return JsonParseCode_malformed_source;
    }

    doc->first = top;

    str = first_char + 1; // advance to next character and now start the actual parsing phase

    while(*str && top != NULL && state_pointer >= state_stack) {
        str = JsonParser_seek(str);
        if(*str == '\0') {
            if(state_pointer < state_stack)
                return JsonParseCode_success;
            return JsonParseCode_malformed_source;
        }

        const int state_current = *state_pointer;
        const char c = *str;

        switch(state_current) {
        case state_array:
        {
            if(c == '"') { // string
                const char* const str_end = JsonParser_consume_string(str);
                if(str_end == NULL) return JsonParseCode_malformed_string;

                JsonNode_t* elem_node = JsonParser_init_element_node(json_allocate_node(alloc_data), top);
                JsonNode_t* str_node  = JsonParser_init_string_node(json_allocate_node(alloc_data), top);
                if(elem_node == NULL || str_node == NULL) return JsonParseCode_allocation_failure;

                elem_node->elem.item = str_node;
                JsonParser_array_add_element(top, elem_node);

                str_node->str.start = 1u + (unsigned int)(str - start_str);
                str_node->str.end   = (unsigned int)(str_end - start_str);

                *(++state_pointer) = state_array_sep;
                str = str_end + 1; // advance past closing quote
                break;
            } else if(c == '-' || JsonParser_is_numeric(c)) { // number
                JsonParseCode_t code;
                const char* num_end = JsonParser_consume_number(str, &code);
                if(num_end == NULL) return code;

                JsonNode_t* elem_node = JsonParser_init_element_node(json_allocate_node(alloc_data), top);
                JsonNode_t* num_node  = JsonParser_init_number_node(json_allocate_node(alloc_data), top);
                if(elem_node == NULL || num_node == NULL) return JsonParseCode_allocation_failure;

                elem_node->elem.item = num_node;
                JsonParser_array_add_element(top, elem_node);

                num_node->num.start = (unsigned int)(str - start_str);
                num_node->num.end   = (unsigned int)(num_end - start_str);
                
                *(++state_pointer) = state_array_sep;
                str = num_end;
                break;
            } else if(c == '[' || c == '{') { // new object or array
                JsonNode_t* elem_node = JsonParser_init_element_node(json_allocate_node(alloc_data), top);
                JsonNode_t* nested    = json_allocate_node(alloc_data);
                if(elem_node == NULL || nested == NULL) return JsonParseCode_allocation_failure;

                elem_node->elem.item = nested;
                JsonParser_array_add_element(top, elem_node);

                nested->parent = top;
                nested->type   = (c == '[') ? JsonNodeType_array : JsonNodeType_object;
                top = nested;

                *(++state_pointer) = state_array_sep;
                *(++state_pointer) = (c == '[') ? state_array : state_key;
                str++;
                break;
            } else if(c == ']') {
                // should only happen if array is empty
                top = top->parent;
                state_pointer--;
                str++;
                break;
            } else {
                JsonNodeType_t type;
                const char* tfn_return = JsonParser_is_tfn(str, &type);
                if(tfn_return) {
                    JsonNode_t* elem_node = JsonParser_init_element_node(json_allocate_node(alloc_data), top);
                    JsonNode_t* tfn_node  = JsonParser_init_tfn_node(json_allocate_node(alloc_data), type, top);
                    if(elem_node == NULL || tfn_node == NULL) return JsonParseCode_allocation_failure;

                    elem_node->elem.item = tfn_node;
                    JsonParser_array_add_element(top, elem_node);

                    *(++state_pointer) = state_array_sep;
                    str = tfn_return;
                    break;
                } else {
                    return JsonParseCode_malformed_array;
                }
            }
        }
        case state_array_sep:
            if(c == ']') { // normal array ending
                top = top->parent;
                state_pointer--; // state is now state_array
                state_pointer--; // whatever state came before
                str++;
                break;
            } else if(c == ',') {
                const char* next = JsonParser_seek(str + 1);
                if(*next != ']') {    // there is another item in the array
                    state_pointer--; // get rid of array_sep state
                    str = next;      // start seeking at whatever this char is
                    break;
                } else { // non-compliant array ending
#ifdef JSONPARSER_NOT_STRICT
                    top = top->parent;
                    state_pointer--; // state is now state_array
                    state_pointer--; // whatever state came before
                    str = next + 1;
                    break;
#else
                    return JsonParseCode_invalid_array_ending;
#endif // JSONPARSER_NOT_STRICT
                }
            } else {
                return JsonParseCode_malformed_array;
            }

        case state_key:
            if(c == '"') {
                const char* key_end = JsonParser_consume_string(str);
                if(key_end == NULL) return JsonParseCode_malformed_object;

                JsonNode_t* pair_node = json_allocate_node(alloc_data);
                if(pair_node == NULL) return JsonParseCode_allocation_failure;

                pair_node->type = JsonNodeType_pair;
                pair_node->pair.key_start = 1u + (unsigned int)(str - start_str);
                pair_node->pair.key_end   = (unsigned int)(key_end - start_str);
                pair_node->parent = top;

                if(top->obj.first == NULL) top->obj.first = pair_node;
                else                       top->obj.last->pair.next = pair_node;
                top->obj.last = pair_node;

                str = key_end + 1;
                const char* colon = JsonParser_seek(str);
                if(*colon != ':') return JsonParseCode_malformed_object;

                str = colon + 1;
                *state_pointer = state_value;
                top = pair_node;
                break;
            } else if(c == '}') {
#ifdef JSONPARSER_NOT_STRICT
                str++;
                state_pointer--;
                top = top->parent;
                break;
#else
                return JsonParseCode_malformed_object;
#endif // JSONPARSER_NOT_STRICT
            }

        case state_value:
            if(c == '"') {
                const char* str_end = JsonParser_consume_string(str);
                if(str_end == NULL) return JsonParseCode_malformed_string;
                JsonNode_t* str_node  = JsonParser_init_string_node(json_allocate_node(alloc_data), top);
                if(str_node == NULL) return JsonParseCode_allocation_failure;

                str_node->str.start = 1u + (unsigned int)(str - start_str);
                str_node->str.end   = (unsigned int)(str_end - start_str);

                top->pair.value = str_node;
                *state_pointer = state_pair_sep;
                str = str_end + 1; // advance past closing quote
                break;
            } else if(c == '-' || JsonParser_is_numeric(c)) {
                JsonParseCode_t code;
                const char* num_end = JsonParser_consume_number(str, &code);
                if(num_end == NULL) return code;
                JsonNode_t* num_node  = JsonParser_init_number_node(json_allocate_node(alloc_data), top);
                if(num_node == NULL) return JsonParseCode_allocation_failure;

                num_node->num.start = (unsigned int)(str - start_str);
                num_node->num.end   = (unsigned int)(num_end - start_str);
                
                top->pair.value = num_node;
                *state_pointer = state_pair_sep;
                str = num_end;
                break;
            } else if(c == '{' || c == '[') {
                JsonNode_t* nested_node = json_allocate_node(alloc_data);
                if(nested_node == NULL) return JsonParseCode_allocation_failure;
                nested_node->type = (c == '[') ? JsonNodeType_array : JsonNodeType_object;
                nested_node->parent = top;
                top->pair.value = nested_node;
                top = nested_node;

                *state_pointer = state_pair_sep;
                *(++state_pointer) = (c == '[') ? state_array : state_key;
                str++;
                break;
            } else {
                JsonNodeType_t type;
                const char* tfn_ptr = JsonParser_is_tfn(str, &type);
                if(tfn_ptr == NULL) return JsonParseCode_malformed_object;
                JsonNode_t* tfn_node = JsonParser_init_tfn_node(json_allocate_node(alloc_data), type, top);
                if(tfn_node == NULL) return JsonParseCode_allocation_failure;

                top->pair.value = tfn_node;
                str = tfn_ptr;
                *state_pointer = state_pair_sep;
                break;
            }

        case state_pair_sep:
            if(c == '}') { // normal end of object
                top = top->parent->parent; // have to get past the pair_node and the object_node
                state_pointer--;
                str++;
                break;
            } else if(c == ',') {
                const char* next = JsonParser_seek(str + 1);
                if(*next != '}') {
                    top = top->parent; // get past the pair_node, but keep the object_node
                    *state_pointer = state_key;
                    str++;
                    break;
                } else {
#ifdef JSONPARSER_NOT_STRICT
                    top = top->parent->parent; // move past pair_node and object_node
                    str = next + 1;
                    state_pointer--;
                    break;                    
#else
                    return JsonParseCode_invalid_object_ending;
#endif // JSONPARSER_NOT_STRICT
                }
            } else {
                return JsonParseCode_malformed_object;
            }

        default:
            return JsonParseCode_unknown_internal_error;
        }

        if(state_pointer >= state_stack_max) return JsonParseCode_stack_error;
    }

    return JsonParseCode_success;
}

