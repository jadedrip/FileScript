#pragma once

#include "map.h"
typedef void (*ParserFunc)(const char*, map_state*) ;  // filename, lua

void recursiveDirectory(const std::string & script_file, const std::string & source_dir, const std::map<std::string, std::string>& defines);

void register_parser(const char* extension, ParserFunc func);
void append_field(lua_State*, const char* name, const char* value);

void init_jpeg_parser();
