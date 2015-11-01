#ifndef _BTREE_H
#define _BTREE_H
#include <cstdio>
#include <string>
#include <vector>
#include <set>
#include "bufmgr.h"
#include "attribute.h"

using namespace std;

unsigned char nullfill[BlockSize];

struct node{
	bool isRoot;
	bool isLeaf;
	vector<int> sons;
	vector<attribute> data;
};

struct btreeHeader{
	int type;
	int length;
	int rootOffset;
	int fanOut;
};

class btree :
	public bufmgr
{
public:
	btree();
	btree(bufmgr &s) :bufmgr(s){}
	~btree();
	void btCreate(const string&, int, int);
	int btFindResult(const string&, const attribute&);
	int btFind(const string&, int, const attribute&, btreeHeader);
	void insertIntoParent(Block, const attribute&, Block, const string&, btreeHeader);
	void btInsert(const string&, const attribute&, int);
	void deleteEntry(Block, attribute, int, const string&, btreeHeader);
	bool btDelete(const string&, const attribute&);
	bool btExist(const string&, const attribute&);
	set<int> btFindLess(const string&, const attribute&);
	set<int> btFindMore(const string&, const attribute&);

	void saveHeader(const btreeHeader&, const string&);

	node getNode(const string&, int, const btreeHeader&);
	void nodetoBinary(const node&, unsigned char*, btreeHeader);
	void seetree(const string&, int, btreeHeader);
	void seetree(const string&);
private:
	vector<int> path;
};
#endif
