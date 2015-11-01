#ifndef _TABLE_H_
#define _TABLE_H_

#include <set>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cassert>



class item{
public:
	string name;
	int type;//int = 1, char = 2, float = 3;
	int length;
	bool isUnique;
	bool isPrimary;
	set<string> line;
	bool operator ==(const item &rhs)const{
		if(name == rhs.name && type == rhs.type && length == rhs.length && isUnique == rhs.isUnique && isPrimary == rhs.isPrimary && line == rhs.line){
			return 1;
		}
		return 0;
	}
	bool operator <(const item &rhs)const{
		if(name < rhs.name){
			return 1;
		}
		return 0;
	}
	bool operator >(const item &rhs)const{
		if(name > rhs.name){
			return 1;
		}
		return 0;
	}
	bool operator >=(const item &rhs)const{
		if(name >= rhs.name){
			return 1;
		}
		return 0;
	}
	bool operator <=(const item &rhs)const{
		if(name <= rhs.name){
			return 1;
		}
		return 0;
	}
};

class table{
public:
	string name;
	vector <item> items;
	int size;
	table(){

	}
	table(string rname, const vector<item> &in){
		size = 0;
		this->name = rname;
		this->items = in;
		for(int i = 0; i < in.size(); i++){
			switch(in[i].type){
				case 1:size += sizeof(int);break;
				case 2:size += items[i].length; break;
				case 3:size += sizeof(float);break;
				default: printf("type mismatched\n"); assert(false); break;
			}
		}	
	}
	~table(){

	}
	void writeToTable(string rname, const vector<item> &in){
		if(this->name != rname){
			printf("name dismatched\n");
			assert(false);
		}
		for(int i = 0; i < in.size(); i++){
			this->items.push_back(in[i]);
			switch(in[i].type){
				case 1:size += sizeof(int);break;
				case 2:size += items[i].length; break;
				case 3:size += sizeof(float);break;
				default: printf("type dismatched\n");break; 
			}
		}
	}
	void writeToFile(){
		ofstream fout((name).c_str());
		fout << name << endl;
		fout << size << endl;
		fout << items.size() << endl;
		for (int i=0;i<items.size();i++) {
			fout << items[i].name << endl;
			fout << items[i].type <<endl;
			fout << items[i].length << endl;
			fout << items[i].isUnique << endl;
			fout << items[i].isPrimary <<endl;
			fout << items[i].line.size();
			for (set <string>:: iterator it = items[i].line.begin();it!=items[i].line.end();it++) {
				fout  << " "<< *it;
		}
		fout <<  endl;
	}
	fout.close();
	}
};
#endif



