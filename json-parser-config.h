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
// makes JsonParser default allocate/deallocate functions available.
// these functions internally use malloc/free
//
///#define JSONPARSER_HAS_MALLOC

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
// defines the size of the char buffer used internally to convert 
// JSON numbers to an internal primitive type
//
#define JSONPARSER_MAX_NUM_LEN 128

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

