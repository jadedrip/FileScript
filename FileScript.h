#pragma once

typedef void (*ParserFunc)(const char*, lua_State *) ;  // filename, lua

void register_parser(const char* extension, ParserFunc func);
void append_field(lua_State*, const char* name, const char* value);

void init_jpeg_parser();
