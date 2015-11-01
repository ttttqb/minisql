#ifndef _FITTER_H_
#define _FITTER_H_
#include "attribute.h"
#include <vector>
using namespace std;
class Rule {
public:
	int index;
	int type; // 0: < 1: <= 2: = 3: >= 4: > 5: <>
	attribute rhs;
	Rule::Rule(int index, int type, attribute rhs) :index(index), type(type), rhs(rhs) {}
};
class Fitter {
public:
	vector <Rule> rules;
	Fitter(){}
	void addRule(const Rule &rule){
		rules.push_back(rule);
	}
	bool test(const vector <attribute> &data) const{
		for (int i = 0; i<rules.size(); i++) {
			int index = rules[i].index;
			assert(index<data.size());
			switch (rules[i].type) {
			case 0:if (!(data[index]<rules[i].rhs)) return false; break;
			case 1:if (!(data[index] <= rules[i].rhs)) return false; break;
			case 2:if (!(data[index] == rules[i].rhs)) return false; break;
			case 3:if (!(data[index] >= rules[i].rhs)) return false; break;
			case 4:if (!(data[index]>rules[i].rhs)) return false; break;
			case 5:if (!(data[index] != rules[i].rhs)) return false; break;
			default:assert(false);
			}
		}
		return true;
	}
};
#endif
