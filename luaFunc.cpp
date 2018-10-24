#include "stdafx.h"
#include <lua.h>
#include <regex>
#include <thread>
#include <fstream>
#include <iterator>
#include <map>
#include <unordered_map>
#include <Windows.h>
#include <boost/tokenizer.hpp>
#include <LuaBridge/LuaBridge.h>
#include <LuaBridge/detail/Iterator.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "buffer.hpp"
#include "FileScript.h"

using namespace std;
using namespace luabridge;
thread_local string hashString;
thread_local const fs::path* currentFile;
Buffer<wchar_t> wbuffer;
Buffer<char> buffer;

std::string httpGet(const std::string& url);
string fastHashFile(const fs::path& file);

wstring fromUtf8(const string& utf8)
{
	size_t sz=utf8.size();
	wchar_t* ptr = wbuffer.get(sz);

	int x=::MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), ptr, (int)sz);
	return wstring(ptr, x);
}

string toGbk(const wstring& unicode)
{
	size_t sz = unicode.size();
	char* ptr = buffer.get(sz*2);

	int x = ::WideCharToMultiByte(CP_ACP, 0, unicode.c_str(), (int)unicode.size(), ptr, (int)sz*2, 0, 0);
	return string(ptr, x);
}

string utf8ToGbk(const string& utf8)
{
	return toGbk(fromUtf8(utf8));
}

void copyFile(std::string& from, std::string& to, bool move = false)
{
	typedef boost::tokenizer<boost::char_separator<char> >     tokenizer;
	static boost::char_separator<char> sep("\\/");

	wstring wf = fromUtf8(from);
	wstring wt = fromUtf8(to);

	fs::path f(wf);
	if (!fs::exists(f)) return;

	fs::path t(wt);
	fs::create_directories(t);

	// std::filesystem 似乎对中文支持不好，先改为 windows api
	wchar_t c = wt.back();
	if (c != '\\' && c != '/') {
		wt.push_back('/');
		wt.append(f.filename().wstring());
	}
	if (move) {
		//fs::rename(f, to);
		::MoveFileW(wf.c_str(), wt.c_str());
	} else {
		::CopyFileW(wf.c_str(), wt.c_str(), TRUE);
	}
		
//	 else
//		fs::copy_file(f, to, fs::copy_options::fail_if_exists);

//#endif // DEBUG
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
	copyFile(from, to, true);
	return 0;
}

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
	// clog << "File " << currentFile->string() << " hash to :" << dir.string() << endl;
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
	// std::clog << "loadData: " << key.tostring() << std::endl;
	fs::path path = getDataPath();
	path /= key.tostring();

	if (!fs::exists(path)) return 0;

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
			break;
		}
		case 't':
		case 'f':
		{
			string v;
			file >> v;
			LuaRef d(L, v != "false");
			d.push();
			break;
		}
		case '{':// TODO: read json file
			return 0;
		default:
		{
			double n;
			file >> n;
			LuaRef d(L, n);
			d.push();
			break;
		}
		}
		return 1;
	} else {
		return 0;
	}
}

/*
	splice( str, regex )
*/
int luaSplice(lua_State*L)
{
	auto rex = LuaRef::fromStack(L, -1);
	auto str = LuaRef::fromStack(L, -2);

	if (!str.isString() || !rex.isString())
		throw runtime_error("splice([string] str, [string]regex) got wrong type.");

	auto s=str.tostring();
	std::regex re{ rex };
	sregex_token_iterator iter(s.begin(), s.end(), re, -1);
	auto table=LuaRef::newTable(L);
	for (; iter != sregex_token_iterator(); iter++) {
		table.append(iter->str());
	}
	table.push();
	return 1;
}

int luaPrintUtf8(lua_State*L)
{
	auto str = LuaRef::fromStack(L, -1);
	if (str.isString()) {
		auto w = fromUtf8(str.tostring());
		auto gbk = toGbk(w);
		cout << gbk << endl;
	}
	return 0;
}

int luaGetGeoInfo(lua_State*L);
void initLuaFunction(lua_State*L)
{
	lua_register(L, "copy", &luaCopyFile);
	lua_register(L, "move", &luaMoveFile);
	lua_register(L, "spliceString", &luaSplice);
	lua_register(L, "saveData", &luaSaveData);
	lua_register(L, "loadData", &luaLoadData);
	lua_register(L, "getGeoInfo", &luaGetGeoInfo);
	lua_register(L, "printUtf8", &luaPrintUtf8);
}

// http://gc.ditu.aliyun.com/regeocoding?l=39.993253,116.473195&type=010