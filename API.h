#ifndef _API_H_
#define _API_H_
#include <string>
#include <vector>
#include "attribute.h"
#include "table.h"
#include "ruletree.h"
#include "catalog.h"
#include "idxmgr.h"
using namespace std;

class API :
	public catalog
{
public:
	bool succeed;
	string info;
	vector <vector <attribute> > result;
	inline API() { succeed = true; }
	inline API(string data) { succeed = false; info = data; }
	inline API(vector <vector <attribute> > result) :result(result), succeed(true){}
	API APIcreateIndex(const string, const string&, const string&);
	API APIdropIndex(const string);
	API APIcreateTable(const string&, vector <item>, int);
	API APIdropTable(const string&);
	API APIselect(const string&, const Ruletree&);
	API APIdelete(const string&, const Ruletree&);
	API APIinsert(const string, const vector<attribute>);
};

#endif
