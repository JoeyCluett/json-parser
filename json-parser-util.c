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

#include "json-parser-util.h"
#include "json-parser-config.h"
#include "json-parser.h"

#include <string.h>
#include <stddef.h>
#include <stdlib.h>

static int JsonAPI_string_eq(const char* start, const char* end, const char* nt_cstr);

int JsonString_init(JsonString_t* str, const char* doc_source, JsonNode_t* str_node) {

    if(str == NULL || doc_source == NULL || str_node->type != JsonNodeType_string)
        return 0;

    str->doc_source = doc_source;
    str->start      = str_node->str.start;
    str->end        = str_node->str.end;
    return 1;
}

//
// handles escape sequences
// returns how many chars were written out
//
static inline size_t JsonAPI_handle_escape_char(char next_char, char* dest) {
    switch(next_char) {
    case 'b':
#ifdef JSONPARSER_BACKSPACE_ALT
        dest[0] = JSONPARSER_BACKSPACE_ALT; break;
#else
        dest[0] = '\b'; break;
#endif

    case 'f':
#ifdef JSONPARSER_FORMFEED_ALT
    dest[0] = JSONPARSER_FORMFEED_ALT; break;
#else
    dest[0] = '\f'; break;
#endif

    case 'r':
#ifdef JSONPARSER_CARRIAGERETURN_ALT
    dest[0] = JSONPARSER_CARRIAGERETURN_ALT; break;    
#else
    dest[0] = '\r'; break;
#endif

    case 't':
#ifdef JSONPARSER_TAB_ALT
    dest[0] = JSONPARSER_TAB_ALT; break;
#else
    dest[0] = '\t'; break;
#endif

    case 'n':  dest[0] = '\n'; break;
    case '"':  dest[0] = '"';  break;
    case '\\': dest[0] = '\\'; break;

    default:   dest[0] = '~';  return 0; // copy nothing i guess
    }

    return 1;
}

#ifdef JSONPARSER_FILTER_NONPRINTABLE_ASCII
static const char JsonAPI_printable_chars[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
};
#endif

size_t JsonString_copy(const JsonString_t* str, char* restrict dest) {

    char* dest_start = dest;

    const char* start = str->doc_source + str->start;
    const char* end   = str->doc_source + str->end;

    while(start < end) {

#if defined(JSONPARSER_USE_UTF8) && defined(JSONPARSER_UTF8_ALT)
        unsigned char c = *(unsigned char*)start;
        if(c & 0b10000000) {
            if((c & 0b11100000) == 0b11000000) { // 2-byte UTF-8 char
                dest[0] = JSONPARSER_UTF8_ALT;
                start += 2;
                dest++;
            } else if((c & 0b11110000) == 0b11100000) { // 3-byte UTF-8 char
                dest[0] = JSONPARSER_UTF8_ALT;
                start += 3;
                dest++;
            } else if((c & 0b11111000) == 0b11110000) { // 4-byte UTF-8 char
                dest[0] = JSONPARSER_UTF8_ALT;
                start += 4;
                dest++;
            } else {
                // malformed UTF-8 char
                // roll the dice
                dest[0] = '~';
                dest++;
                start++;
            }
        } else if(start[0] == '\\') {
            size_t l = JsonAPI_handle_escape_char(start[1], dest);
            dest += l;
            start += 2;
        } else {
#ifdef JSONPARSER_FILTER_NONPRINTABLE_ASCII
            if(JsonAPI_printable_chars[start[0]]) {
                dest[0] = start[0];
                dest++;
            }
            start++;
#else
            dest[0] = start[0];
            dest++;
            start++;
#endif
        }
#else
        if(start[0] == '\\') {
            size_t l = JsonAPI_handle_escape_char(start[1], dest);
            dest += l;
            start += 2;
#ifdef JSONPARSER_FILTER_NONPRINTABLE_ASCII
        } else if(JsonAPI_printable_chars[*(unsigned char*)start]) {
            dest[0] = start[0];
            dest++;
            start++;
        } else {
            start++;
        }
#else
        } else {
            dest[0] = start[0];
            dest++;
            start++;
        }
#endif
#endif
    }
    return (size_t)(dest - dest_start);
}

unsigned long JsonString_size(JsonString_t* str) {
    return (unsigned long)(str->end - str->start);
}

