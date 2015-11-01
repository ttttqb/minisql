#include "bufmgr.h"

bufmgr::bufmgr()
{
	flist = new map < string, FileStruct > ;
	plist = new map < string, list<int> > ;
	page = new list < Buffer > ;
}


bufmgr::~bufmgr()
{
}

FileStruct bufmgr::bmopenFile(const string &fname){
	if (flist->find(fname) != flist->end()){
		return (*flist)[fname];
	}
	FileStruct temp;
	FILE *fplist = fopen((fname + ".plist").c_str(), "r");
	if (fplist){
		int num;
		//read page file
		while (fscanf(fplist, "%d", num) != EOF){
			(*plist)[fname].push_back(num);
		}
		fclose(fplist);
	}
	if ((temp.fp = fopen(fname.c_str(), "rb+")) == 0){
		//create new file
		temp.fp = fopen(fname.c_str(), "w");
		fclose(temp.fp);
		temp.fp = fopen(fname.c_str(), "rb+");
		temp.fsize = 0;
	}
	else{
		//file exists then get the size
		fseek(temp.fp, 0, SEEK_END);
		temp.fsize = ftell(temp.fp);
	}
	assert(temp.fp);
	(*flist)[fname] = temp;
	return temp;
}

Block bufmgr::bmnewBlock(const string &fname){
	FileStruct temp = bmopenFile(fname);
	if ((*plist)[fname].empty()){
		//add new block
		fseek(temp.fp, 0, SEEK_END);
		fwrite(nullfill, BlockSize, 1, temp.fp);
		int offset = temp.fsize;
		temp.fsize += BlockSize;
		(*flist)[fname] = temp;
		return bmreadBlock(fname, offset);
	}
	else{
		int offset = (*plist)[fname].front();
		fseek(temp.fp, offset, SEEK_SET);
		fwrite(nullfill, BlockSize, 1, temp.fp);
		(*plist)[fname].pop_front();//LRU?
		return bmreadBlock(fname, offset);
	}
}

void bufmgr::bmwriteback(const Buffer &buffer){
	if (buffer.dirty){
		FileStruct temp = bmopenFile(buffer.fname);
		fseek(temp.fp, buffer.offset, SEEK_END);
		fwrite(buffer.data, BlockSize, 1, temp.fp);
	}
}

void bufmgr::bmnewBuffer(const string &fname, int offset){
	Buffer temp;
	temp.fname = fname;
	temp.offset = offset;
	temp.dirty = false;
	if (page->size() == BufferSize){
		bmwriteback(page->front());//LRU
		page->pop_front();
	}
	FileStruct ftemp;
	ftemp = bmopenFile(fname);
	fseek(ftemp.fp, offset, SEEK_SET);
	fread(temp.data, BlockSize, 1, ftemp.fp);
	page->push_back(temp);
}

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
			page->push_back(temp);
			return;
		}
	}
	bmnewBuffer(block.fname, block.offset);//miss
	memcpy(page->back().data, block.data, BlockSize);
	page->back().dirty = true;
}

void bufmgr::bmflushBuffer(){
	for (list<Buffer>::iterator ir = page->begin(); ir != page->end(); ir++){
		bmwriteback(*ir);
		ir->dirty = false;
	}
	for (map<string, list<int>>::iterator ir = plist->begin();
		ir != plist->end(); ir++){
		FILE* fp = fopen((ir->first + ".plist").c_str(), "w");
		for (list<int>::iterator irr = ir->second.begin();
			irr != ir->second.end(); irr++) {
			fprintf(fp, "%d", *irr);
		}
		fclose(fp);
	}
}

void bufmgr::bmreleaseBlock(const string &fname, int offset){
	for (list<int>::iterator ir = (*plist)[fname].begin();
		ir != (*plist)[fname].end(); ir++){
		if (*ir == offset)
			assert(false);
	}
	for (list<Buffer>::iterator ir = page->begin();
		ir != page->end(); ir++){
		if (ir->fname == fname && ir->offset == offset){
			page->erase(ir);
			break;
		}
	}
	(*plist)[fname].push_back(offset);
}

void bufmgr::bmclear(const string &fname){
	if (flist->find(fname) != flist->end()){
		FileStruct file = bmopenFile(fname);
		fclose(file.fp);
		flist->erase(flist->find(fname));
	}
	if (plist->find(fname) != plist->end()){
		plist->erase(plist->find(fname));
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
	remove((fname + ".plist").c_str());
}