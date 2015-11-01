#include "btree.h"
void nodeToBinary(const node &n, unsigned char b[BlockSize], btreeHeader bth) {
	memset(b, 0, BlockSize);
	b[0] = n.isRoot;
	b[1] = n.isLeaf;

	int numSons = n.sons.size();
	int numData = n.data.size();
	assert(numSons <= bth.fanOut);
	assert(numData<bth.fanOut);

	memcpy(&b[2], &numSons, sizeof(numSons));
	memcpy(&b[6], &numData, sizeof(numData));
	int offset = 10;

	for (int i = 0; i<numSons; i++) {
		memcpy(&b[offset], &n.sons[i], sizeof(n.sons[i]));
		offset += sizeof(n.sons[i]);
	}
	int null = 0;
	for (int i = numSons; i<bth.fanOut; i++) {
		memcpy(&b[offset], &null, sizeof(null));
		offset += sizeof(null);
	}

	for (int i = 0; i<numData; i++) {
		assert(n.data[i].type == bth.type);
		switch (bth.type) {
		case 0:
			memcpy(&b[offset], &n.data[i].datai, sizeof(int));
			offset += sizeof(int);
			break;
		case 1:
			memcpy(&b[offset], &n.data[i].dataf, sizeof(float));
			offset += sizeof(float);
			break;
		case 2:
			assert(n.data[i].datas.length() <= bth.length);
			memcpy(&b[offset], n.data[i].datas.c_str(), n.data[i].datas.length());
			offset += bth.length;
			break;
		default:
			assert(false);
		}
	}

}

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

node binaryToNode(const unsigned char *b, btreeHeader bth) {
	node temp;
	temp.isRoot = b[0];
	temp.isLeaf = b[1];

	int numSons = charToInt(b + 2);
	int numData = charToInt(b + 6);

	assert(numSons <= bth.fanOut);
	assert(numData<bth.fanOut);


	int offset = 10;
	temp.sons.clear();
	for (int i = 0; i<numSons; i++) {
		temp.sons.push_back(charToInt(b + offset));
		offset += 4;
	}
	offset = 10 + (bth.fanOut) * 4;

	for (int i = 0; i<numData; i++) {
		switch (bth.type) {
		case 0:
			temp.data.push_back(charToInt(b + offset));
			offset += sizeof(int);
			break;
		case 1:
			temp.data.push_back(charToFloat(b + offset));
			offset += sizeof(float);
			break;
		case 2:
			temp.data.push_back(charToString(b + offset, bth.length));
			offset += bth.length;
			break;
		default:
			assert(false);
		}
	}
	return temp;

}


void btree::btCreate(const string &fname,int type,int length=-1){
	assert(type != 2 || length != -1);
	btreeHeader bth;
	bth.type = type;
	bth.length = length;
	bth.rootOffset = BlockSize;
	switch (type){
	case 0: bth.fanOut = (BlockSize - sizeof(bool) * 2 - sizeof(int) * 3 - sizeof(int)) / (sizeof(int) + sizeof(int)); break;
	case 1: bth.fanOut = (BlockSize - sizeof(bool) * 2 - sizeof(int) * 3 - sizeof(int)) / (sizeof(int) + sizeof(float)); break;
	case 2: bth.fanOut = (BlockSize - sizeof(bool) * 2 - sizeof(int) * 3 - sizeof(int)) / (sizeof(int) + length); break;
	default: assert(false);
	}
	bth.fanOut++;
	Block b = bmnewBlock(fname);
	assert(b.offset == 0);
	memset(b.data, 0, sizeof(b.data));
	memcpy(b.data, &bth, sizeof(btreeHeader));
	bmwriteBlock(b);


	b = bmnewBlock(fname);
	node Root;
	Root.isLeaf = true;
	Root.isRoot = true;
	Root.sons.push_back(-1);

	nodeToBinary(Root, b.data, bth);
	bmwriteBlock(b);
}