int JsonArrIter_init(JsonArrIter_t* iter, JsonNode_t* arr) {
    if(iter == NULL || arr == NULL | arr->type != JsonNodeType_array)
        return 0;

    iter->arr = arr;
    iter->element = arr->arr.first;
    return 1;
}

int JsonArrIter_next(JsonArrIter_t* iter) {
    if(iter->element == NULL)
        return 0;

    iter->element = iter->element->elem.next;
    return (iter->element == NULL ? 0 : 1);
}

JsonNode_t* JsonArrIter_current(JsonArrIter_t* iter) {
    if(iter->element == NULL)
        return NULL;
    return iter->element->elem.item;
}

int JsonObjIter_field_name_matches(JsonObjIter_t* iter, const char* doc_source, const char* fieldname) {
    JsonNode_t* pair = JsonObjIter_current(iter);

    if(pair == NULL)
        return 0;

    return JsonAPI_string_eq(doc_source + pair->pair.key_start, doc_source + pair->pair.key_end, fieldname);
}

JsonNode_t* JsonObj_field_by_name(const char* doc_source, JsonNode_t* obj, const char* fieldname) {
    JsonObjIter_t iter;
    if(!JsonObjIter_init(&iter, obj))
        return NULL;

    while(iter.current != NULL) {
        JsonNode_t* cur = iter.current;
        if(JsonAPI_string_eq(doc_source + cur->pair.key_start, doc_source + cur->pair.key_end, fieldname))
            return cur;

        // advance to next field
        (void)JsonObjIter_next(&iter);
    }

    return NULL;
}

JsonNode_t* JsonPair_field(JsonNode_t* pair) {
    return (pair->type == JsonNodeType_pair ? pair->pair.value : NULL);
}

int JsonPair_key(JsonNode_t* pair, const char* doc_source, JsonString_t* str) {
    if(pair->type != JsonNodeType_pair)
        return 0;
    
    str->doc_source = doc_source;
    str->start = pair->pair.key_start;
    str->end   = pair->pair.key_end;
    return 1;
}

JsonNode_t* JsonArr_index(JsonNode_t* arr, size_t idx) {

    JsonArrIter_t iter;
    if(!JsonArrIter_init(&iter, arr))
        return NULL;

    if(JsonArrIter_current(&iter) == NULL)
        return NULL;

    size_t i = 0ul;
    for(; i < idx; i++) {
        if(!JsonArrIter_next(&iter))
            return NULL;
    }

    return JsonArrIter_current(&iter);
}

