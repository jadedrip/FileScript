#include "stdafx.h"

using namespace std;
//基于某些原因，以下两个子函数的实现跟原来参考代码中的实现有所区别,
//详细原因，笔者在CalculateDigest函数的注释中写明
string CalculateDigest(string &Digest, const string &Message)
{
	    SHA256 sha256;
	    int DigestSize = sha256.DigestSize();
	    char* byDigest;
	    char* strDigest;
	
		    byDigest = new char[DigestSize];
	    strDigest = new char[DigestSize * 2 + 1];
	
		    sha256.CalculateDigest((byte*)byDigest, (const byte *)Message.c_str(), Message.size());
	    memset(strDigest, 0, sizeof(strDigest));

		
		    Digest = byDigest;
	
		    delete[]byDigest;
	    byDigest = NULL;
	    delete[]strDigest;
	    strDigest = NULL;
	
		    return;
}
}