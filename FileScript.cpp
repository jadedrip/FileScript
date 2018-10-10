// FileScript.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "FileScript.h"
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <LuaBridge/LuaBridge.h>

namespace fs = std::experimental::filesystem;
namespace po = boost::program_options;
using namespace std;

map<string, ParserFunc> parsers;
void initLuaFunction(lua_State*L);
string fastHashFile(const fs::path& file);

std::string datapath;

void append_field(lua_State *L, const char* name, const char* value)
{
	lua_pushstring(L, name);
	lua_pushstring(L, value);
	lua_settable(L, -3);
}

// 弹出一个值，并转换成字符串
std::string popString(lua_State*L, int index)
{
	int type = lua_type(L, index);
	switch (type) {
		case LUA_TSTRING: // string
			return lua_tostring(L, index);
		case LUA_TBOOLEAN:
		{// bool
			int v = lua_toboolean(L, index);
			return v ? "true" : "false";
		}
		case LUA_TNUMBER:
		{
			auto v = lua_tonumber(L, index);
			return std::to_string(v);
		}
		default:
			throw std::runtime_error("Unknown type in returned table: " + std::string(lua_typename(L, type)));
	}
}

void saveProp(const std::string& hash, const map<string,string>& prop)
{

}

using namespace luabridge;
void scanDirectory(lua_State*L, const fs::path& dir)
{
	fs::recursive_directory_iterator end_iter;
	for (fs::recursive_directory_iterator i(dir); i != end_iter; i++) {
		auto file = i->path();

		if (fs::is_directory(file)) {
			scanDirectory(L, file);
			continue;
		}

		auto ext = file.extension().string();
		boost::to_upper(ext);
		std::cout << "Find: " << file << std::endl;

		auto filename = file.string();
		LuaRef run = getGlobal(L, "run");        // 获取函数，压入栈中  

		// 准备参数
		std::map<std::string, std::string> prop;
		prop["filename"] = filename;
		string hash=fastHashFile(file);
		prop["fast_hash"] = hash;

		map_state state;
		state.data = &prop;

		// 运行插件
		auto a = parsers.find(ext);
		if (a != parsers.end()) {
			a->second(filename.c_str(), &state);
		}

		auto tab = LuaRef::newTable(L);
		for (auto i : prop) {
			tab[i.first]= i.second;
		}
		// int x = lua_pcall(L, 1, LUA_MULTRET, 0);
		auto ref=run(tab);
		if (ref.isTable()) {
			size_t size = ref.length();
			for (auto i = 1; i <= size; i++) {
				LuaRef v=ref[i];
				if (v.isBool()) {
					std::cout << v.cast<bool>() << std::endl;
				}
			}

			cout << ref["moved"].cast<bool>() << std::endl;
			ref.print(std::cout);
		}

		//if (x > 0) {
		//	auto c = lua_tostring(L, 1);
		//	std::cout << "return :" << c << std::endl;
		//} else {
		//	std::cout << "End of: " << filename << std::endl;
		//	if (lua_istable(L, -1)) {
		//	/*  表放在索引 '-2' 处 */
		//		lua_pushnil(L);  /* 第一个键 */
		//		while (lua_next(L, -2) != 0) {
		//		  /* 使用 '键' （在索引 -2 处） 和 '值' （在索引 -1 处）*/
		//			string key = popString(L, -2);
		//			string val = popString(L, -1);
		//			prop[key] = val;
		//			/* 移除 '值' ；保留 '键' 做下一次迭代 */
		//			lua_pop(L, 1);
		//		}
		//		saveProp(hash, prop);
		//	}
		//}
	}
}

void recursiveDirectory(const std::string& script_file, const std::string& source_dir, const map<string, string>& defines) {
	lua_State*L = luaL_newstate();		// LUA 虚拟机指针
	int status = luaL_loadfile(L, script_file.c_str());
	if (status == LUA_ERRSYNTAX)
		throw std::runtime_error("Can't load file " + script_file + " with syntax error.");

	fs::path data(datapath);
	fs::create_directories(data);

	luaL_openlibs(L);
	initLuaFunction(L);

	//初始化函数调用，如果没有这一句，将无法获取到函数的实体
	if (lua_pcall(L, 0, 0, 0) != 0)
		throw std::runtime_error("Can't init lua " + script_file);

	// 设置全局变量
	for (auto &i : defines) {
		lua_pushstring(L, i.second.c_str());
		lua_setglobal(L, i.first.c_str());
	}

	fs::path fullpath(source_dir);
	scanDirectory(L, fullpath);
	lua_close(L);
}

int main(int ac, char* av[])
{
	std::string script_file;
	std::string source_dir;
	std::vector<string> defines;

	init_jpeg_parser();

	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("define,D", po::value<vector<string>>(&defines), "define k=v")
		("datapath,P", po::value<string>(&datapath), "The path which stone data file.")
		;

	po::options_description hidden("Hidden options");
	hidden.add_options()
		("source-dir", po::value<string>(&source_dir)->default_value("."), "Search the path")
		("script-file", po::value<string>(&script_file), "Run the script file.")
		;

	po::positional_options_description p;
	p.add("script-file", 1);
	p.add("source-dir", 2);

	po::options_description cmdline_options;
	cmdline_options.add(desc).add(hidden);

	po::variables_map vm;
	po::store(po::command_line_parser(ac, av).options(cmdline_options).positional(p).run(), vm);
	po::notify(vm);

	// 确定数据目录
	if (datapath.empty())
		datapath = "./data";
	char c=datapath.back();
	if (c != '/' && c != '\\')
		datapath.push_back('/');

	if (script_file.empty() || vm.count("help") || !fs::exists(script_file)) {
		std::cout << "filescript [options] script-file [source-dir] " << endl << desc << "\n";
		return 1;
	}

	map<string, string> paramters;
	for (auto &i : defines) {
		size_t idx = i.find('=');
		if (idx == string::npos) continue;

		string key = i.substr(0, idx);
		string val = i.substr(idx + 1);

		paramters[key] = val;
	}
	try {
		recursiveDirectory(script_file, source_dir, paramters);
	}
	catch (exception e) {
		std::cerr << e.what() << std::endl;
		return -1;
	}
	return 0;
}

void register_parser(const char* extension, ParserFunc func)
{
	string e = extension;
	boost::to_upper(e);
	parsers[e] = func;
}

