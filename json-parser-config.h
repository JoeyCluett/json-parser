#pragma once

//
// makes JsonParser default allocate/deallocate functions available.
// these functions internally use malloc/free
//
#define JSONPARSER_HAS_MALLOC

//
// allows arrays to end with ,] and objects with ,}
// true, false, null are case-insensitive
//
#define JSONPARSER_NOT_STRICT

//
// used to determine max nested depth of JSON documents.
// this is not an exact measurement as there are internal
// nested behaviors as well
//
#define JSONPARSER_MAX_DEPTH 64

//
// support utf-8 encoding
//
#define JSONPARSER_USE_UTF8

//
// if utf-8 encoding is supported, convert extended chars to given.
// this is used when copying strings out via JsonString_t
//
#define JSONPARSER_UTF8_ALT '~'

//
// define alternate chars for various escape sequences.
// if a given alternate is not defined, the usual 
// corresponding C chars are used instead.
// backslash, quote, and newline cannot be re-defined
//
#define JSONPARSER_BACKSPACE_ALT      ' '
#define JSONPARSER_FORMFEED_ALT       ' '
#define JSONPARSER_CARRIAGERETURN_ALT '\n'
#define JSONPARSER_TAB_ALT            ' '

//
// skip all ASCII chars that dont have a printable representation
// most whitespace chars are handled elsewhere
//
#define JSONPARSER_FILTER_NONPRINTABLE_ASCII

