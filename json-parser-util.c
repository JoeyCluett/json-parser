#include "json-parser-util.h"
#include "json-parser.h"

#include <stddef.h>

static int JsonAPI_string_cmp(const char* start, const char* end, const char* nt_cstr);

int JsonString_init(JsonString_t* str, const char* doc_source, JsonNode_t* str_node) {

    if(str == NULL || doc_source == NULL || str_node->type != JsonNodeType_string)
        return 0;

    str->doc_source = doc_source;
    str->start      = str_node->str.start;
    str->end        = str_node->str.end;
    return 1;
}

void JsonString_copy(const JsonString_t* str, char* restrict dest) {

    const char* start = str->doc_source + str->start;
    const char* end   = str->doc_source + str->end;

    while(start != end) {
        dest[0] = start[0];
        dest++;
        start++;
    }
}

unsigned long JsonString_size(JsonString_t* str) {
    return (unsigned long)(str->end - str->start);
}

int JsonObjIter_field_name_matches(JsonObjIter_t* iter, const char* doc_source, const char* fieldname) {
    JsonNode_t* pair = JsonObjIter_current(iter);

    if(pair == NULL)
        return 0;

    return JsonAPI_string_cmp(doc_source + pair->pair.key_start, doc_source + pair->pair.key_end, fieldname);
}

JsonNode_t* JsonObj_field_by_name(const char* doc_source, JsonNode_t* obj, const char* fieldname) {
    JsonObjIter_t iter;
    if(!JsonObjIter_init(&iter, obj))
        return NULL;

    while(iter.current != NULL) {
        JsonNode_t* cur = iter.current;
        if(JsonAPI_string_cmp(doc_source + cur->pair.key_start, doc_source + cur->pair.key_end, fieldname))
            return cur;

        // advance to next field
        (void)JsonObjIter_next(&iter);
    }

    return NULL;
}

int JsonObjIter_init(JsonObjIter_t* iter, JsonNode_t* top) {
    if(top == NULL || top->type != JsonNodeType_object)
        return 0;

    iter->obj = top;
    iter->current = top->obj.first;
    return 1;
}

int JsonObjIter_next(JsonObjIter_t* iter) {
    if(iter->current == NULL)
        return 0;
    iter->current = iter->current->pair.next; // advance to the next pair
    return iter->current ? 1 : 0;
}

JsonNode_t* JsonObjIter_current(JsonObjIter_t* iter) {
    return iter->current;
}

static int JsonAPI_string_cmp(const char* start, const char* end, const char* nt_cstr) {

    char c = *nt_cstr;

    while(start != end && c) {

        if(*start != c)
            return 0;

        start++;
        nt_cstr++;
        c = *nt_cstr;
    }

    return (start == end && c == '\0');
}