node btree::getNode(const string &fname, int offset, const btreeHeader &bth){
	Block block = bmreadBlock(fname, offset);
	return binaryToNode(block.data, bth);
}

int btree::btFind(const string &fname, int offset, const attribute& key, btreeHeader bth){
	node now = getNode(fname, offset, bth);
	if (now.isRoot) path.clear();
	path.push_back(offset);
	if (now.isLeaf) return offset;
	for (int i = 0; i < now.data.size(); i++){
		if (now.data[i] >= key)
			return btFind(fname, now.sons[i], key, bth);
	}
	return btFind(fname, now.sons[now.sons.size() - 1], key, bth);
}

int btree::btFindResult(const string& fname, const attribute& key){
	btreeHeader bth;
	Block block = bmreadBlock(fname, 0);
	memcpy(&bth, block.data, sizeof(bth));

	assert(bth.type == key.type);

	return btFind(fname, bth.rootOffset, key, bth);
	//?
}

void btree::saveHeader(const btreeHeader &bth, const string &fname) {
	Block block = bmreadBlock(fname, 0);
	memset(block.data, 0, BlockSize);
	memcpy(block.data, &bth, sizeof(bth));
	bmwriteBlock(block);
}

void btree::insertIntoParent(Block b, const attribute& keyp, Block bp, const string &fname, btreeHeader bth) {
	node N = binaryToNode(b.data, bth);
	node NP = binaryToNode(bp.data, bth);
	if (N.isRoot) {
		Block newblock = bmnewBlock(fname);
		node newnode;
		newnode.isRoot = true;
		newnode.isLeaf = false;
		newnode.sons.push_back(b.offset);
		newnode.sons.push_back(bp.offset);
		newnode.data.push_back(keyp);
		N.isRoot = false;


		bth.rootOffset = newblock.offset;
		saveHeader(bth, fname);

		nodeToBinary(newnode, newblock.data, bth);
		bmwriteBlock(newblock);

		nodeToBinary(N, b.data, bth);
		bmwriteBlock(b);
	}
	else {
		int n = bth.fanOut;
		Block BP = bmreadBlock(fname, *(path.end() - 2));
		node P = binaryToNode(BP.data, bth);
		assert(P.sons.size() == P.data.size() + 1);
		if (P.sons.size()<n) {
			bool findd = false;
			for (int i = 0; i<P.sons.size(); i++) {
				if (P.sons[i] == b.offset) {
					P.sons.insert(P.sons.begin() + i + 1, bp.offset);
					P.data.insert(P.data.begin() + i, keyp);
					nodeToBinary(P, BP.data, bth);
					bmwriteBlock(BP);
					findd = true;
					return;
				}
			}
			assert(findd);
		}
		else {
			node T = P;
			bool findd = false;

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
			nodeToBinary(P, BP.data, bth);
			nodeToBinary(PP, BPP.data, bth);
			bmwriteBlock(BP);
			bmwriteBlock(BPP);
			path.pop_back();
			insertIntoParent(BP, KPP, BPP, fname, bth);
		}
	}
}

