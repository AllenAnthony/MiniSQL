#pragma once
#ifndef _SQL_H
#define _SQL_H
#include <string>
#include <stdlib.h>
#include "Macro.h"
#include <vector>
using namespace std;

typedef enum { INT, STRING, FLOAT } TYPE;


class Attribute
{
public:
	string name;
	TYPE type;
	int length;
	bool isPrimeryKey;
	bool isUnique;
	Attribute()
	{
		isPrimeryKey = false;
		isUnique = false;
	}
	Attribute(string name, TYPE type, int length, bool ispri, bool isuni):name(name), type(type), length(length), isPrimeryKey(ispri), isUnique(isuni){}
};

class Table//代表一个table文件,记录了一个table文件的信息
{
public:
	string tableName;   //all the datas is store in file name.table
	int blockNum;	//number of block the datas of the table occupied in the file name.table
	//int recordNum;	//number of record in name.table
	int attriNum;	//the number of attributes in the tables
	int totalLength;	//total length of one record, should be equal to sum(attributes[i].length)以字节为最小单位
	vector<Attribute> attribute;
	Table(string tablename) : tableName(tablename),blockNum(0), attriNum(0), totalLength(0)
	{
		attribute.clear();
	}
};

class Index//代表一个index文件,记录了一个文件的信息
{
public:
	string indexName;	//all the datas is store in file index_name.index
	string tableName;	//the name of the table on which the index is create
	int column;			//on which column the index is created, that is to say the key
	TYPE keytype;
	int columnLength;   //the length of the key
	int blockNum;		//number of block the datas of the index occupied in the file index_name.table
	int degree;         //B+树的degree
	int leafhead;
	Index() : column(0), blockNum(0),columnLength(0){}
};

class Row
{
public:
	vector<string> columns;
};
class Data//这样就把Data搞成了一个二维数组
{
public:
	vector<Row> rows;
};

enum Comparison{ Lt, Le, Gt, Ge, Eq, Ne };//stants for less than, less equal, greater than, greater equal, equal, not equal respectivly
class Condition{
public:
	Comparison op;
	int columnNum;
	string value;
};

class block{//代表一个读到内存里的block
public:
	bool Written;
	bool Valid;
	string filename;    //block所在文件
	int blockOffset;	//block所在文件中的位置block offset in file, indicate position in file
	int LRUvalue;		//用于实现LRU算法,the lower, the better,越小说明最近刚被访问
	bool Lock;
	char value[BLOCKSIZE + 1];	//c++的用法怎么不对？ char* values = new char[BLOCKSIZE];
	block()
	{
		Written = 0;
		Valid = 0;
		Lock = 0;
		filename = "NULL";
		blockOffset = 0;
		LRUvalue = 0;
		for (int i = 0; i< BLOCKSIZE; i++) value[i] = EMPTY;
		value[BLOCKSIZE] = '\0';//表示这个block结束了
	}
	bool initialize()
	{
		if (this->Lock)
			return 0;
		Written = 0;
		Valid = 0;
		Lock = 0;
		filename = "NULL";
		blockOffset = -1;
		LRUvalue = 0;
		for (int i = 0; i< BLOCKSIZE; i++) value[i] = EMPTY;
		value[BLOCKSIZE] = '\0';//表示这个block结束了
		return 1;
	}
	string getvalue(int startpos, int endpos){
		string tmpt = "";
		if (startpos >= 0 && startpos <= endpos && endpos <= BLOCKSIZE)
		for (int i = startpos; i< endpos; i++)
			tmpt += value[i];
		return tmpt;
	}
	char getvalue(int pos){
		if (pos >= 0 && pos <= BLOCKSIZE)
			return value[pos];
		return '\0';
	}
};

class insertPos{
public:
	int BLOCKNUM;
	int position;
};
#endif