int JsonObjIter_init(JsonObjIter_t* iter, JsonNode_t* top) {
    if(iter == NULL || top == NULL || top->type != JsonNodeType_object)
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

static int JsonAPI_string_eq(const char* start, const char* end, const char* nt_cstr) {
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

int JsonNumber_convert_from_json_string(const char* doc_source, JsonNode_t* num_node, JsonNumber_t* num) {
    if(doc_source == NULL || num_node == NULL || num_node->type != JsonNodeType_number || num_node == NULL)
        return 0;

    unsigned int numlen = num_node->num.end - num_node->num.start;
    if(numlen >= (JSONPARSER_MAX_NUM_LEN - 1))
        return 0;

    const char*       iter     = doc_source + num_node->num.start;
    const char* const end_iter = doc_source + num_node->num.end;

    if(iter >= end_iter)
        return 0;

    char tmpbuf[JSONPARSER_MAX_NUM_LEN] = {0};
    char* dest = tmpbuf;

    int real_type   = 0;
    int signed_type = 0;

    if(*iter == '-') {
        signed_type = 1;
    }

    while(iter != end_iter) {
        const char c = *(iter++);
        *(dest++) = c;
        if(c == '.' || c == 'e' || c == 'E') {
            real_type = 1;
            break; // dont need to test anything more, just copy the rest
        }
    }

    while(iter != end_iter)
        *(dest++) = *(iter++);

    if(real_type) {
        num->type = JsonNumberType_real;
        num->r = atof(tmpbuf);
    } else if(signed_type) {
        num->type = JsonNumberType_signed;
        num->s = atol(tmpbuf);
    } else {
        // default must be an unsigned type
        num->type = JsonNumberType_unsigned;
        num->u = 0ul;

        dest = tmpbuf;
        while(*dest) {
            num->u *= 10ul;
            num->u += (unsigned long int)(*dest - '0');
            dest++;
        }
    }

    return 1;
}

void JsonDateTime_init_default(JsonDateTime_t* dt) {
    dt->year = 0;
    dt->month = 0;
    dt->day = 0;
    dt->hours = 0;
    dt->minutes = 0;
    dt->seconds = 0;
    dt->milliseconds = 0;
}

//
// helper for converting datetime strings to integers.
// returns new string location on success, else NULL
//
static inline const char* JsonAPI_dt_convert_integer(const char* src, const int len, int* dest, const char followup) {
    char c = *src;
    int t = 0;
    int i;
    for(i = 0; i < len; i++) {
        if(c >= '0' && c <= '9') {
            t *= 10;
            t += (int)(c - '0');
            c = *(++src);
        } else {
            return NULL;
        }
    }

    if(c == followup) {
        *dest = t;
        return src + 1;
    }
    return NULL;
}

static inline char* JsonAPI_place_int(int val, char* d, int dlen) {
    char* tmp = d + dlen;
    d += (dlen - 1);
    while(dlen > 0) {
        int p = val % 10;
        val /= 10;
        *d-- = (char)(p + '0');
        dlen--;
    }
    return tmp;
}

int JsonDateTime_to_string(JsonDateTime_t* dt, char* dest, int dlen) {
    if(dlen != JSONDATETIME_LEN && dlen != JSONDATETIME_LEN_TR)
        return 0;

    // YYYY-MM-DDThh:mm:ss.xxxZ
    // - or -
    // YYYY-MM-DDThh:mm:ssZ

    dest = JsonAPI_place_int(dt->year, dest, 4);
    *dest++ = '-';
    dest = JsonAPI_place_int(dt->month, dest, 2);
    *dest++ = '-';
    dest = JsonAPI_place_int(dt->day, dest, 2);
    *dest++ = 'T';
    dest = JsonAPI_place_int(dt->hours, dest, 2);
    *dest++ = ':';
    dest = JsonAPI_place_int(dt->minutes, dest, 2);
    *dest++ = ':';
    dest = JsonAPI_place_int(dt->seconds, dest, 2);

    if(dlen == JSONDATETIME_LEN_TR) {
        *dest = 'Z';
        return 1;
    }

    *dest++ = '.';
    dest = JsonAPI_place_int(dt->milliseconds, dest, 3);
    *dest = 'Z';
    return 1;
}

static int JsonAPI_date_time_from_string(JsonDateTime_t* dt, const char* start, const char* end) {
    JsonDateTime_t t;
    t.milliseconds = 0; // all other fields are overwritten

    // YYYY-MM-DDThh:mm:ss.xxxZ
    // - or -
    // YYYY-MM-DDThh:mm:ssZ

    if((start = JsonAPI_dt_convert_integer(start, 4, &t.year,    '-')) == NULL) return 0;
    if((start = JsonAPI_dt_convert_integer(start, 2, &t.month,   '-')) == NULL) return 0;
    if((start = JsonAPI_dt_convert_integer(start, 2, &t.day,     'T')) == NULL) return 0;
    if((start = JsonAPI_dt_convert_integer(start, 2, &t.hours,   ':')) == NULL) return 0;
    if((start = JsonAPI_dt_convert_integer(start, 2, &t.minutes, ':')) == NULL) return 0;

    if(start[2] == '.') {
        if((start = JsonAPI_dt_convert_integer(start, 2, &t.seconds,      '.')) == NULL) return 0;
        if((start = JsonAPI_dt_convert_integer(start, 3, &t.milliseconds, 'Z')) == NULL) return 0;
    } else {
        if((start = JsonAPI_dt_convert_integer(start, 2, &t.seconds, 'Z')) == NULL) return 0;
    }

    if(start == end) {
        memcpy(dt, &t, sizeof(JsonDateTime_t));
        return 1;
    }
    return 0;
}

int JsonDateTime_from_json_string(JsonDateTime_t* dt, JsonString_t* str) {
    return JsonAPI_date_time_from_string(dt, str->doc_source + str->start, str->doc_source + str->end);
}

int JsonDateTime_from_cstring(JsonDateTime_t* dt, const char* str) {
    return JsonAPI_date_time_from_string(dt, str, str + strlen(str));
}
