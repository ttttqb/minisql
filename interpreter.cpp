#include "interpreter.h"
/* execfile command */
void interpreter::execfile(string fname) {
	FILE *fp = fopen(fname.c_str(), "r");
	char buf[1024];
	if (!fp)
		printf("no exist file.\n");
	else {
		while (fgets(buf, 1024, fp)) {
			if (buf[strlen(buf) - 1] == '\n') {
				buf[strlen(buf) - 1] = 0;
			}
			/* running line by line */
			nextline(buf);
		}
	}
}

/* convert data from string to attribute */
attribute interpreter::convertData(string data) {
	char *c;
	if (data.find(".") != -1) {
		return strtof(data.c_str(), &c);

	}
	if (data[0] == '\'') {
		string res;
		for (int i = 1; i<data.length() - 1; i++)
			res = res + data[i];
		return res;
	}
	else
		return int(strtod(data.c_str(), &c) + 0.5);
}

int convertOperation(string& oper){
	if (oper == "<") {
		return 0;
	}
	else if (oper == "<=") {
		return 1;
	}
	else if (oper == "=") {
		return 2;
	}
	else if (oper == ">=") {
		return 3;
	}
	else if (oper == ">") {
		return 4;
	}
	else if (oper == "<>") {
		return 5;
	}
	else {

		return -1;
	}
}
/* analyse a sentence */
void interpreter::analysis(string input) {
	if (input == "quit") {
		bmflushBuffer();
		exit(0);
	}


	if (input == "")
		return;
	/* add ' ' and remove ' ' in string for analysing easily */
	for (int i = 0; i<input.length(); i++) {
		if (input[i] == '(' || input[i] == ')' || input[i] == ',') {
			input.insert(i, " ");
			input.insert(i + 2, " ");
			i += 2;
		}
		if (input[i] == '\'' && input[i - 1] == ' '){
			int j = i + 1;
			while (input[j] != '\''){
				if (input[j] == ' ') input.erase(input.begin() + j);
				else j++;
			}
		}
	}
	strstream str;
	str << input;

	string opt;
	str >> opt;

	string streater;
	/* execfile */
	if (opt == "execfile") {
		string fname;
		str >> fname;
		execfile(fname);

	}
	else if (opt == "create"){
		string type;
		str >> type;
		/* create table */
		if (type == "table") {
			string tablename;
			str >> tablename;
			str >> streater;
			vector <item> data;
			int pk = 0x3f3f3f3f;
			while (1) {
				string itemname;
				str >> itemname;
				/* add primary key */
				if (itemname == "primary") {
					str >> streater;
					str >> streater;
					string pkname;
					str >> pkname;
					for (int i = 0; i<data.size(); i++) {
						if (data[i].name == pkname)
							pk = i;
					}
					break;
				}
				/* add attribute */
				string datatype;
				int length = 0;
				str >> datatype;
				if (datatype == "char") {
					str >> streater;
					str >> length;
					str >> streater;
				}
				string unique;
				str >> unique;
				item newitem;
				if (unique == "unique") {
					newitem.unique = true;
					str >> streater;
				}
				else {
					newitem.unique = false;
				}
				newitem.name = itemname;
				if (datatype == "char") {
					newitem.type = 2;
					newitem.length = length;
				}
				else if (datatype == "int"){
					newitem.type = 0;
					newitem.length = 0;
				}
				else if (datatype == "float"){
					newitem.type = 1;
					newitem.length = 0;
				}
				else {
					printf("unrecognized type %s\n", datatype.c_str());
					return;
				}
				data.push_back(newitem);
			}
			if (pk == 0x3f3f3f3f) {
				printf("primary key wrong");
			}
			else {
				/* create table */
				API res = APIcreateTable(tablename, data, pk);
				if (!res.succeed) {
					printf("%s\n", res.info.c_str());
				}
				else
					printf("OK\n");
			}

		}
		/* create index */
		else if (type == "index") {
			string indexname;
			string tablename;
			string itemname;
			str >> indexname >> streater >> tablename >> streater >> itemname;
			/* create table function */
			API res = APIcreateIndex(indexname, tablename, itemname);
			if (!res.succeed) {
				printf("%s\n", res.info.c_str());
			}
			else
				printf("OK\n");
		}
		else printf("syntax error");

	}
	else if (opt == "drop"){
		string type;
		str >> type;
		/* drop table */
		if (type == "table") {
			string tablename;
			str >> tablename;
			API res = APIdropTable(tablename);
			if (!res.succeed) {
				printf("%s\n", res.info.c_str());
			}
			else
				printf("OK\n");
		}
		/* drop index */
		else if (type == "index") {
			string indexname;
			str >> indexname;
			API res = APIdropIndex(indexname);
			if (!res.succeed) {
				printf("%s\n", res.info.c_str());
			}
			else
				printf("OK\n");
		}

	}
	else if (opt == "insert") {
		/* insert data */
		string tablename;
		str >> streater >> tablename >> streater >> streater;
		/* save data in a vector<attribute>*/
		vector <attribute> entry;
		while (1) {
			string data;
			str >> data;
			entry.push_back(convertData(data));
			str >> streater;
			if (streater == ")")
				break;
		}
		API res = APIinsert(tablename, entry);
		if (!res.succeed) {
			printf("%s\n", res.info.c_str());
		}
		else
			printf("OK\n");

	}
	else if (opt == "select") {
		/* select table */
		string tablename;
		str >> streater >> streater >> tablename;
		Ruletree ruletree;
		if (!cmExistTable(tablename + ".table")) {
			printf("no such table\n");
			return;
		}
		table nowtable = cmReadTable(tablename + ".table");
		/* read the select condition */
		while (str >> streater) {
			string itemname;
			string oper;
			string data;
			str >> itemname >> oper >> data;
			int index = -1;
			for (int i = 0; i<nowtable.items.size(); i++) {
				if (nowtable.items[i].name == itemname) {
					index = i;
					break;
				}
			}
			if (index<0)  {
				printf("no such item\n");
				return;
			}
			int op = convertOperation(oper);
			if (op == -1)
			{
				printf("syntax error\n");
				return;
			}
			ruletree.addRule(Rule(index, op, convertData(data)));
		}
		/* call select function */
		API res = APIselect(tablename, ruletree);
		if (!res.succeed)
			printf("%s\n", res.info.c_str());
		else {
			/* get select result and output */
			vector <int> space(nowtable.items.size());
			for (int i = 0; i<nowtable.items.size(); i++) {
				space[i] = nowtable.items[i].name.length() + 1;
			}
			for (int i = 0; i<res.result.size(); i++) {
				for (int j = 0; j<res.result[i].size(); j++) {
					int len;
					switch (res.result[i][j].type)  {
					case 0:{str << res.result[i][j].datai; string s; str >> s; len = s.length(); break; }
					case 1:{str << res.result[i][j].dataf; string s; str >> s; len = s.length(); break; }
					case 2:{len = res.result[i][j].datas.length(); break; }
					}
					if (len>space[j])
						space[j] = len + 1;
				}
			}
			int sum = 0;
			for (int i = 0; i<nowtable.items.size(); i++)
				sum += space[i] + 1;
			sum++;
			for (int i = 0; i<sum; i++) printf("-");
			printf("\n");
			printf("|");
			for (int i = 0; i<nowtable.items.size(); i++) {
				printf("%s", nowtable.items[i].name.c_str());
				for (int j = 0; j<space[i] - nowtable.items[i].name.length(); j++)
					printf(" ");
				printf("|");
			}
			printf("\n");
			for (int i = 0; i<res.result.size(); i++) {
				printf("|");
				for (int j = 0; j<res.result[i].size(); j++) {
					int len;
					strstream str;
					switch (res.result[i][j].type)  {
					case 0:{str << res.result[i][j].datai; string s; str >> s; len = s.length(); break; }
					case 1:{str << res.result[i][j].dataf; string s; str >> s; len = s.length(); break; }
					case 2:{len = res.result[i][j].datas.length(); break; }
					}
					res.result[i][j].print();
					for (int s = 0; s<space[j] - len; s++)
						printf(" ");
					printf("|");
				}
				printf("\n");
			}
			for (int i = 0; i<sum; i++) printf("-");
			printf("\n");
		}
	}
	else if (opt == "delete") {
		string tablename;
		str >> streater >> tablename;
		Ruletree ruletree;
		if (!cmExistTable(tablename + ".table")) {
			printf("no such table\n");
			return;
		}
		table nowtable = cmReadTable(tablename + ".table");
		/* delete condition */
		while (str >> streater) {
			string itemname;
			string oper;
			string data;
			str >> itemname >> oper >> data;
			int index = -1;
			for (int i = 0; i<nowtable.items.size(); i++) {
				if (nowtable.items[i].name == itemname) {
					index = i;
					break;
				}
			}
			if (index<0)  {
				printf("no such item\n");
				return;
			}
			int op = convertOperation(oper);
			if (op == -1)
			{
				printf("syntax error\n");
				return;
			}
			ruletree.addRule(Rule(index, op, convertData(data)));
		}
		/* call delete function */
		API res = APIdelete(tablename, ruletree);
		if (!res.succeed)
			printf("%s\n", res.info.c_str());
	}
	else {
		puts("syntax error");
	}


}
void interpreter::nextline(string now) {

	static string data;
	for (int i = 0; i<now.length(); i++) {
		if (now[i] == ';') {
			string dd = data;
			data = "";
			analysis(dd);
		}
		else {
			data = data + now[i];
		}
	}
}