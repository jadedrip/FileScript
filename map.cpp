#include "stdafx.h"
#include <map>
#include <string>
#include "map.h"

typedef std::map<std::string, std::string> MapType;

void __stdcall map_append(map_state *m, const char* name, const char* value)
{
	auto& x=*((MapType*)m->data);
	x[name] = value;
}

void __stdcall map_remove(map_state *m, const char* name)
{
	auto& x = *((MapType*)m->data);
	x.erase(name);
}

const char* __stdcall map_get(map_state *m, const char* name)
{
	auto& x = *((MapType*)m->data);
	auto i=x.find(name);
	if (i == x.end()) return nullptr;
	return i->second.c_str();
}

unsigned long long __stdcall map_size(map_state *m)
{
	auto& x = *((MapType*)m->data);
	return x.size();
}
