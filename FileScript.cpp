// FileScript.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "FileScript.h"
#include <curl/curl.h>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <LuaBridge/Map.h>
#include <LuaBridge/LuaBridge.h>

namespace po = boost::program_options;
using namespace std;
using namespace luabridge;

std::string datapath;
map<string, ParserFunc> parsers;
string fastHashFile(const fs::path& file);

boost::property_tree::ptree g_config;
void initLuaFunction(lua_State*L);
void scanDirectory(lua_State*L, const fs::path& dir)
{
	fs::recursive_directory_iterator end_iter;
	for (fs::recursive_directory_iterator i(dir); i != end_iter; i++) {
		auto file = i->path();
		if (!fs::exists(file)) continue;
		auto filename = file.string();
		try {
			if (fs::is_directory(file)) {
				scanDirectory(L, file);
				continue;
			}
			initFile(file);

			auto ext = file.extension().string();
			boost::to_upper(ext);
			std::cout << "Find: " << file << std::endl;

			LuaRef run = getGlobal(L, "run");        // 获取函数，压入栈中  

			// 准备参数
			std::map<std::string, std::string> prop;
			prop["filename"] = file.u8string();
			//string hash=fastHashFile(file);
			//prop["fast_hash"] = hash;

			map_state state;
			state.data = &prop;
			// 运行插件
			auto a = parsers.find(ext);
			if (a != parsers.end()) {
				a->second(filename.c_str(), &state);

			}

			run(prop);
		} catch (exception& e) {
			std::cerr << "File "<< filename << " exception: " << e.what() << std::endl;
		}
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

#include "radix_tree/radix_tree.hpp"
int main(int ac, char* av[])
{
	std::string script_file;
	std::string source_dir;
	std::vector<string> defines;

	curl_global_init(CURL_GLOBAL_ALL);
	init_jpeg_parser();

	if (fs::exists("fileScript.conf")) {
		ifstream ss("fileScript.conf");
		try {
			boost::property_tree::read_json(ss, g_config);
		} catch (exception &e) {
			cerr << "Can't load config file fileScript.conf: " << e.what() << endl;
			return -1;
		}
	}

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
	catch (LuaException& e) {
		std::cerr << e.what() << std::endl;
		return -1;
	}
	catch (exception e) {
		std::cerr << e.what() << std::endl;
		return -1;
	}
	return 0;
}

void registerParser(const char* extension, ParserFunc func)
{
	string e = extension;
	boost::to_upper(e);
	parsers[e] = func;
}

