#pragma once
#include <string>
#include "table.h"
#include "attribute.h"
#include "fitter.h"
#include <vector>
#include <set>
using namespace std;

class recmgr
{
public:
	recmgr();
	~recmgr();
	int rmInsertRecord(const string &fileName, const vector<attribute> &entry, const table &datatable);
	void rmDeleteWithIndex(const string fileName, int offset, const Fitter &fitter, const table &datatable);
	void rmDeleteWithoutIndex(const string fileName, const Fitter &fitter, const table &datatable);
	vector <vector <attribute> > rmSelectWithIndex(const string fileName, int offset, const Fitter &fitter, const table &datatable);
	vector <vector <attribute> > rmSelectWithoutIndex(const string fileName, const Fitter &fitter, const table &datatable);
	void rmAddIndex(const string dbName, const string BTreeName, const table &datatable, int itemIndex);
	set<int> rmGetAllOffsets(const string &fileName);
	void rmClear(const string fileName);
};

