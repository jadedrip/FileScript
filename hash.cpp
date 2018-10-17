#include "stdafx.h"
#include <fstream>
#include <filesystem>
#include <cryptopp/sha.h>
#include <boost/iterator.hpp>
#include <boost/algorithm/hex.hpp>
#include "FileScript.h"

using namespace std;
using namespace CryptoPP;
// ���� hash һ���ļ�������ļ���СС�� 4M��ȫ�ļ� sha256������ʹ�ó���+ǰ4194304�ֽ�
string fastHashFile(const fs::path& file)
{
	SHA256 sha256;
	int digestSize = sha256.DigestSize();
	byte* byDigest = new byte[digestSize];

	size_t size = fs::file_size(file);
	if (size > 4194304) {
		sha256.Update((byte*)&size, sizeof(size_t));
	}

	char buffer[4096];
	ifstream fstream(file, ios::in | ios::binary);
	if (fstream.is_open()) {
		for (int i = 0; i < 1024; i++) {
			if (fstream.eof()) break;
			fstream.read(buffer, 4096);
			size_t len = fstream.gcount();
			sha256.Update((byte*)buffer, len);
		}
	}
	sha256.Final(byDigest);
	string hex;
	hex.reserve(digestSize * 2);
	boost::algorithm::hex_lower(byDigest, byDigest + digestSize, std::back_inserter(hex));
	delete[] byDigest;
	return hex;
}
