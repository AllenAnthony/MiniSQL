#ifndef __yyySQL_h__
#define __yyySQL_h__
#include <string>
#include "macro.h"
#include <vector>
using namespace std;

class Attribute
{
public:
	string name;
	int type;
	int length;
	bool isPrimeryKey;
	bool isUnique;
	Attribute()
	{
	 isPrimeryKey=false;
	 isUnique=false;
	}
	Attribute(string n, int t, int l, bool isP, bool isU)
		:name(n), type(t), length(l), isPrimeryKey(isP), isUnique(isU){}
};

class Table//����һ��table�ļ�
{
public:
	string name;   //all the datas is store in file name.table
	int blockNum;	//number of block the datas of the table occupied in the file name.table
	//int recordNum;	//number of record in name.table
	int attriNum;	//the number of attributes in the tables
	int totalLength;	//total length of one record, should be equal to sum(attributes[i].length)���ֽ�Ϊ��С��λ
	vector<Attribute> attributes;
	Table(): blockNum(0), attriNum(0), totalLength(0){}
};

class Index//����һ��index�ļ�
{
public:
	string index_name;	//all the datas is store in file index_name.index
	string table_name;	//the name of the table on which the index is create
	int column;			//on which column the index is created, that is to say the key
	int columnLength;   //the length of the key
	int blockNum;		//number of block the datas of the index occupied in the file index_name.table
	Index(): column(0), blockNum(0){}
};

class Row
{
public:
	vector<string> columns;
};
class Data//�����Ͱ�Data�����һ����ά����
{
public:
	vector<Row> rows;
};

enum Comparison{Lt, Le, Gt, Ge, Eq, Ne};//stants for less than, less equal, greater than, greater equal, equal, not equal respectivly
class Condition{
public:
	Comparison op;
	int columnNum;
	string value;
};

class buffer{//����һ�������ڴ����block
public:
	bool isWritten;
	bool isValid;
	string filename;
	int blockOffset;	//block offset in file, indicate position in file
	int LRUvalue;		//����ʵ��LRU�㷨,the lower, the better
	char values[BLOCKSIZE + 1];	//c++���÷���ô���ԣ� char* values = new char[BLOCKSIZE];
	buffer(){
		initialize();
	}
	void initialize(){
		isWritten  = 0;
		isValid = 0;
		filename = "NULL";
		blockOffset = 0;
		LRUvalue= 0;
		for(int i = 0; i< BLOCKSIZE; i++) values[i] = EMPTY;
		values[BLOCKSIZE] = '\0';//��ʾ���block������
	}
	string getvalues(int startpos, int endpos){
		string tmpt = "";
		if(startpos >=0 && startpos <= endpos && endpos <= BLOCKSIZE)
			for(int i = startpos; i< endpos; i++)
				tmpt += values[i];
		return tmpt;
	}
	char getvalues(int pos){
		if(pos >=0 && pos <= BLOCKSIZE)
			return values[pos];
		return '\0';
	}
};

class insertPos{
public:
	int bufferNUM;
	int position;
};

#endif



//һ���ļ�(.table   .index)��Ϊ���block
