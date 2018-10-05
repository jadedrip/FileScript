#include "stdafx.h"

using namespace std;
//基于某些原因，以下两个子函数的实现跟原来参考代码中的实现有所区别,
//详细原因，笔者在CalculateDigest函数的注释中写明
string CalculateDigest(string &Digest, const string &Message)
69{
	70    SHA256 sha256;
	71    int DigestSize = sha256.DigestSize();
	72    char* byDigest;
	73    char* strDigest;
	74
		75    byDigest = new char[DigestSize];
	76    strDigest = new char[DigestSize * 2 + 1];
	77
		78    sha256.CalculateDigest((byte*)byDigest, (const byte *)Message.c_str(), Message.size());
	79    memset(strDigest, 0, sizeof(strDigest));
	80    //uCharToHex(strDigest, byDigest, DigestSize);
		81    //参考的代码中有以上这么一行，但是貌似不是什么库函数。
		82    //原作者大概是想把Hash值转换成16进制数保存到一个string buffer中，
		83    //然后在主程序中输出，方便debug的时候对照查看。
		84    //但是这并不影响计算Hash值的行为。
		85    //因此笔者注释掉了这行代码，并且修改了一下这个函数和后面的VerifyDigest函数，
		86    //略去原作者这部分的意图，继续我们的程序执行。
		87
		88    Digest = byDigest;
	89
		90    delete[]byDigest;
	91    byDigest = NULL;
	92    delete[]strDigest;
	93    strDigest = NULL;
	94
		95    return;
	96}