#include "bufmgr.h"

unsigned char nullfill[BlockSize];
bufmgr::bufmgr()
{
	flist = new map < string, FileStruct >;
	blist = new map < string, list<int> >;
	page = new list < Buffer >;
}


bufmgr::~bufmgr()
{
}

FileStruct bufmgr::bmopenFile(const string &fname){
	if (flist->find(fname) != flist->end()){
		return (*flist)[fname];
	}
	FileStruct temp;
	FILE *fblist = fopen((fname + ".blist").c_str(), "r");
	if (fblist){
		int num;
		/* read page file */
		while (fscanf(fblist, "%d", num) != EOF){
			(*blist)[fname].push_back(num);
		}
		fclose(fblist);
	}
	if ((temp.fp = fopen(fname.c_str(), "rb+")) == 0){
		/* create new file */
		temp.fp = fopen(fname.c_str(), "w");
		fclose(temp.fp);
		temp.fp = fopen(fname.c_str(), "rb+");
		temp.fsize = 0;
	}
	else{
		/* file exists then get the size */
		fseek(temp.fp, 0, SEEK_END);
		temp.fsize = ftell(temp.fp);
	}
	assert(temp.fp);
	(*flist)[fname] = temp;
	return temp;
}

Block bufmgr::bmnewBlock(const string &fname){
	FileStruct temp = bmopenFile(fname);
	if ((*blist)[fname].empty()){
		/* add new block */
		fseek(temp.fp, 0, SEEK_END);
		fwrite(nullfill, BlockSize, 1, temp.fp);
		int offset = temp.fsize;
		temp.fsize += BlockSize;
		(*flist)[fname] = temp;
		return bmreadBlock(fname, offset);
	}
	else{
		int offset = (*blist)[fname].front();
		fseek(temp.fp, offset, SEEK_SET);
		fwrite(nullfill, BlockSize, 1, temp.fp);
		(*blist)[fname].pop_front();
		return bmreadBlock(fname, offset);
	}
}

void bufmgr::bmwriteback(const Buffer &buffer){
	if (buffer.dirty){
		/* write data back to db file */
		assert(buffer.dirty);
		FileStruct temp = bmopenFile(buffer.fname);
		fseek(temp.fp, buffer.offset, SEEK_END);
		fwrite(buffer.data, BlockSize, 1, temp.fp);
	}
}
/* create a new buffer */
void bufmgr::bmnewBuffer(const string &fname, int offset){
	Buffer temp;
	temp.fname = fname;
	temp.offset = offset;
	temp.dirty = false;
	/* LRU algorithm in buffer adding */
	if (page->size() == BufferSize){
		bmwriteback(page->front());
		page->pop_front();
	}
	FileStruct ftemp;
	ftemp = bmopenFile(fname);
	fseek(ftemp.fp, offset, SEEK_SET);
	fread(temp.data, BlockSize, 1, ftemp.fp);
	page->push_back(temp);
}
/* test function */
void bufmgr::bmprintself(){
	for (list<Buffer>::iterator ir = page->begin();
		ir != page->end(); ir++)
	{
		printf("offset=%d dirty=%d", ir->offset, ir->dirty);
		for (int i = 0; i < 0; i++)
			printf("%d", ir->data[i]);
		putchar('\n');
	}
}

Block bufmgr::bmreadBlock(const string &fname, int offset){
	Block temp;
	temp.fname = fname;
	temp.offset = offset;
	for (list<Buffer>::iterator ir = page->begin();
		ir != page->end(); ir++){
		if (ir->fname == fname &&
			ir->offset == offset){//hit
			memcpy(temp.data, ir->data, BlockSize);
			Buffer buffer = *ir;
			page->erase(ir);
			page->push_back(buffer);//LRU
			return temp;
		}
	}
	bmnewBuffer(fname, offset);//miss
	memcpy(temp.data, page->back().data, BlockSize);
	return temp;
}

void bufmgr::bmwriteBlock(Block &block){
	for (list<Buffer>::iterator ir = page->begin();
		ir != page->end(); ir++){
		if (ir->fname == block.fname &&
			ir->offset == block.offset){//hit
			memcpy(ir->data, block.data, BlockSize);
			ir->dirty = true;
			Buffer temp = *ir;
			page->erase(ir);
			page->push_back(temp);//LRU
			return;
		}
	}
	bmnewBuffer(block.fname, block.offset);//miss
	memcpy(page->back().data, block.data, BlockSize);
	page->back().dirty = true;
}
/* flush all data back to db file */
void bufmgr::bmflushBuffer(){
	/* flush data buffer */
	for (list<Buffer>::iterator ir = page->begin(); ir != page->end(); ir++){
		bmwriteback(*ir);
		ir->dirty = false;
	}
	/* flush page list */
	for (map<string, list<int>>::iterator ir = blist->begin();
		ir != blist->end(); ir++){
		FILE* fp = fopen((ir->first + ".blist").c_str(), "w");
		for (list<int>::iterator irr = ir->second.begin();
			irr != ir->second.end(); irr++) {
			fprintf(fp, "%d", *irr);
		}
		fclose(fp);
	}
}
/* release a block by offset */
void bufmgr::bmreleaseBlock(const string &fname, int offset){
	/* check if it's a block have data */
	for (list<int>::iterator ir = (*blist)[fname].begin();
		ir != (*blist)[fname].end(); ir++){
		if (*ir == offset)
			assert(false);
	}
	/* release it from page */
	for (list<Buffer>::iterator ir = page->begin();
		ir != page->end(); ir++){
		if (ir->fname == fname && ir->offset == offset){
			page->erase(ir);
			break;
		}
	}
	/* push back a empty offset */
	(*blist)[fname].push_back(offset);
}
/* clear all data of one table */
void bufmgr::bmclear(const string &fname){
	if (flist->find(fname) != flist->end()){
		FileStruct file = bmopenFile(fname);
		fclose(file.fp);
		flist->erase(flist->find(fname));
	}
	if (blist->find(fname) != blist->end()){
		blist->erase(blist->find(fname));
	}
	while (true){
		bool deleted = false;
		for (list<Buffer>::iterator ir = page->begin();
			ir != page->end(); ir++)
		{
			if (ir->fname == fname)
			{
				page->erase(ir);
				deleted = true;
				break;
			}
		}
		if (!deleted) break;
	}
	remove(fname.c_str());
	remove((fname + ".blist").c_str());
}