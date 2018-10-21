#pragma once

#include <filesystem>
#include "map.h"
typedef void (*ParserFunc)(const char*, map_state*) ;  // filename, map

void recursiveDirectory(const std::string & script_file, const std::string & source_dir, const std::map<std::string, std::string>& defines);

void registerParser(const char* extension, ParserFunc func);
void init_jpeg_parser();

namespace fs = std::experimental::filesystem;
void initFile(const fs::path& fname);

