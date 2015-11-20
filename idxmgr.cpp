#include "idxmgr.h"

/* type transformation */
int charToInt(const unsigned char *loc) {
	int temp;
	memcpy(&temp, loc, sizeof(int));
	return temp;
}
float charToFloat(const unsigned char *loc) {
	float temp;
	memcpy(&temp, loc, sizeof(float));
	return temp;
}
string charToString(const unsigned char *loc, int len) {
	char temp[BlockSize];
	memcpy(temp, loc, len);
	temp[len] = 0;
	return temp;
}

/* convert node in btree to binary blocks */
void btree::convertBinary(const node &n, unsigned char b[BlockSize], btHeader bth) {
	/* b[0] saves isRoot, b[1] saves isLeaf */
	memset(b, 0, BlockSize);
	b[0] = n.isRoot;
	b[1] = n.isLeaf;

	int num1 = n.sons.size();
	int num2 = n.data.size();
	int null = 0;
	//assert(num1 <= bth.fanOut);
	//assert(num2<bth.fanOut);

	/* b[2:5] saves son size, b[6:9] saves data size
	4 byte each one								*/
	memcpy(&b[2], &num1, sizeof(num1));
	memcpy(&b[6], &num2, sizeof(num2));
	int offset = 10;
	/* list of sons */
	for (int i = 0; i<num1; i++) {
		memcpy(&b[offset], &n.sons[i], sizeof(n.sons[i]));
		offset += sizeof(n.sons[i]);
	}
	/* fullfill the list to fanout */
	for (int i = num1; i<bth.fanOut; i++) {
		memcpy(&b[offset], &null, sizeof(null));
		offset += sizeof(null);
	}
	/* save data by type */
	for (int i = 0; i<num2; i++) {
		assert(n.data[i].type == bth.type);
		if (bth.type == 0) {
			memcpy(&b[offset], &n.data[i].datai, sizeof(int));
			offset += sizeof(int);
		}
		else if (bth.type == 1){
			memcpy(&b[offset], &n.data[i].dataf, sizeof(float));
			offset += sizeof(float);
		}
		else if (bth.type == 2){
			assert(n.data[i].datas.length() <= bth.length);
			memcpy(&b[offset], n.data[i].datas.c_str(), n.data[i].datas.length());
			offset += bth.length;
		}
		else
			assert(false);
	}

}

/* convert binary array to node in btree */
node convertNode(const unsigned char *b, btHeader bth) {
	node temp;
	/* b[0] saves isRoot, b[1] saves isLeaf */
	int num1 = charToInt(b + 2);
	int num2 = charToInt(b + 6);
	temp.isRoot = b[0];
	temp.isLeaf = b[1];

	//assert(num1 <= bth.fanOut);
	//assert(num2<bth.fanOut);
	/* b[2:5] saves son size, b[6:9] saves data size
	4 byte each one								*/
	int offset = 10;
	temp.sons.clear();
	/* list of sons */
	for (int i = 0; i<num1; i++) {
		temp.sons.push_back(charToInt(b + offset));
		offset += 4;
	}
	/* fullfill the list to fanout */
	offset = 10 + (bth.fanOut) * 4;
	/* save data by type */
	for (int i = 0; i<num2; i++) {
		if (bth.type == 0){
			temp.data.push_back(charToInt(b + offset));
			offset += sizeof(int);
		}
		else if (bth.type == 1){
			temp.data.push_back(charToFloat(b + offset));
			offset += sizeof(float);
		}
		else if (bth.type == 2){
			temp.data.push_back(charToString(b + offset, bth.length));
			offset += bth.length;
		}
		else assert(false);
	}
	return temp;

}


