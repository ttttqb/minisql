#ifndef _CATALOG_MANAGER_H_
#define _CATALOG_MANAGER_H_

#include <string>
#include <algorithm>
#include "table.h"
using namespace std;

class catalogmanager{
public:
	map<string,pair<string,int> > indexToTable;

	bool ExistTable(const string &name);
	table CreateTable(const string &name,const vector <item> &data);
	table ReadTable(const string &name);
	void DropTable(const string &name);
	bool ExistIndex(const string &indexname);
	bool RegisterIndex(const string &tablename,const string &indexname, int itemIndex);
	pair<string,int> AskIndex(const string indexName);
	bool DeleteIndex(const string indexName);

};


#endif