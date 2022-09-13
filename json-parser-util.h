#pragma once

/*
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

//
// search for top-level field name in given JSON object node
// returns pointer to JSON pair if found, else NULL
// NOTE : this is not the most efficient way to search for a given field
//
JsonNode_t* JsonObj_field_by_name(const char* doc_source, JsonNode_t* obj, const char* fieldname);

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

typedef struct JsonString {
    const char* doc_source;
    unsigned int start;
    unsigned int end;
} JsonString_t;

//
// initialize JsonString_t to reference the same data as the orginal JsonNode_t::string type
// returns 1 on success, else 0
//
int JsonString_init(JsonString_t* str, const char* doc_source, JsonNode_t* str_node);

//
// return length (in bytes) of given JsonString_t
//
unsigned long JsonString_size(JsonString_t* str);

//
// copy the contents of the string out of the original source
//
void JsonString_copy(const JsonString_t* str, char* restrict dest);
