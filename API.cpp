#include <vector>
#include <cstdio>
#include <cassert>
#include <algorithm>
#include "API.h"
#include "catalog.h"
#include "recmgr.h"

using namespace std;

API API::APIcreateIndex(const string indexName, const string &tname, const string &itemName) {
	if (!cmExistTable(tname + ".table")) {
		return API("No such table");
	}
	table nowtable = cmReadTable(tname + ".table");
	int itemIndex = -1;
	/* find item in table */
	for (int i = 0; i<nowtable.items.size(); i++) {
		if (nowtable.items[i].name == itemName) {
			itemIndex = i;
		}
	}
	/* not found */
	if (itemIndex == -1) {
		return API("No such item");
	}
	/* not a unique item */
	if (!nowtable.items[itemIndex].unique) {
		return API("The item must be unique");
	}
	/* index on this item exists*/
	if (!ixCreateIndex(tname, indexName, itemIndex)) {
		return API("The index already exists");
	}
	/* create index */
	nowtable.items[itemIndex].indices.insert(indexName);
	nowtable.write();
	if (nowtable.items[itemIndex].indices.size() == 1) {
		rmAddIndex(tname + ".db", tname + "." + nowtable.items[itemIndex].name + ".index", nowtable, itemIndex);
	}
	return API();
}

API API::APIdropIndex(const string indexName) {
	if (!ixExistIndex(indexName)) {
		return API("No such index");
	}
	/* get index from indextable */
	pair<string, int> psi = ixAskIndex(indexName);
	/* psi.first is dbname, second is index id */
	table nowtable = cmReadTable(psi.first + ".table");
	assert(nowtable.items[psi.second].indices.count(indexName));
	/* erase index */
	nowtable.items[psi.second].indices.erase(indexName);
	nowtable.write();
	ixDeleteIndex(indexName);
	/* delete file */
	if (nowtable.items[psi.second].indices.size() == 0)
		bmclear(psi.first + "." + nowtable.items[psi.second].name + ".index");
	return API();
}
API API::APIcreateTable(const string &tname, vector <item> data, int pk) {
	if (cmExistTable(tname + ".table")) {
		return API("Table already exists");
	}
	/* call function in record manager */
	table newtable = cmCreateTable(tname + ".table", data);
	/* primary key is defaultly unique */
	newtable.items[pk].unique = true;
	newtable.write();
	return APIcreateIndex(tname + ".PrimaryKeyDefault", tname, newtable.items[pk].name);
}

API API::APIdropTable(const string &tname) {
	if (!cmExistTable(tname + ".table")) {
		return API("No such table");
	}
	/* just call the function */
	cmDropTable(tname + ".table");
	return API();
}

API API::APIselect(const string &tname, const Ruletree &ruletree) {
	if (!cmExistTable(tname + ".table")) {
		return API("No such table");
	}
	/* .table for table info */
	const table nowtable = cmReadTable(tname + ".table");
	/* .db for main data*/
	string dbname = tname + ".db";
	/* type check */
	for (int i = 0; i<ruletree.rules.size(); i++) {
		attribute rhs = ruletree.rules[i].rhs;
		int index = ruletree.rules[i].index;
		if (nowtable.items[index].type != rhs.type) {
			return API("Type mismatch");
		}
	}
	for (int i = 0; i<ruletree.rules.size(); i++) {
		if (ruletree.rules[i].type == 2) {
			attribute rhs = ruletree.rules[i].rhs;
			int index = ruletree.rules[i].index;
			/* use index for select */
			if (nowtable.items[index].indices.size()) {
				string btname = tname + "." + nowtable.items[index].name + ".index";
				int offset = btFindResult(btname, rhs);
				return rmSelectWithIndex(dbname, offset, ruletree, nowtable);
			}
		}
	}
	/* no condition select */
	if (ruletree.rules.size() == 0) {
		return rmSelectWithoutIndex(dbname, ruletree, nowtable);
	}

	/* find the offset of blocks */
	set<int> offset = rmGetAllOffsets(tname + ".db");
	for (int i = 0; i<ruletree.rules.size(); i++) {
		Rule rule = ruletree.rules[i];

		int attributeIndex = rule.index;
		if (!nowtable.items[attributeIndex].indices.size())
			continue;
		attribute rhs = rule.rhs;
		string btname = tname + "." + nowtable.items[attributeIndex].name + ".index";
		set <int> newset;
		/* select by condition */
		/* 0: < 1: <= 2: = 3: >= 4: > 5: != */
		switch (rule.type) {
		case 0:newset = btFindLess(btname, rhs); break;
		case 1:newset = btFindLess(btname, rhs); newset.insert(btFindResult(btname, rhs)); break;
		case 2:assert(false); break;
		case 3:newset = btFindMore(btname, rhs); newset.insert(btFindResult(btname, rhs)); break;
		case 4:newset = btFindMore(btname, rhs);
		case 5:break;
		default:assert(false);
		}
		vector <int> tmp(offset.size());
		vector <int>::iterator end = set_intersection(offset.begin(), offset.end(), newset.begin(), newset.end(), tmp.begin());
		offset.clear();
		for (vector<int>::iterator it = tmp.begin(); it != end; it++) {
			offset.insert(*it);
		}
	}
	/* get the data by offset */
	vector <vector <attribute> > res;
	for (set <int>::iterator it = offset.begin(); it != offset.end(); it++) {
		vector <vector <attribute> > tmp = rmSelectWithIndex(tname + ".db", *it, ruletree, nowtable);
		for (int i = 0; i<tmp.size(); i++) {
			res.push_back(tmp[i]);
		}
	}
	return API(res);
}


