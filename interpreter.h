#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#include <cstdio>
#include <string>
#include <cstdlib>
#include <cstring>
#include <strstream>
#include "API.h"
#include "table.h"


using namespace std;

class interpreter:
	public API
{
public:
	void execfile(string);
	attribute convertData(string);
	void analysis(string);
	void nextline(string);
};

#endif