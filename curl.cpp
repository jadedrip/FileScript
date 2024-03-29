#include "stdafx.h"
#include <curl/curl.h>
#include <codecvt>

size_t writeData(void *buffer, size_t size, size_t nmemb, void *lp)
{
	char* pData = (char*)buffer;

	std::string* out = (std::string*)lp;
	out->append(pData, size*nmemb);
	return nmemb;
}

std::wstring httpGet(const std::string& url)
{
	CURL *curl;
	std::string out;
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
	int res=curl_easy_perform(curl);
	if (res != CURLE_OK)
		out.clear();
	curl_easy_cleanup(curl);

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> conversion;

	return conversion.from_bytes(out.c_str());
}