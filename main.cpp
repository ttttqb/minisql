#include <cstdio>
#include <string>
#include "interpreter.h"

using namespace std;
char buf[256];

int main(void){
	bufmgr *db = new bufmgr();
	interpreter dbms(*db);
	while (true){
		printf(">>");
		gets(buf);
		dbms.ipAddLine(buf);
	}
}