#include "stdafx.h"
#include <unordered_map>
#include <LuaBridge/LuaBridge.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "geohash/geohash.h"
#include "radix_tree/radix_tree.hpp"

std::string httpGet(const std::string& url);

using namespace std;
using namespace luabridge;

extern boost::property_tree::ptree g_config;
unordered_map<string, map<string, string>> regeoCache;

bool currentGeoInited = false;
radix_tree<string, map<string, string>> currentGeo;

/*
	getRegeoName( log, lat )
	JSON like {
	"queryLocation": [39.993253, 116.473195],
	"addrList": [{
		"type": "street",
		"status": 1,
		"name": "阜荣街",
		"admCode": "110105",
		"admName": "北京市,朝阳区",
		"addr": "",
		"nearestPoint": [116.47361, 39.99380],
		"distance": 69.203
	}, {
		"type": "poi",
		"status": 1,
		"name": "新一城购物中心",
		"id": "ANB000A80GTY",
		"admCode": "110105",
		"admName": "北京市,北京市,朝阳区,",
		"addr": "阜荣街10号(望京广顺南大街口)",
		"nearestPoint": [116.47318, 39.99327],
		"distance": 2.236
	}, {
		"type": "doorPlate",
		"status": 0,
		"name": "",
		"admCode": "",
		"admName": "",
		"nearestPoint": [],
		"distance": -1
	}]
}
*/
void parseAliyunJson(const std::string& req, std::map<string, string> &m)
{
	boost::property_tree::ptree root;
	boost::property_tree::ptree items;
	istringstream ss(req);
	boost::property_tree::read_json(ss, root);

	items = root.get_child("addrList");
	for (boost::property_tree::ptree::iterator it = items.begin(); it != items.end(); ++it) {
		for (boost::property_tree::ptree::iterator it = items.begin(); it != items.end(); ++it) {
			auto item = it->second;
			auto type = item.get<string>("type");
			auto name = item.get<string>("name");
			if (!name.empty()) m[type] = name;
		}
	}
}

int getGeoHash(int distance)
{
	static const int tb[] = { 2500000, 630000, 78000, 20000, 2400, 610, 76, 19, 2 };
	const int a = sizeof(tb) / sizeof(int);
	for (int i = a; i > 1; i--) {
		if (distance < tb[i - 1]) return i;
	}
	return 1;
}

string getGeoHash(double latitude, double longitude, size_t distance)
{
	auto capacity = getGeoHash((int)distance);
	char cache[9];
	geohash_encode(latitude, longitude, cache, capacity);
	return string(cache, cache + capacity);
}

string utf8ToGbk(const string& utf8);

void initGeoFromConfig()
{
	auto iter=g_config.get_child("location");
	for (auto &i : iter) {
		double latitude=i.second.get<double>("latitude");
		double longitude = i.second.get<double>("longitude");
		double distance = i.second.get<double>("distance");

		auto h=getGeoHash(latitude, longitude, distance);

		map<string, string> info;
		for (auto &l : i.second.get_child("info")) {
			string v = l.second.get<string>("");
			info[l.first] = v;
		}
		currentGeo[h] = info;
	}
	currentGeoInited = true;

	// radix_tree<std::string, int>::iterator it;
	// it = tree.longest_match(key);
}

std::map<string, string> loadFromConfig(double latitude, double longitude)
{
	char out[10];
	if (GEOHASH_OK != geohash_encode(latitude, longitude, out, 10)) 
		return std::map<string, string>();

	std::string key(out, out + 10);

	if (!currentGeoInited)
		initGeoFromConfig();

	typedef decltype(currentGeo)::iterator Iterator;
	vector<Iterator> vec;

	// TODO: greedy_match 并不符合我们的需求，需要重写算法
	currentGeo.greedy_match(key, vec);
	if (vec.empty()) 
		return std::map<string, string>();

	// 返回键最长（最准确）的
	size_t sz = 0;
	std::map<string, string>* m = nullptr;
	for (auto& i : vec) {
		auto k = i->first;
		auto x=key.find_first_of(k);
		if (x == 0) {
			if (i->first.size() > sz) {
				sz = i->first.size();
				m = &(i->second);
			}
		}
	}
	return m ? *m : std::map<string, string>();
}

int luaGetGeoInfo(lua_State*L)
{
	// TODO: 将更多数据暴露给 lua
	auto log = LuaRef::fromStack(L, -1);
	auto lat = LuaRef::fromStack(L, -2);
	if (!lat.isNumber() || !log.isNumber())
		throw runtime_error("getRegeoName( [number] log, [number]lat )  got wrong type");

	double dLat = lat;
	double dLog = log;

	map<string, string> map = loadFromConfig(dLat, dLog);
	if (!map.empty()) {
		LuaRef(L, map).push();
		return 1;
	}

	std::string key = to_string(dLat) + "," + to_string(dLog);
	auto iter = regeoCache.find(key);
	if (iter == regeoCache.end()) {
		std::string req;
		try {
			std::string url = "http://gc.ditu.aliyun.com/regeocoding?type=110&l=" + key;
			req = httpGet(url);
			std::clog << "Http get: " << url << "\r\n\t" << utf8ToGbk(req) << std::endl;
			parseAliyunJson(req, map);
			if (map.empty()) {
				clog << "Can't get name for " << key << endl;
			}
			LuaRef(L, map).push();
			regeoCache[key] = move(map);
		} catch (exception & e) {
			cerr << "Get regeo " << dLat << "," << dLog << " failed with " << e.what() << "\r\n\t" << req << endl;
		}
	} else {
		auto &map = iter->second;
		LuaRef(L, map).push();
	}
	return 1;
}