API API::APIdelete(const string &tname, const Ruletree &ruletree) {
	if (!cmExistTable(tname + ".table")) {
		return API("No such table");
	}
	const table nowtable = cmReadTable(tname + ".table");
	string dbname = tname + ".db";
	/* type check */
	for (int i = 0; i<ruletree.rules.size(); i++) {
		attribute rhs = ruletree.rules[i].rhs;
		int index = ruletree.rules[i].index;
		if (nowtable.items[index].type != rhs.type) {
			return API("Type mismatch");
		}
	}
	for (int i = 0; i<ruletree.rules.size(); i++) {
		if (ruletree.rules[i].type == 2) {
			attribute rhs = ruletree.rules[i].rhs;
			int index = ruletree.rules[i].index;
			if (nowtable.items[index].indices.size()) { // has index on it
				string btname = tname + "." + nowtable.items[index].name + ".index";
				int offset = btFindResult(btname, rhs);
				rmDeleteWithIndex(dbname, offset, ruletree, nowtable);
				return API();
			}
		}
	}
	if (ruletree.rules.size() == 0) {
		rmDeleteWithoutIndex(dbname, ruletree, nowtable);
		return API();
	}
	/* the same as select */
	set<int> offset = rmGetAllOffsets(tname + ".db");
	for (int i = 0; i<ruletree.rules.size(); i++) {
		Rule rule = ruletree.rules[i];

		int attributeIndex = rule.index;
		if (!nowtable.items[attributeIndex].indices.size())
			continue;
		attribute rhs = rule.rhs;
		string btname = tname + "." + nowtable.items[attributeIndex].name + ".index";
		set <int> newset;
		switch (rule.type) {
		case 0:newset = btFindLess(btname, rhs); break;
		case 1:newset = btFindLess(btname, rhs); newset.insert(btFindResult(btname, rhs)); break;
		case 2:assert(false); break;
		case 3:newset = btFindMore(btname, rhs); newset.insert(btFindResult(btname, rhs)); break;
		case 4:newset = btFindMore(btname, rhs);
		case 5:break;
		default:assert(false);
		}
		vector <int> tmp(offset.size());
		vector <int>::iterator end = set_intersection(offset.begin(), offset.end(), newset.begin(), newset.end(), tmp.begin());
		offset.clear();
		for (vector<int>::iterator it = tmp.begin(); it != end; it++) {
			offset.insert(*it);
		}
	}
	for (set <int>::iterator it = offset.begin(); it != offset.end(); it++) {
		rmDeleteWithIndex(tname + ".db", *it, ruletree, nowtable);
	}
	return API();
}

API API::APIinsert(const string tname, const vector<attribute> entry) {
	if (!cmExistTable(tname + ".table")) {
		return API("No such table");
	}
	const table nowtable = cmReadTable(tname + ".table");
	string dbname = tname + ".db";
	/* item count check */
	if (nowtable.items.size() != entry.size())
		return API("Type mismatch");
	/* type check */
	for (int i = 0; i<nowtable.items.size(); i++) {
		if (nowtable.items[i].type != entry[i].type) {
			return API("Type mismatch");
		}
	}
	/* unique conflit check */
	for (int i = 0; i<nowtable.items.size(); i++) {
		if (nowtable.items[i].unique) {
			Ruletree ruletree;
			ruletree.addRule(Rule(i, 2, entry[i]));
			API tmp = APIselect(tname, ruletree);
			if (tmp.result.size())
				return API("unique integrity violation");
		}
	}
	rmInsertRecord(tname + ".db", entry, nowtable);
	return API();
}