void btree::btCreate(const string &fname, int type, int length = -1){
	assert(type != 2 || length != -1);
	btHeader bth(type, length, BlockSize);
	/* calculate the fanout */
	if (type == 0)
		bth.fanOut = (BlockSize - sizeof(bool) * 2 - sizeof(int) * 3 - sizeof(int)) / (sizeof(int) + sizeof(int));
	else if (type == 1)
		bth.fanOut = (BlockSize - sizeof(bool) * 2 - sizeof(int) * 3 - sizeof(int)) / (sizeof(int) + sizeof(float));
	else if (type == 2)
		bth.fanOut = (BlockSize - sizeof(bool) * 2 - sizeof(int) * 3 - sizeof(int)) / (sizeof(int) + length);
	bth.fanOut++;
	/* create a block for new btreeHeader */
	Block b = bmnewBlock(fname);
	assert(b.offset == 0);
	memset(b.data, 0, sizeof(b.data));
	memcpy(b.data, &bth, sizeof(btHeader));
	bmwriteBlock(b);

	/* create root node */
	b = bmnewBlock(fname);
	node Root(true, true);
	Root.sons.push_back(-1);
	/* transform root to binary and save */
	convertBinary(Root, b.data, bth);
	bmwriteBlock(b);
}

/* get node by offset */
node btree::getNode(const string &fname, int offset, const btHeader &bth){
	Block block = bmreadBlock(fname, offset);
	return convertNode(block.data, bth);
}
/* find data key on node in offset */
int btree::btFind(const string &fname, int offset, const attribute& key, btHeader bth){
	node now = getNode(fname, offset, bth);
	if (now.isRoot) path.clear();
	path.push_back(offset);
	if (now.isLeaf) return offset;
	/* recursion search */
	for (int i = 0; i < now.data.size(); i++){
		if (now.data[i] >= key)
			return btFind(fname, now.sons[i], key, bth);
	}
	return btFind(fname, now.sons[now.sons.size() - 1], key, bth);
}
/* find data main function */
int btree::btFindResult(const string& fname, const attribute& key){
	btHeader bth;
	Block block = bmreadBlock(fname, 0);
	memcpy(&bth, block.data, sizeof(bth));

	assert(bth.type == key.type);
	int offset = btFind(fname, bth.rootOffset, key, bth);
	node now = getNode(fname, offset, bth);
	for (int i = 0; i<now.data.size(); i++) {
		if (now.data[i] >= key) {
			return now.sons[i];
		}
	}
	int next = *(now.sons.end() - 1);
	if (next == -1) {
		return -1;
	}
	node nnode = getNode(fname, next, bth);
	return nnode.sons[0];
}

