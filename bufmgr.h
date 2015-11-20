#ifndef _BUFMGR_H
#define _BUFMGR_H
#include <cstdio>
#include <string>
#include <cassert>
#include <list>
#include <map>

using namespace std;
const int BlockSize = 4096;
const int BufferSize = 100;

class Block{
public:
	unsigned char data[BlockSize];
	unsigned int offset;
	string fname;
};

class Buffer :public Block{
public:
	bool dirty;
};

class FileStruct{
public:
	FILE *fp;
	int fsize;
	~FileStruct(){
		//delete fp;
	}
};

class bufmgr
{
public:
	bufmgr();
	bufmgr(bufmgr& s) :flist(s.flist), blist(s.blist), page(s.page){}
	~bufmgr();

	FileStruct bmopenFile(const string&);
	Block bmnewBlock(const string&);
	void bmwriteback(const Buffer&);
	void bmnewBuffer(const string&, int);
	void bmprintself(void);
	Block bmreadBlock(const string&, int);
	void bmwriteBlock(Block&);
	void bmflushBuffer(void);
	void bmreleaseBlock(const string&, int);
	void bmclear(const string&);
private:
	map <string, FileStruct> *flist;
	map <string, list<int>> *blist;
	list <Buffer> *page;
};

#endif