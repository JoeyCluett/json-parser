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

//
// contains utils for
// - iterating over objects/arrays
// - performing type checks/conversions
// - generating formatted JSON strings
// - TODO : converting JSON to other file types
//

#include "json-parser.h"
#include "json-parser-config.h"

#include <stddef.h>

typedef struct JsonString {
    const char* doc_source;
    unsigned int start;
    unsigned int end;
} JsonString_t;

//
// search for top-level field name in given JSON object node
// returns pointer to JSON pair if found, else NULL
// NOTE : this is not the most efficient way to search for a given field
//
JsonNode_t* JsonObj_field_by_name(const char* doc_source, JsonNode_t* obj, const char* fieldname);

//
// search for first field-pair in the given JSON object node
// returns pointer to first JSON pair if found, else NULL
//
JsonNode_t* JsonObj_first_field(JsonNode_t* obj);

//
// simple access wrapper
//
JsonNode_t* JsonPair_field(JsonNode_t* pair);
int JsonPair_key(JsonNode_t* pair, const char* doc_source, JsonString_t* str);

JsonNode_t* JsonArr_index(JsonNode_t* arr, size_t idx);

typedef struct JsonArrIter {
    JsonNode_t* arr;
    JsonNode_t* element;
} JsonArrIter_t;

int JsonArrIter_init(JsonArrIter_t* iter, JsonNode_t* arr);
int JsonArrIter_next(JsonArrIter_t* iter);
JsonNode_t* JsonArrIter_current(JsonArrIter_t* iter);

typedef struct JsonObjIter {
    JsonNode_t* obj;
    JsonNode_t* current;
} JsonObjIter_t;

//
// top-level initialization function for JsonObjIter type
//
int JsonObjIter_init(JsonObjIter_t* iter, JsonNode_t* top);

//
// advances iterator to next field
// returns 1 if field available, else 0
//
int JsonObjIter_next(JsonObjIter_t* iter);

//
// returns the field currently referred to be the iterator
// NULL if no field
//
JsonNode_t* JsonObjIter_current(JsonObjIter_t* iter);

//
// compare the currently referenced field name with the given fieldname
// returns 1 if field name matched else 0
//
int JsonObjIter_field_name_matches(JsonObjIter_t* iter, const char* doc_source, const char* fieldname);

//
// initialize JsonString_t to reference the same data as the orginal JsonNode_t::string type
// returns 1 on success, else 0
//
int JsonString_init(JsonString_t* str, const char* doc_source, JsonNode_t* str_node);

//
// return length (in bytes) of given JsonString_t
// note this function does not consider evaluated escape sequences 
// or redefined utf-8 sequences. it represents an upper limit on the 
// size of the given string
//
unsigned long JsonString_size(JsonString_t* str);

//
// copy the contents of the string out of the original source
// returns the actual (byte) size of the copied out string
//
size_t JsonString_copy(const JsonString_t* str, char* restrict dest);

//
// structure for holding one of the primitive types that 
// the number conversion routines recognize
//
typedef enum {
    JsonNumberType_unsigned = 0,
    JsonNumberType_signed   = 1,
    JsonNumberType_real     = 2,
} JsonNumberType_t;

typedef struct JsonNumber {
    JsonNumberType_t type;
    union {
        unsigned long int u;
        long int          s;
        double            r;
    };
} JsonNumber_t;

//
// converts contents of given JsonNodeType_number node to actual primitive number type
// returns 1 on success, else 0
//
int JsonNumber_convert_from_json_string(const char* doc_source, JsonNode_t* num_node, JsonNumber_t* num);

//
// structure holding a date/time as per ISO-8601
//
typedef struct JsonDateTime {
    int year;
    int month;
    int day;
    int hours;
    int minutes;
    int seconds;
    int milliseconds;
} JsonDateTime_t;

//
// default initialize given JsonDateTime_t structure
//
void JsonDateTime_init_default(JsonDateTime_t* dt);

#define JSONDATETIME_LEN    24 // full ISO-8601
#define JSONDATETIME_LEN_TR 20 // truncated ISO-8601

//
// put the contents of the given JsonDateTime_t into the destination string
// returns 1 on success, else 0.
// will output truncated ISO-8601 (no milliseconds field) if dlen==20 and full time string if dlen==24
// other dlen's return 0 right away
//
int JsonDateTime_to_string(JsonDateTime_t* dt, char* dest, int dlen);

//
// convert given JsonString_t in ISO-8601 to JsonDateTime_t structure
//
int JsonDateTime_from_json_string(JsonDateTime_t* dt, JsonString_t* str);

//
// convert given c-string in ISO-8601 to JsonDateTime_t structure
//
int JsonDateTime_from_cstring(JsonDateTime_t* dt, const char* str);
