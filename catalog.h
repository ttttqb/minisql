#ifndef _CATALOG_H_
#define _CATALOG_H_
#include "table.h"
#include "recmgr.h"
#include "idxmgr.h"


class catalog :
	public recmgr, public idxmgr
{
public:
	catalog();
	~catalog();
	bool cmExistTable(const string &name);
	table cmCreateTable(const string &name, const vector <item> &data);
	table cmReadTable(const string &name);
	void cmDropTable(const string &name);
};

#endif