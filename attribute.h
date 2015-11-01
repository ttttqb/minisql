#pragma once
#include <string>
#include <cassert>
#include <cmath>
using namespace std;

class attribute
{
public:
	int datai;
	float dataf;
	string datas;
	int type;
	bool operator < (const attribute&) const;
	bool operator == (const attribute&) const;
	bool operator > (const attribute&) const;
	bool operator >= (const attribute&) const;
	bool operator <= (const attribute&) const;
	bool operator != (const attribute&) const;
	attribute();
	attribute(int);
	attribute(float);
	attribute(string);
	attribute(const char*);
	~attribute();
	void print();
};