/* save btreeheader */
void btree::saveHeader(const btHeader &bth, const string &fname) {
	Block block = bmreadBlock(fname, 0);
	memset(block.data, 0, BlockSize);
	memcpy(block.data, &bth, sizeof(bth));
	bmwriteBlock(block);
}
/* insert a new node to btree */
void btree::insertIntoParent(Block b, const attribute& keyp, Block bp, const string &fname, btHeader bth) {
	node N = convertNode(b.data, bth);
	node NP = convertNode(bp.data, bth);
	/* not a root */
	if (!N.isRoot) {
		Block BP = bmreadBlock(fname, *(path.end() - 2));
		node P = convertNode(BP.data, bth);
		int n = bth.fanOut;
		assert(P.sons.size() == P.data.size() + 1);
		/* if parent size < fanout, insert directly */
		if (P.sons.size()<n) {
			bool findd = false;
			for (int i = 0; i<P.sons.size(); i++) {
				if (P.sons[i] == b.offset) {
					P.sons.insert(P.sons.begin() + i + 1, bp.offset);
					P.data.insert(P.data.begin() + i, keyp);
					convertBinary(P, BP.data, bth);
					bmwriteBlock(BP);
					findd = true;
					return;
				}
			}
			assert(findd);
		}
		/* parent size is not enough */
		else {
			/* copy parent node to T */
			node T = P;
			bool findd = false;
			/* first find the position and insert */
			for (int i = 0; i<T.sons.size(); i++) {
				if (T.sons[i] == b.offset) {
					T.sons.insert(T.sons.begin() + i + 1, bp.offset);
					T.data.insert(T.data.begin() + i, keyp);
					findd = true;
					break;
				}
			}
			assert(findd);
			assert(T.sons.size() == n + 1);

			P.sons.clear();
			P.data.clear();
			Block BPP = bmnewBlock(fname);
			node PP;
			PP.isLeaf = false;
			PP.isRoot = false;
			/* invide new parent node to two parts */
			for (int i = 0; i<(n + 1) / 2; i++) {
				P.sons.push_back(T.sons[i]);
				if (i != (n + 1) / 2 - 1)
					P.data.push_back(T.data[i]);
			}
			attribute KPP = T.data[(n + 1) / 2 - 1];
			for (int i = (n + 1) / 2; i <= n; i++) {
				PP.sons.push_back(T.sons[i]);
				if (i != n)
					PP.data.push_back(T.data[i]);
			}
			/* save the parts */
			convertBinary(P, BP.data, bth);
			convertBinary(PP, BPP.data, bth);
			bmwriteBlock(BP);
			bmwriteBlock(BPP);
			path.pop_back();
			/* insert new node */
			insertIntoParent(BP, KPP, BPP, fname, bth);
		}
	}
	/* if it's root */
	else {
		Block newblock = bmnewBlock(fname);
		node newnode(true, false);
		newnode.sons.push_back(b.offset);
		newnode.sons.push_back(bp.offset);
		newnode.data.push_back(keyp);
		N.isRoot = false;
		/* store root to btreeheader */
		bth.rootOffset = newblock.offset;
		saveHeader(bth, fname);
		convertBinary(newnode, newblock.data, bth);
		bmwriteBlock(newblock);
		convertBinary(N, b.data, bth);
		bmwriteBlock(b);

	}
}
/* insert main function */
void btree::btInsert(const string &fname, const attribute &key, int value) {
	/* load btreeheader*/
	btHeader bth;
	Block block = bmreadBlock(fname, 0);
	memcpy(&bth, block.data, sizeof(bth));
	assert(bth.type == key.type);

	/* find the position of key */
	int offset = btFind(fname, bth.rootOffset, key, bth);
	block = bmreadBlock(fname, offset);
	node now = convertNode(block.data, bth);

	/* if node size is enough
	then insert directly   */
	if (now.data.size()<bth.fanOut - 1) {
		for (int i = 0; i<now.data.size(); i++) {
			if (now.data[i]>key) {
				now.data.insert(now.data.begin() + i, key);
				now.sons.insert(now.sons.begin() + i, value);
				convertBinary(now, block.data, bth);
				bmwriteBlock(block);
				return;
			}
		}
		now.data.push_back(key);
		now.sons.insert(now.sons.end() - 1, value);
		convertBinary(now, block.data, bth);
		bmwriteBlock(block);

	}
	/* node size is full */
	else {
		/* create new node copy from node searched before */
		Block newblock = bmnewBlock(fname);
		node tmp = now;
		int n = bth.fanOut;
		bool findd = false;
		/* insert data to its position */
		for (int i = 0; i<tmp.data.size(); i++) {
			if (tmp.data[i]>key) {
				tmp.data.insert(tmp.data.begin() + i, key);
				tmp.sons.insert(tmp.sons.begin() + i, value);
				findd = true;
				break;
			}
		}
		if (!findd) {
			tmp.data.push_back(key);
			tmp.sons.insert(tmp.sons.end() - 1, value);
		}



		node newnode;
		assert(now.sons.size() == n);
		int orgnext = now.sons[n - 1];

		/* invide the node to 2 part */
		now.sons.clear();
		now.data.clear();
		for (int i = 0; i<(n + 1) / 2; i++) {
			now.sons.push_back(tmp.sons[i]);
			now.data.push_back(tmp.data[i]);
		}
		now.sons.push_back(newblock.offset);
		/* another node */
		newnode.isLeaf = true;
		newnode.isRoot = false;
		for (int i = (n + 1) / 2; i<n; i++) {
			newnode.sons.push_back(tmp.sons[i]);
			newnode.data.push_back(tmp.data[i]);
		}
		newnode.sons.push_back(orgnext);

		/* save to node to block */
		convertBinary(now, block.data, bth);
		bmwriteBlock(block);


		convertBinary(newnode, newblock.data, bth);
		bmwriteBlock(newblock);

		attribute keyp = newnode.data[0];
		/* insert new node to btree */
		insertIntoParent(block, keyp, newblock, fname, bth);
	}
}