void btree::btInsert(const string &fname, const attribute &key, int value) {
	btreeHeader bth;
	Block block = bmreadBlock(fname, 0);
	memcpy(&bth, block.data, sizeof(bth));
	assert(bth.type == key.type);

	int offset = btFind(fname, bth.rootOffset, key, bth);
	block = bmreadBlock(fname, offset);
	node now = binaryToNode(block.data, bth);

	if (now.data.size()<bth.fanOut - 1) {
		for (int i = 0; i<now.data.size(); i++) {
			if (now.data[i]>key) {
				now.data.insert(now.data.begin() + i, key);
				now.sons.insert(now.sons.begin() + i, value);
				nodeToBinary(now, block.data, bth);
				bmwriteBlock(block);
				return;
			}
		}
		now.data.push_back(key);
		now.sons.insert(now.sons.end() - 1, value);
		nodeToBinary(now, block.data, bth);
		bmwriteBlock(block);

	}
	else {
		Block newblock = bmnewBlock(fname);
		node tmp = now;
		int n = bth.fanOut;
		bool findd = false;
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


		now.sons.clear();
		now.data.clear();
		for (int i = 0; i<(n + 1) / 2; i++) {
			now.sons.push_back(tmp.sons[i]);
			now.data.push_back(tmp.data[i]);
		}
		now.sons.push_back(newblock.offset);
		newnode.isLeaf = true;
		newnode.isRoot = false;
		for (int i = (n + 1) / 2; i<n; i++) {
			newnode.sons.push_back(tmp.sons[i]);
			newnode.data.push_back(tmp.data[i]);
		}
		newnode.sons.push_back(orgnext);


		nodeToBinary(now, block.data, bth);
		bmwriteBlock(block);


		nodeToBinary(newnode, newblock.data, bth);
		bmwriteBlock(newblock);

		attribute keyp = newnode.data[0];
		insertIntoParent(block, keyp, newblock, fname, bth);
	}
}

void btree::seetree(const string &fname, int offset, btreeHeader bth) {
	node now = getNode(fname, offset, bth);
	printf("ID=%d\n", offset);
	printf("pointers : ");
	for (int i = 0; i<now.sons.size(); i++)
		printf("%d ", now.sons[i]);
	printf("\n");

	printf("data : ");
	for (int i = 0; i<now.data.size(); i++)
		printf("%d ", now.data[i].datai);
	printf("\n");
	if (now.isLeaf)
		return;
	for (int i = 0; i<now.sons.size(); i++)
		seetree(fname, now.sons[i], bth);
}

void btree::seetree(const string &fname) {
	Block block = bmreadBlock(fname, 0);
	btreeHeader bth;
	memcpy(&bth, block.data, sizeof(bth));
	printf("----------------\n");
	seetree(fname, bth.rootOffset, bth);
	printf("----------------\n");
}

