#include "catalog.h"
#include <fstream>

catalog::catalog()
{
}


catalog::~catalog()
{
}


/* check table existing */
bool catalog::cmExistTable(const string &name) {
	ifstream fin((name).c_str());
	if (!fin)
		return false;
	string header;
	fin >> header;
	fin.close();
	return header == name;

}
/* create a new table */
table catalog::cmCreateTable(const string &name, const vector <item> &data) {
	assert(!cmExistTable(name));
	table newtable(name, data);
	newtable.write();
	return newtable;
}

/* read a table from file */
table catalog::cmReadTable(const string &name) {
	assert(cmExistTable(name));
	table now;
	/* read it using file stream */
	ifstream fin((name).c_str());
	fin >> now.name;
	fin >> now.size;
	int itemsize;
	fin >> itemsize;
	for (int i = 0; i<itemsize; i++) {
		item nitem;
		fin >> nitem.name;
		fin >> nitem.type;
		fin >> nitem.length;
		fin >> nitem.unique;
		int n;
		fin >> n;
		for (int j = 0; j<n; j++) {
			string index;
			fin >> index;
			nitem.indices.insert(index);
		}
		now.items.push_back(nitem);
	}
	return now;
}
/* delete a table */
void catalog::cmDropTable(const string &name) {
	assert(cmExistTable(name));
	string tablename = name;
	/* delete the post name */
	while (*(tablename.end() - 1) != '.')
		tablename.erase(tablename.end() - 1);
	tablename.erase(tablename.end() - 1);

	/* delete all index file */
	table nowtable = cmReadTable(name);
	for (int i = 0; i<nowtable.items.size(); i++) {
		bool have = false;
		for (set<string>::iterator it = nowtable.items[i].indices.begin(); it != nowtable.items[i].indices.end(); it++)  {
			ixDeleteIndex(*it);
			have = true;
		}
		if (have) bmclear(tablename + "." + nowtable.items[i].name + ".index");
	}
	/* delete db file */
	rmClear(tablename + ".db");
	remove((name).c_str());
}