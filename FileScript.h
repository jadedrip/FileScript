#pragma once

typedef void (*ParserFunc)(const char*, lua_State *) ;  // filename, lua

void run_script(const std::string & script_file, const std::string & source_dir, const map<string, string>& defines);

void register_parser(const char* extension, ParserFunc func);
void append_field(lua_State*, const char* name, const char* value);

void init_jpeg_parser();
