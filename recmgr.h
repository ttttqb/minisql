#ifndef _RECMGR_H_
#define _RECMGR_H_
#include <string>
#include "table.h"
#include "attribute.h"
#include "ruletree.h"
#include "idxmgr.h"
#include <vector>
#include <set>
using namespace std;

class recmgr:public btree
{
public:
	recmgr();
	~recmgr();
	int rmInsertRecord(const string &fileName, const vector<attribute> &entry, const table &datatable);
	void rmDeleteWithIndex(const string fileName, int offset, const Ruletree &ruletree, const table &datatable);
	void rmDeleteWithoutIndex(const string fileName, const Ruletree &ruletree, const table &datatable);
	vector <vector <attribute> > rmSelectWithIndex(const string fileName, int offset, const Ruletree &ruletree, const table &datatable);
	vector <vector <attribute> > rmSelectWithoutIndex(const string fileName, const Ruletree &ruletree, const table &datatable);
	void rmAddIndex(const string dbName, const string BTreeName, const table &datatable, int itemIndex);
	set<int> rmGetAllOffsets(const string &fileName);
	void rmClear(const string fileName);
};

#endif