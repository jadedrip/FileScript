#include "stdafx.h"
#include <lua.h>
#include <thread>
#include <fstream>
#include <boost/tokenizer.hpp>
#include <LuaBridge/LuaBridge.h>
#include <LuaBridge/detail/Iterator.h>

using namespace std;
namespace fs = std::experimental::filesystem;

string fastHashFile(const fs::path& file);
void copyFile(std::string& from, std::string& to, bool move = false)
{
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
	} else
		fs::copy_file(f, to, fs::copy_options::skip_existing);
}

int luaCopyFile(lua_State* L)
{
	std::string to = lua_tostring(L, -1);
	std::string from = lua_tostring(L, -2);

	lua_pop(L, 2);
	copyFile(from, to, false);
	return 0;
}

int luaMoveFile(lua_State* L)
{
	std::string to = lua_tostring(L, -1);
	std::string from = lua_tostring(L, -2);

	lua_pop(L, 2);
	// TODO: 调试用
#ifdef _DEBUG
	std::cout << "Move: " << from << " to " << to << std::endl;
#else
	copyFile(from, to, true);
#endif // DEBUG
	return 0;
}

using namespace luabridge;
thread_local string hashString;
thread_local const fs::path* currentFile;

void initFile(const fs::path& f)
{
	currentFile = &f;
	hashString.clear();
}

extern std::string datapath;
fs::path getDataPath()
{
	if (hashString.empty()) {
		hashString=fastHashFile(*currentFile);
	}
	string p1 = hashString.substr(0, 2);
	string p2 = hashString.substr(2, 2);
	fs::path dir(datapath);
	dir /= p1;
	dir /= p2;
	dir /= hashString;
	return dir;
}

void writeObject(ofstream& o, const LuaRef& data)
{
	if (data.isString()) {
		o << "\"" << data.tostring() << "\"";
	} else if (data.isNumber()) {
		double n = data;
		o << to_string(n);
	} else if (data.isBool()){
		o << ((bool)data ? "true" : "false");
	} else if (data.isTable()) {
		o.put('{');
		Iterator iter(data);
		while (true) {
			auto key=iter.key();
			if (key.isString()) {
				o << '"' << key.tostring() << '"' << ':';
			} else {
				o << "\"___" << key.tostring() << '"' << ':';
			}
			writeObject(o, iter.value());
			++iter;
			if (iter.isNil()) break;
			o.put(',');
		}
		o.put('}');
	}
}

int luaSaveData(lua_State* L)
{
	auto data = LuaRef::fromStack(L, -1);
	auto key = LuaRef::fromStack(L, -2);
	if (!key.isString())
		throw std::runtime_error("saveData([string] key, data) got wrong type.");
	fs::path path = getDataPath();
	fs::create_directories(path);
	path /= key.tostring();
	ofstream ofs(path);
	writeObject(ofs, data);
	return 0;
}

int luaLoadData(lua_State* L)
{
	auto key = LuaRef::fromStack(L, -1);
	if (!key.isString())
		throw std::runtime_error("loadData([string] key) got wrong type");
	std::clog << "loadData: " << key.tostring() << std::endl;
	fs::path path = getDataPath();
	path /= key.tostring();

	ifstream file(path);
	if (file.is_open()) {
		char c = file.peek();
		switch (c) {
		case '"':
		{
			string v;
			file >> v;
			v = v.substr(1, v.size() - 2);
			LuaRef d(L, v);
			d.push();
		}
		case 't':
		case 'f':
		{
			string v;
			file >> v;
			LuaRef d(L, v != "false");
			d.push();
		}
		case '{':// TODO: read json file
			return 0;
		default:
		{
			double n;
			file >> n;
			LuaRef d(L, n);
			d.push();
		}
		}
		return 1;
	} else {
		return 0;
	}
}

void initLuaFunction(lua_State*L)
{
	lua_register(L, "copy", &luaCopyFile);
	lua_register(L, "move", &luaMoveFile);
	lua_register(L, "saveData", &luaSaveData);
	lua_register(L, "loadData", &luaLoadData);
}