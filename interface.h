#ifndef _INTERFACE_H_
#define _INTERFACE_H_
#include <string>
#include <vector>
#include "attribute.h"
#include "table.h"
#include "fitter.h"
#include "catalog.h"
using namespace std;
extern unsigned char nullfill[BlockSize];

class Response :
	public catalog
{
public:
	Response(bufmgr &s) :catalog(s){}
	bool succeed;
	string info;
	vector <vector <attribute> > result;
	inline Response() { succeed = true; }
	inline Response(string data) { succeed = false; info = data; }
	inline Response(vector <vector <attribute> > result) :result(result), succeed(true){}
	Response CreateIndex(const string indexName, const string &tableName, const string &itemName);
	Response DropIndex(const string indexName);
	Response CreateTable(const string &tableName, vector <item> data, int pk);
	Response DropTable(const string &tableName);
	Response Select(const string &tableName, const Fitter &fitter);
	Response Delete(const string &tableName, const Fitter &fitter);
	Response Insert(const string tableName, const vector<attribute> entry);
};

#endif
