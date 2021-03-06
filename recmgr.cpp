#include "recmgr.h"
#include <map>
#include <cstdio>
#include <cstring>
#include <string>
#include <cassert>
#include "bufmgr.h"
#include "table.h"
#include "attribute.h"

using namespace std;

recmgr::recmgr()
{
}


recmgr::~recmgr()
{
}

struct FileStatus {
	FILE *fp;
	int fileSize;
};

map <string, map <int, int> >  blockStatus;
class blockStatusSaver{
public:
	~blockStatusSaver() {
		for (map <string, map<int, int> >::iterator it = blockStatus.begin(); it != blockStatus.end(); it++) {
			string fileName = it->first;
			FILE *fp = fopen((fileName + ".blockinfo").c_str(), "w");
			assert(fp);
			for (map<int, int>::iterator i = blockStatus[fileName].begin(); i != blockStatus[fileName].end(); i++) {
				fprintf(fp, "%d %d\n", i->first, i->second);
			}
			fclose(fp);
		}
	}
}bss;
void loadBlockStatus(const string &fileName) {
	if (blockStatus.find(fileName) != blockStatus.end())//if the file doesn't exist
		return;

	FILE *fblist = fopen((fileName + ".blockinfo").c_str(), "r");
	if (fblist) {
		int offset, occ;
		while (fscanf(fblist, "%d%d", &offset, &occ) != EOF) {
			blockStatus[fileName][offset] = occ;//read the data from the file
		}
		fclose(fblist);
	}
}
bool fitInTable(const vector<attribute> &entry, const table &datatable) {
	if (entry.size() != datatable.items.size())//首先检查表和输入数据的大小是否符合
		return false;//当仅查看大小就不一致的时候，直接返回false
	for (int i = 0; i<entry.size(); i++)  {
		if (entry[i].type != datatable.items[i].type)
			return false;//查看每一项的类型是否一致，如果有一项不一致的就返回false
	}
	return true;
}
void entryToBinary(const vector<attribute> &entry, unsigned char * c, const table &datatable) {//confort the entry to the memory
	int size = datatable.size;
	memset(c, 0, size);
	for (int i = 0; i<entry.size(); i++) {
		switch (entry[i].type) {
		case 0: memcpy(c, &entry[i].datai, sizeof(entry[i].datai)); c += sizeof(entry[i].datai); break;
		case 1: memcpy(c, &entry[i].dataf, sizeof(entry[i].dataf)); c += sizeof(entry[i].dataf); break;
		case 2: memcpy(c, entry[i].datas.c_str(), entry[i].datas.length()); c += datatable.items[i].length; break;
		default:assert(false);
		}
	}
}
static int fetchint(const unsigned char *loc) {
	int temp;
	memcpy(&temp, loc, sizeof(int));
	return temp;
}
static float fetchfloat(const unsigned char *loc) {
	float temp;
	memcpy(&temp, loc, sizeof(float));
	return temp;
}
static string fetchstring(const unsigned char *loc, int len) {
	char temp[BlockSize];
	memcpy(temp, loc, len);
	temp[len] = 0;
	return temp;
}
vector <attribute> binaryToEntry(unsigned char *c, const table &datatable) {
	vector <attribute> res;
	for (int i = 0; i<datatable.items.size(); i++) {//pushback according to the type
		switch (datatable.items[i].type) {
		case 0:res.push_back(fetchint(c)); c += sizeof(int); break;
		case 1:res.push_back(fetchfloat(c)); c += sizeof(float); break;
		case 2:res.push_back(fetchstring(c, datatable.items[i].length)); c += datatable.items[i].length; break;
		default:assert(false);
		}
	}
	return res;
}
int recmgr::rmInsertRecord(const string &fileName, const vector<attribute> &entry, const table &datatable) { // return the offset of data inserted
	if (!fitInTable(entry, datatable)) {
		return -1;
	}

	loadBlockStatus(fileName);

	int capacity = BlockSize / (datatable.size + 1);
	int offset = -1;
	for (map<int, int>::iterator it = blockStatus[fileName].begin(); it != blockStatus[fileName].end(); it++) {
		if (it->second<capacity) {
			offset = it->first;
			break;
		}
	}
	Block block;
	if (offset == -1) {

		block = bmnewBlock(fileName);
		offset = block.offset;
		blockStatus[fileName][offset] = 0;
		memset(block.data, 0, sizeof(block.data));
	}
	else {
		block = bmreadBlock(fileName, offset);
	}
	bool findd = false;
	for (int i = 0; i<capacity; i++) {
		if (!block.data[i]) {
			entryToBinary(entry, block.data + capacity + i*datatable.size, datatable);
			findd = true;
			block.data[i] = true;
			break;
		}
	}
	for (int i = 0; i<datatable.items.size(); i++) {
		if (datatable.items[i].indices.size()) { // has index on it
			string tablename = datatable.name;
			while (*(tablename.end() - 1) != '.')
				tablename.erase(tablename.end() - 1);
			tablename.erase(tablename.end() - 1);
			string btname = tablename + "." + datatable.items[i].name + ".index";
			assert(!btExist(btname, entry[i]));
			btInsert(btname, entry[i], offset);
		}
	}
	assert(findd);
	bmwriteBlock(block);
	blockStatus[fileName][offset]++;
	return offset;
}
void recmgr::rmDeleteWithIndex(const string fileName, int offset, const Ruletree &ruletree, const table &datatable) {
	loadBlockStatus(fileName);

	Block block = bmreadBlock(fileName, offset);
	int capacity = BlockSize / (datatable.size + 1);

	unsigned char *c = block.data + capacity;
	for (int i = 0; i<capacity; i++) {
		if (block.data[i]) {
			vector <attribute> entry = binaryToEntry(c, datatable);
			if (ruletree.test(entry)) {
				for (int j = 0; j<datatable.items.size(); j++) {
					if (datatable.items[j].indices.size()) {
						string tablename = datatable.name;
						while (*(tablename.end() - 1) != '.')
							tablename.erase(tablename.end() - 1);
						tablename.erase(tablename.end() - 1);
						string btname = tablename + "." + datatable.items[j].name + ".index";
						assert(btExist(btname, entry[j]));
						btDelete(btname, entry[j]);
					}
				}
				block.data[i] = false;
				blockStatus[fileName][offset]--;
			}
		}
		c += datatable.size;
	}
	assert(blockStatus[fileName][offset] >= 0);

	bmwriteBlock(block);
}