void btree::deleteEntry(Block BN, attribute K, int P, const string& fileName, btHeader bth) {
	node N = convertNode(BN.data, bth);
	const int n = bth.fanOut;
	/* delete in sons and data vector */
	for (int i = 0; i<N.data.size(); i++) {
		if (N.data[i] == K) {
			N.data.erase(N.data.begin() + i);
			break;
		}
	}
	for (int i = 0; i<N.sons.size(); i++) {
		if (N.sons[i] == P) {
			N.sons.erase(N.sons.begin() + i);
			break;
		}
	}
	/* isroot */
	if (N.isRoot && N.sons.size() == 1) {

		if (N.sons[0] == -1) {
			/* have no son */
			convertBinary(N, BN.data, bth);
			bmwriteBlock(BN);
			return;
		}
		/* create a new node */
		Block BNewRoot = bmreadBlock(fileName, N.sons[0]);
		node newRoot = convertNode(BNewRoot.data, bth);
		newRoot.isRoot = true;
		convertBinary(newRoot, BNewRoot.data, bth);
		bmwriteBlock(BNewRoot);
		/* release the old one */
		bmreleaseBlock(fileName, BN.offset);
		/* updata btreeheader info */
		bth.rootOffset = BNewRoot.offset;
		saveHeader(bth, fileName);
		return;
	}
	/* is not root */
	else {

		convertBinary(N, BN.data, bth);
		bmwriteBlock(BN);

		assert(N.sons.size() == N.data.size() + 1);
		/* size < fanout */
		if (!N.isRoot && ((N.isLeaf && N.data.size()<n / 2) || (!N.isLeaf && N.sons.size()<(n + 1) / 2))) {
			assert(path.size() >= 2);

			int parentoffset = *(path.end() - 2);

			Block BParent = bmreadBlock(fileName, parentoffset);
			node parent = convertNode(BParent.data, bth);
			Block BNP;
			attribute KP;
			bool Nisfront;
			bool findd = false;

			/* get the position of key
			and store other data    */
			for (int i = 0; i<parent.sons.size(); i++) {
				if (parent.sons[i] == BN.offset) {
					findd = true;
					if (i == parent.sons.size() - 1) {
						BNP = bmreadBlock(fileName, parent.sons[i - 1]);
						KP = parent.data[i - 1];
						Nisfront = false;
					}
					else {
						BNP = bmreadBlock(fileName, parent.sons[i + 1]);
						KP = parent.data[i];
						Nisfront = true;
					}
				}
			}
			node NP = convertNode(BNP.data, bth);
			assert(findd);
			/* union parent and son */
			if (N.sons.size() + NP.sons.size() <= n) {
				if (Nisfront) {
					swap(N, NP);
					swap(BN, BNP);
				}
				if (!N.isLeaf) {
					/* leaf save data */
					NP.data.push_back(KP);
					for (int i = 0; i<N.data.size(); i++)
						NP.data.push_back(N.data[i]);
					for (int i = 0; i<N.sons.size(); i++)
						NP.sons.push_back(N.sons[i]);
				}
				else {
					NP.sons.pop_back();
					for (int i = 0; i<N.data.size(); i++)
						NP.data.push_back(N.data[i]);
					for (int i = 0; i<N.sons.size(); i++)
						NP.sons.push_back(N.sons[i]);
				}
				convertBinary(N, BN.data, bth);
				convertBinary(NP, BNP.data, bth);
				bmwriteBlock(BN);
				bmwriteBlock(BNP);
				path.pop_back();
				deleteEntry(BParent, KP, BN.offset, fileName, bth);
				bmreleaseBlock(fileName, BN.offset);
			}
			else {
				/* cannot union */
				if (!Nisfront) {
					if (!N.isLeaf) {
						/* add a son from parent */
						int nppm = *(NP.sons.end() - 1);
						attribute nkmm1 = *(NP.data.end() - 1);
						NP.sons.pop_back();
						NP.data.pop_back();
						N.sons.insert(N.sons.begin(), nppm);
						N.data.insert(N.data.begin(), KP);
						bool findd = false;
						/* save data in parent */
						for (int i = 0; i<parent.data.size(); i++) {
							if (parent.data[i] == KP) {
								findd = true;
								parent.data[i] = nkmm1;
								break;
							}
						}
						assert(findd);
						convertBinary(parent, BParent.data, bth);
						bmwriteBlock(BParent);
						convertBinary(N, BN.data, bth);
						bmwriteBlock(BN);
						convertBinary(NP, BNP.data, bth);
						bmwriteBlock(BNP);
					}
					else {

						int m = NP.data.size() - 1;
						attribute npkm = *(NP.data.end() - 1);

						N.data.insert(N.data.begin(), NP.data[m]);
						N.sons.insert(N.sons.begin(), NP.sons[m]);
						NP.data.erase(NP.data.begin() + m);
						NP.sons.erase(NP.sons.begin() + m);
						bool findd = false;
						for (int i = 0; i<parent.data.size(); i++) {
							if (parent.data[i] == KP) {
								findd = true;
								parent.data[i] = npkm;
							}
						}
						assert(findd);
						convertBinary(parent, BParent.data, bth);
						bmwriteBlock(BParent);
						convertBinary(N, BN.data, bth);
						bmwriteBlock(BN);
						convertBinary(NP, BNP.data, bth);
						bmwriteBlock(BNP);
					}
				}
				else  {
					if (!N.isLeaf) {
						/* N is at begin */
						int nppm = *(NP.sons.begin());
						attribute nkmm1 = *(NP.data.begin());
						NP.sons.erase(NP.sons.begin());
						NP.data.erase(NP.data.begin());
						N.sons.push_back(nppm);
						N.data.push_back(KP);
						bool findd = false;
						for (int i = 0; i<parent.data.size(); i++) {
							if (parent.data[i] == KP) {
								findd = true;
								parent.data[i] = nkmm1;
								break;
							}
						}
						assert(findd);
						convertBinary(parent, BParent.data, bth);
						bmwriteBlock(BParent);
						convertBinary(N, BN.data, bth);
						bmwriteBlock(BN);
						convertBinary(NP, BNP.data, bth);
						bmwriteBlock(BNP);
					}
					else {
						int m = N.sons.size() - 1;
						N.data.push_back(NP.data[0]);
						N.sons.insert(N.sons.begin() + m, NP.sons[0]);
						NP.data.erase(NP.data.begin());
						NP.sons.erase(NP.sons.begin());
						bool findd = false;
						for (int i = 0; i<parent.data.size(); i++) {
							if (parent.data[i] == KP) {
								findd = true;
								parent.data[i] = NP.data[0];
							}
						}
						assert(findd);
						convertBinary(parent, BParent.data, bth);
						bmwriteBlock(BParent);
						convertBinary(N, BN.data, bth);
						bmwriteBlock(BN);
						convertBinary(NP, BNP.data, bth);
						bmwriteBlock(BNP);
					}
				}
			}
		}
		else {
		}
	}
}
/* delete main function */
bool btree::btDelete(const string &fileName, const attribute &key) {

	btHeader bth;
	Block block = bmreadBlock(fileName, 0);
	memcpy(&bth, block.data, sizeof(bth));
	assert(bth.type == key.type);
	/* find key value */
	int offset = btFind(fileName, bth.rootOffset, key, bth);
	block = bmreadBlock(fileName, offset);
	node now = convertNode(block.data, bth);
	bool findd = false;
	attribute K;
	int P;
	for (int i = 0; i<now.data.size(); i++) {
		if (now.data[i] == key) {
			findd = true;
			K = key;
			P = now.sons[i];
		}
	}
	assert(findd);
	/* not found */
	if (!findd)
		return false;
	/* found */
	deleteEntry(block, K, P, fileName, bth);
	return true;
}
/* find attribute existing */
bool btree::btExist(const string &fileName, const attribute &key) {

	btHeader bth;
	Block block = bmreadBlock(fileName, 0);
	memcpy(&bth, block.data, sizeof(bth));
	assert(bth.type == key.type);
	/* find offset near key */
	int offset = btFind(fileName, bth.rootOffset, key, bth);
	block = bmreadBlock(fileName, offset);
	node now = convertNode(block.data, bth);
	bool findd = false;
	attribute K;
	int P;
	/* search in sons */
	for (int i = 0; i<now.data.size(); i++) {
		if (now.data[i] == key) {
			findd = true;
			K = key;
			P = now.sons[i];
		}
	}
	return findd;
}
/* find less than key and return a set */
set <int> btree::btFindLess(const string &fileName, const attribute &key) {
	btHeader bth;
	Block block = bmreadBlock(fileName, 0);
	memcpy(&bth, block.data, sizeof(bth));
	if (bth.type != key.type) {
		printf("%s %d %d\n", fileName.c_str(), bth.type, key.type);
	}
	assert(bth.type == key.type);

	int offset = bth.rootOffset;
	/* get the beginning of data */
	while (1) {
		node now = getNode(fileName, offset, bth);
		if (now.isLeaf)
			break;
		offset = now.sons[0];
	}
	/* find the first value > key by order
	and add data into set */
	set <int> ret;
	while (offset != -1) {
		node now = getNode(fileName, offset, bth);
		bool findd = false;
		for (int i = 0; i<now.data.size(); i++) {
			if (now.data[i]<key) {
				ret.insert(now.sons[i]);
			}
			else {
				findd = true;
				break;
			}
		}
		if (findd)
			break;
		offset = now.sons[now.sons.size() - 1];
	}
	return ret;
}
/* find more function */
set <int> btree::btFindMore(const string &fileName, const attribute &key) {
	btHeader bth;
	Block block = bmreadBlock(fileName, 0);
	memcpy(&bth, block.data, sizeof(bth));
	assert(bth.type == key.type);

	int offset = btFind(fileName, bth.rootOffset, key, bth);;
	/* get the first value >= key
	add data after it to set   */
	set <int> ret;
	while (offset != -1) {
		node now = getNode(fileName, offset, bth);
		for (int i = 0; i<now.data.size(); i++) {
			if (now.data[i]>key) {
				ret.insert(now.sons[i]);
			}
		}
		offset = now.sons[now.sons.size() - 1];
	}
	return ret;
}
/* index exsisting */
bool idxmgr::ixExistIndex(const string &indexname) {
	return indexToTable.find(indexname) != indexToTable.end();
}
/* create a new index */
bool idxmgr::ixCreateIndex(const string &tablename, const string &indexname, int itemIndex) {
	if (indexToTable.find(indexname) != indexToTable.end())
		return false;
	indexToTable[indexname] = make_pair(tablename, itemIndex);
	return true;
}
/* get index's file and offset */
pair<string, int> idxmgr::ixAskIndex(const string indexName) {
	assert(ixExistIndex(indexName));
	return indexToTable[indexName];
}
/* delete a index */
bool idxmgr::ixDeleteIndex(const string indexName) {
	if (indexToTable.find(indexName) == indexToTable.end())
		return false;
	indexToTable.erase(indexToTable.find(indexName));
	return true;
}