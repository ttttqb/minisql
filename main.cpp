#include <cstdio>
#include <string>
#include "interpreter.h"

using namespace std;

int main(void){
	/* initiallization */
	interpreter dbms;
	char buf[256];
	while (true){
		printf(">>");
		gets(buf);
		dbms.nextline(buf);
	}
}