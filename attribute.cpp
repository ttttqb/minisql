#include "attribute.h"


attribute::attribute() :type(-1){}

attribute::attribute(int data) : datai(data), type(0){}

attribute::attribute(float data) : dataf(data), type(1){}

attribute::attribute(string data) : datas(data), type(2){}

attribute::attribute(const char *data) : datas(data), type(2){}

attribute::~attribute()
{
}

bool attribute::operator<(const attribute &data) const{
	assert(this->type == data.type);
	switch (data.type){
	case 0:return this->datai < data.datai;
	case 1:return this->dataf < data.dataf;
	case 2:return this->datas < data.datas;
	default:assert(false);
	}
}

bool attribute::operator==(const attribute &data) const{
	assert(this->type == data.type);
	switch (data.type){
	case 0:return this->datai == data.datai;
	case 1:return fabs(this->dataf-data.dataf)<1e-7;
	case 2:return this->datas == data.datas;
	default:assert(false);
	}
}

bool attribute::operator>(const attribute &data) const{
	return !(*this < data || *this == data);
}

bool attribute::operator>=(const attribute &data) const{
	return !(*this < data);
}

bool attribute::operator<=(const attribute &data) const{
	return *this < data || *this == data;
}

bool attribute::operator!=(const attribute &data) const{
	return !(*this == data);
}

void attribute::print(){
	switch (type){
	case 0:printf("%d", datai); break;
	case 1:printf("%f", dataf); break;
	case 2:printf("%s", datas.c_str()); break;
	default:assert(false);
	}
}