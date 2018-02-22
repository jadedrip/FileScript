// FileScript.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "FileScript.h"
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

namespace fs = std::experimental::filesystem;
namespace po = boost::program_options;
using namespace std;

map<string, ParserFunc> parsers;

void copy_file(std::string& from, std::string& to, bool move=false) {
	typedef boost::tokenizer<boost::char_separator<char> >     tokenizer;
	static boost::char_separator<char> sep("\\/");

	fs::path f(from);
	if (!fs::exists(f)) return;

	string cur;
	tokenizer tokens(to, sep);
	for (tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ) {
		const string& n = *tok_iter;
		cur += n + "/";

		if (tok_iter->empty()) {	// 这说明最后是斜杠
			to += f.filename().string();
			break;
		}
		// 最后一组特殊处理，如果有点认为是文件，否则当目录
		if (++tok_iter == tokens.end()) {
			if (string::npos != n.find('.'))
				break;
			else {
				to += "/" + f.filename().string();
			}
		}

		if (!fs::exists(cur)) {
			fs::create_directory(cur);
		}
	}

	if (move) {
		fs::rename(f, to);
	}
	else
		fs::copy_file(f, to, fs::copy_options::skip_existing);
}

int lua_copy_file(lua_State* L) {
	std::string to = lua_tostring(L, -1);
	std::string from = lua_tostring(L, -2);

	lua_pop(L, 2);
	copy_file(from, to, false);
	return 0;
}

int lua_move_file(lua_State* L) {
	std::string to = lua_tostring(L, -1);
	std::string from = lua_tostring(L, -2);

	lua_pop(L, 2);
	copy_file(from, to, true);
	return 0;
}

void init_c_function(lua_State*L) {
	lua_register(L, "copy", &lua_copy_file);
	lua_register(L, "move", &lua_move_file);
}

void run_script(const std::string& script_file, const std::string& source_dir, const vector<string>& defines) {
	lua_State*L = luaL_newstate();		// LUA 虚拟机指针
	int status = luaL_loadfile(L, script_file.c_str());
	if (status == LUA_ERRSYNTAX)
		throw std::runtime_error("Can't load file " + script_file + " with syntax error.");

	luaL_openlibs(L);
	init_c_function(L);

	//初始化函数调用，如果没有这一句，将无法获取到函数的实体
	if (lua_pcall(L, 0, 0, 0) != 0)
		throw std::runtime_error("Can't init lua " + script_file);

	for (auto &i : defines) {
		size_t idx = i.find('=');
		if (idx == string::npos) continue;

		string key = i.substr(0, idx);
		string val = i.substr(idx + 1);

		lua_pushstring(L, val.c_str());
		lua_setglobal(L, key.c_str());
	}

	fs::path fullpath(source_dir);

	fs::recursive_directory_iterator end_iter;
	for (fs::recursive_directory_iterator i(fullpath); i != end_iter; i++) {
		auto file = i->path();
		auto ext = file.extension().string();
		boost::to_upper(ext);
		// std::cout << file << std::endl;

		auto filename = file.string();
		int v = lua_getglobal(L, "run");        // 获取函数，压入栈中  

		// 准备参数
		lua_newtable(L);
		lua_pushstring(L, "filename");
		lua_pushstring(L, filename.c_str());
		lua_settable(L, -3);

		auto a = parsers.find(ext);
		if (a != parsers.end()) {
			a->second(filename.c_str(), L);
		}

		int x = lua_pcall(L, 1, 0, 0);
		if (x > 0) {
			auto c = lua_tostring(L, 1);
			std::cout << "return :" << c << std::endl;
		}
	}
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

	if (script_file.empty() || vm.count("help") || !fs::exists(script_file)) {
		std::cout << "filescript [options] script-file [source-dir] " << endl << desc << "\n";
		return 1;
	}

	try {
		run_script(script_file, source_dir, defines);
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

void append_field(lua_State *L, const char* name, const char* value)
{
	lua_pushstring(L, name);
	lua_pushstring(L, value);
	lua_settable(L, -3);
}
