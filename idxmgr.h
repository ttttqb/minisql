#ifndef _IDXMGR_H_
#define _IDXMGR_H_
#include <fstream>
#include <cstdio>
#include <string>
#include <vector>
#include <set>
#include "bufmgr.h"
#include "attribute.h"

using namespace std;

struct node{
	bool isRoot;
	bool isLeaf;
	vector<int> sons;
	vector<attribute> data;
	node(){}
	node(bool root, bool leaf) :isRoot(root), isLeaf(leaf){}
};

struct btHeader{
	int type;
	int length;
	int rootOffset;
	int fanOut;
	btHeader(){}
	btHeader(int type, int length, int rootOffset) :type(type), length(length), rootOffset(rootOffset){}
};

class btree :public bufmgr
{
public:
	btree(){}
	~btree(){}
	void btCreate(const string&, int, int);
	int btFindResult(const string&, const attribute&);
	int btFind(const string&, int, const attribute&, btHeader);
	void insertIntoParent(Block, const attribute&, Block, const string&, btHeader);
	void btInsert(const string&, const attribute&, int);
	void deleteEntry(Block, attribute, int, const string&, btHeader);
	bool btDelete(const string&, const attribute&);
	bool btExist(const string&, const attribute&);
	set<int> btFindLess(const string&, const attribute&);
	set<int> btFindMore(const string&, const attribute&);

	void saveHeader(const btHeader&, const string&);

	node getNode(const string&, int, const btHeader&);
	void convertBinary(const node&, unsigned char*, btHeader);
private:
	vector<int> path;
};
class idxmgr
{
	/* pair<string,int> for file name and offset */
	map <string, pair<string, int> > indexToTable;
public:
	idxmgr() {
		/* index.catalog save all index of all table */
		ifstream fin("index.catalog");
		if (!fin)
			return;
		string a, b;
		int c;
		while (fin >> a >> b >> c) {
			indexToTable[a] = make_pair(b, c);
		}
		fin.close();
	}
	~idxmgr() {
		ofstream fout("index.catalog");
		for (map <string, pair<string, int> >::iterator it = indexToTable.begin(); it != indexToTable.end(); it++) {
			fout << it->first << " " << it->second.first << " " << it->second.second << endl;
		}
		fout.close();
	}
	bool ixExistIndex(const string &indexname);
	bool ixCreateIndex(const string &tablename, const string &indexname, int itemIndex);
	pair<string, int> ixAskIndex(const string indexName);
	bool ixDeleteIndex(const string indexName);
};

#endif