void recmgr::rmDeleteWithoutIndex(const string fileName, const Ruletree &ruletree, const table &datatable) {
	loadBlockStatus(fileName);
	for (map <int, int> ::iterator it = blockStatus[fileName].begin(); it != blockStatus[fileName].end(); it++) {
		int offset = it->first;
		if (it->second == 0)
			continue;
		Block block = bmreadBlock(fileName, offset);
		int capacity = BlockSize / (datatable.size + 1);

		unsigned char *c = block.data + capacity;
		for (int i = 0; i<capacity; i++) {
			if (block.data[i]) {
				vector <attribute> entry = binaryToEntry(c, datatable);
				if (ruletree.test(entry)) {
					for (int j = 0; j<datatable.items.size(); j++) {
						if (datatable.items[j].indices.size()) {
							string tablename = datatable.name;
							while (*(tablename.end() - 1) != '.')
								tablename.erase(tablename.end() - 1);
							tablename.erase(tablename.end() - 1);
							string btname = tablename + "." + datatable.items[j].name + ".index";
							assert(btExist(btname, entry[j]));
							btDelete(btname, entry[j]);
						}
					}

					block.data[i] = false;
					blockStatus[fileName][offset]--;
				}
			}
			c += datatable.size;
		}
		assert(blockStatus[fileName][offset] >= 0);

		bmwriteBlock(block);
	}
}
vector <vector <attribute> > recmgr::rmSelectWithIndex(const string fileName, int offset, const Ruletree &ruletree, const table &datatable) {
	vector <vector <attribute> > temp;
	loadBlockStatus(fileName);

	Block block = bmreadBlock(fileName, offset);
	int capacity = BlockSize / (datatable.size + 1);

	unsigned char *c = block.data + capacity;
	for (int i = 0; i<capacity; i++) {
		if (block.data[i]) {
			vector <attribute> entry = binaryToEntry(c, datatable);
			if (ruletree.test(entry)) {
				temp.push_back(entry);
			}
		}
		c += datatable.size;
	}
	return temp;
}

vector <vector <attribute> > recmgr::rmSelectWithoutIndex(const string fileName, const Ruletree &ruletree, const table &datatable) {
	vector <vector <attribute> > temp;
	loadBlockStatus(fileName);

	for (map <int, int> ::iterator it = blockStatus[fileName].begin(); it != blockStatus[fileName].end(); it++) {
		int offset = it->first;
		if (it->second == 0)
			continue;
		Block block = bmreadBlock(fileName, offset);
		int capacity = BlockSize / (datatable.size + 1);

		unsigned char *c = block.data + capacity;
		for (int i = 0; i<capacity; i++) {
			if (block.data[i]) {
				vector <attribute> entry = binaryToEntry(c, datatable);
				if (ruletree.test(entry)) {
					temp.push_back(entry);
				}
			}
			c += datatable.size;
		}
	}
	return temp;
}


void recmgr::rmAddIndex(const string dbName, const string BTreeName, const table &datatable, int itemIndex) {
	btCreate(BTreeName, datatable.items[itemIndex].type, datatable.items[itemIndex].length);
	loadBlockStatus(dbName);

	for (map <int, int> ::iterator it = blockStatus[dbName].begin(); it != blockStatus[dbName].end(); it++) {
		int offset = it->first;
		if (it->second == 0)
			continue;
		Block block = bmreadBlock(dbName, offset);
		int capacity = BlockSize / (datatable.size + 1);

		unsigned char *c = block.data + capacity;
		for (int i = 0; i<capacity; i++) {
			if (block.data[i]) {
				vector <attribute> entry = binaryToEntry(c, datatable);
				assert(itemIndex<entry.size());
				assert(!btExist(BTreeName, entry[itemIndex]));
				btInsert(BTreeName, entry[itemIndex], offset);
			}
			c += datatable.size;
		}
	}
}

set<int> recmgr::rmGetAllOffsets(const string &fileName) {
	loadBlockStatus(fileName);
	set <int> temp;
	for (map<int, int>::iterator it = blockStatus[fileName].begin(); it != blockStatus[fileName].end(); it++) {
		temp.insert(it->first);
	}
	return temp;

}
void recmgr::rmClear(const string fileName) {
	if (blockStatus.find(fileName) != blockStatus.end())
		blockStatus.erase(blockStatus.find(fileName));
	bmclear(fileName);
	remove((fileName + ".blockinfo").c_str());
}
