#include "stdafx.h"
#include <lua.h>
#include <boost/tokenizer.hpp>

using namespace std;
namespace fs = std::experimental::filesystem;
void copy_file(std::string& from, std::string& to, bool move = false)
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

int lua_copy_file(lua_State* L)
{
	std::string to = lua_tostring(L, -1);
	std::string from = lua_tostring(L, -2);

	lua_pop(L, 2);
	copy_file(from, to, false);
	return 0;
}

int lua_move_file(lua_State* L)
{
	std::string to = lua_tostring(L, -1);
	std::string from = lua_tostring(L, -2);

	lua_pop(L, 2);
	copy_file(from, to, true);
	return 0;
}

void init_c_function(lua_State*L)
{
	lua_register(L, "copy", &lua_copy_file);
	lua_register(L, "move", &lua_move_file);
}