void btree::deleteEntry(Block BN, attribute K, int P, const string& fileName, btreeHeader bth) {
	node N = binaryToNode(BN.data, bth);
	const int n = bth.fanOut;
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

	if (N.isRoot && N.sons.size() == 1) {

		if (N.sons[0] == -1) {
			nodeToBinary(N, BN.data, bth);
			bmwriteBlock(BN);
			return;
		}
		Block BNewRoot = bmreadBlock(fileName, N.sons[0]);
		node newRoot = binaryToNode(BNewRoot.data, bth);
		newRoot.isRoot = true;
		nodeToBinary(newRoot, BNewRoot.data, bth);
		bmwriteBlock(BNewRoot);

		bmreleaseBlock(fileName, BN.offset);

		bth.rootOffset = BNewRoot.offset;
		saveHeader(bth, fileName);
		return;
	}
	else {

		nodeToBinary(N, BN.data, bth);
		bmwriteBlock(BN);

		assert(N.sons.size() == N.data.size() + 1);
		if (!N.isRoot && ((N.isLeaf && N.data.size()<n / 2) || (!N.isLeaf && N.sons.size()<(n + 1) / 2))) {
			assert(path.size() >= 2);

			int parentoffset = *(path.end() - 2);

			Block BParent = bmreadBlock(fileName, parentoffset);
			node parent = binaryToNode(BParent.data, bth);
			Block BNP;
			attribute KP;
			bool Nisfront;
			bool findd = false;

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
			node NP = binaryToNode(BNP.data, bth);
			assert(findd);
			if (N.sons.size() + NP.sons.size() <= n) {
				if (Nisfront) {
					swap(N, NP);
					swap(BN, BNP);
				}
				if (!N.isLeaf) {
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
				nodeToBinary(N, BN.data, bth);
				nodeToBinary(NP, BNP.data, bth);
				bmwriteBlock(BN);
				bmwriteBlock(BNP);
				path.pop_back();
				deleteEntry(BParent, KP, BN.offset, fileName, bth);
				bmreleaseBlock(fileName, BN.offset);
			}
			else {
				/*redistribute*/

				if (!Nisfront) {
					// if N' -> N
					if (!N.isLeaf) {
						int nppm = *(NP.sons.end() - 1);
						attribute nkmm1 = *(NP.data.end() - 1);
						NP.sons.pop_back();
						NP.data.pop_back();
						N.sons.insert(N.sons.begin(), nppm);
						N.data.insert(N.data.begin(), KP);
						bool findd = false;
						for (int i = 0; i<parent.data.size(); i++) {
							if (parent.data[i] == KP) {
								findd = true;
								parent.data[i] = nkmm1;
								break;
							}
						}
						assert(findd);
						nodeToBinary(parent, BParent.data, bth);
						bmwriteBlock(BParent);
						nodeToBinary(N, BN.data, bth);
						bmwriteBlock(BN);
						nodeToBinary(NP, BNP.data, bth);
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
						nodeToBinary(parent, BParent.data, bth);
						bmwriteBlock(BParent);
						nodeToBinary(N, BN.data, bth);
						bmwriteBlock(BN);
						nodeToBinary(NP, BNP.data, bth);
						bmwriteBlock(BNP);
					}
				}
				else  {
					//N ->N'
					if (!N.isLeaf) {
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
						nodeToBinary(parent, BParent.data, bth);
						bmwriteBlock(BParent);
						nodeToBinary(N, BN.data, bth);
						bmwriteBlock(BN);
						nodeToBinary(NP, BNP.data, bth);
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
						nodeToBinary(parent, BParent.data, bth);
						bmwriteBlock(BParent);
						nodeToBinary(N, BN.data, bth);
						bmwriteBlock(BN);
						nodeToBinary(NP, BNP.data, bth);
						bmwriteBlock(BNP);
					}
				}
			}
		}
		else {
		}
	}
}

bool btree::btDelete(const string &fileName, const attribute &key) {

	btreeHeader bth;
	Block block = bmreadBlock(fileName, 0);
	memcpy(&bth, block.data, sizeof(bth));
	assert(bth.type == key.type);

	int offset = btFind(fileName, bth.rootOffset, key, bth);
	block = bmreadBlock(fileName, offset);
	node now = binaryToNode(block.data, bth);
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
	if (!findd)
		return false;
	deleteEntry(block, K, P, fileName, bth);
	return true;
}

bool btree::btExist(const string &fileName, const attribute &key) {

	btreeHeader bth;
	Block block = bmreadBlock(fileName, 0);
	memcpy(&bth, block.data, sizeof(bth));
	assert(bth.type == key.type);

	int offset = btFind(fileName, bth.rootOffset, key, bth);
	block = bmreadBlock(fileName, offset);
	node now = binaryToNode(block.data, bth);
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
	return findd;
}

set <int> btree::btFindLess(const string &fileName, const attribute &key) {
	btreeHeader bth;
	Block block = bmreadBlock(fileName, 0);
	memcpy(&bth, block.data, sizeof(bth));
	if (bth.type != key.type) {
		printf("%s %d %d\n", fileName.c_str(), bth.type, key.type);
	}
	assert(bth.type == key.type);

	int offset = bth.rootOffset;
	while (1) {
		node now = getNode(fileName, offset, bth);
		if (now.isLeaf)
			break;
		offset = now.sons[0];
	}
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

set <int> btree::btFindMore(const string &fileName, const attribute &key) {
	btreeHeader bth;
	Block block = bmreadBlock(fileName, 0);
	memcpy(&bth, block.data, sizeof(bth));
	assert(bth.type == key.type);

	int offset = btFind(fileName, bth.rootOffset, key, bth);;

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