#pragma once
#ifndef _INDEX_H
#define _INDEX_H
#include "BufferManager.h"
#include "SQL.h"
#include <list>
#include <typeinfo>
#include <iostream>
#include <iterator>
#define POINTERLENGTH 5
extern BufferManager buf;

typedef int POINTER;

template <typename KeyType> KeyType getValue(int bufferNum, int position, int length);
template <typename KeyType> KeyType writeValue(int bufferNum, int position, int length, KeyType K);
int getRecordNum(int bufferNum);
class recordPosition
{
	int blockNum;
	int blockPosition;
	recordPosition() :blockNum(-1), blockPosition(0){}
	recordPosition(int blockNum, int blockPosition):blockNum(blockNum),blockPosition(blockPosition){}
};

template <typename KeyType>
class leafUnit{//a part of a leaf,���leafUnit����һ��B+��leaf�ڵ�,һ��B+���Ľڵ�Ϊһ��block
public:
	KeyType key;
	recordPosition POS;
	template <typename KeyType> leafUnit() :key(KeyType()),POS(0,0){}
	template <typename KeyType> leafUnit(KeyType k, int oif, int oib) : key(k), POS(oif, oib){}
};

template <typename KeyType>
class branchUnit{//not a leaf, normal node,���indexbranch����һ��B+���Ľڵ�,һ��B+���Ľڵ�Ϊһ��block
public:
	KeyType key;
	int ptrChild;	//block pointer,��֬ի����������ν��ָ����ʵ����block���ļ��е�ƫ��,����������index�ļ���,��Ҷ�ӽڵ����,ÿ��leaf����һ��block,����ָblock�ı��
	template <typename KeyType> branchUnit() :key(KeyType()), ptrChild(0){}
	template <typename KeyType> branchUnit(KeyType k, int ptrC) : key(k), ptrChild(ptrC){}
};

template <typename KeyType>
class fatherNode{
public:
	bool isRoot;
	int bufferNum;      //��¼����������ڵ���buf�е���һ���ڵ�
	int father;		//block pointer, if is root, this pointer is useless,��¼�����ĸ��ڵ���index�ļ�������block��λ��
	int recordNum;
	int columnLength;  //index��key�����ݳ�

	fatherNode(){}
	fatherNode(int bufferNum) : bufferNum(bufferNum), recordNum(0){}
	int getPointer(int pos)//һ��pointer���5λ��,��5���ֽڴ洢,��λ��11��ʼ����
	{
		int ptr = 0;
		for (int i = pos; i<pos + POINTERLENGTH; i++){
			ptr = 10 * ptr + buf.bufferBlock[bufferNum].value[i] - '0';
		}
		return ptr;
	}
	int writePointer(int position, POINTER P)//һ��pointer���5λ��,��5���ֽڴ洢,��λ��11��ʼ����
	{
		char tmpf[6];
		itoa(P, tmpf, 10);
		string str = tmpf;
		while (str.length() < 5)
			str = '0' + str;
		strncpy(buf.bufferBlock[bufferNum].value + position, str.c_str(), 5);
	}
	int getRecordNum(){//���block�е�record�ĸ���,���4λ��,��4���ֽڴ洢,Ϊ���block��2~5λ
		int recordNum = 0;
		for (int i = 2; i<6; i++){
			if (buf.bufferBlock[bufferNum].value[i] == EMPTY) break;
			recordNum = 10 * recordNum + buf.bufferBlock[bufferNum].value[i] - '0';
		}
		return recordNum;
	}
	
};

//��branch�ڵ����block��,��0λ�Ƿ�Ϊ'R'�������Ƿ��Ǹ�root,��1λ�Ƿ�Ϊ'L'�������Ƿ��Ǹ�leaf,���block�е�record�ĸ���,���4λ��,��4���ֽڴ洢,Ϊ���block��2~5λ,6~10λΪparentָ��,
//11~15λΪ����ڵ�ĵ�һ��ָ��,��16λ��ʼ��һ����һ����key+child_pointer���,ÿһ��ռ��columnLength+5λ(�����λָ��С��λ���ֽ�)
//unleaf node
template <typename KeyType>
class Branch : public fatherNode<KeyType>
{
public:
	vector<KeyType> key;
	vector<POINTER> child;
	int degree;
public:
	Branch():recordNum(-1),bufferNum(-1){}
	Branch(int _bufferNum) : fatherNode<KeyType>(_bufferNum)
	{
		this->bufferNum = _bufferNum;
		this->child.clear();
		this->key.clear();
		this->columnLength = 0;
		this->isRoot = 0;
		this->recordNum = 0;
		this->father = -1;
		this->degree = 0;
	}//this is for new added brach
	Branch(int _bufferNum, const Index& indexinfor);
	~Branch()
	{
		this->child.clear();
		this->key.clear();
	}
	
public:
	bool search(KeyType mykey, int& offset);//������ڵ�������key���������򷵻������key�ı�ţ����������򷵻����key���ܴ��ڵ��ӽڵ��ָ���ţ�����ͨ��indexʵ��
	void writeBack();//������ڵ�����ݴ����ڴ�buffer�е�һ��block

};

template <typename KeyType>
bool Branch<KeyType>::search(KeyType mykey, int& offset)
{
	offset = 0;
	int cou;
	int count = this->key.size();
	if (count == 0)
	{
		offset = 0;
		return 0;
	}
	else if (count <= 20)// sequential search
	{
		for (cou = 0; cou < count; cou++)
		{
			if (mykey < key[cou])
			{
				offset = cou;
				return 0;
			}
			else if (mykey == key[cou])
			{
				offset = cou;
				return 1;
			}
			else
				continue;
		}
		offset = count;
		return 0;
	}
	else// too many key, binary search. 2* log(n,2) < (1+n)/2
	{
		int start = 0;
		int tail = count - 1;
		int mid;
		if (mykey < key[0])
		{
			offset = 0;
			return 0
		}
		else if (mykey>key[count - 1])
		{
			offset = count;
			return 0;
		}
		while ((start + 1) < tail)
		{
			mid = (start + tail) / 2;
			if (mykey < key[mid])
			{
				tail = mid;
			}
			else if (mykey == key[mid])
			{
				offset = mid;
				return 1;
			}
			else
			{
				start = mid;
			}
		}
		if (mykey == key[start])
		{
			offset = start;
			return 1;
		}
		else if (mykey == key[tail])
		{
			offset = tail;
			return 1;
		}
		else
		{
			offset = tail;
			return 0;
		}


	}

	return 0;
}

template <typename KeyType> 
Branch<KeyType>::Branch(int _bufferNum, const Index& indexinfor) : fatherNode<KeyType>(_bufferNum)//�����block�ж������ݹ���branch�ڵ�,һ���ڵ㼴һ��nodelist
{	
	POINTER ptr = 0;
	this->bufferNum = _bufferNum;
	this->isRoot = (buf.bufferBlock[this->bufferNum].value[0] == 'R');//��һλ�Ƿ�Ϊ'R'�������Ƿ��Ǹ�root
	int recordCount = getRecordNum(_bufferNum);
	this->recordNum = 0;//recordNum will increase when function insert is called, and finally as large as recordCount
	this->father = getPointer(6);//6~10λΪparentָ��
	this->columnLength = indexinfor.columnLength;
	this->degree = indexinfor.degree;
	int position = 6 + POINTERLENGTH;
	for (int i = position; i<position + POINTERLENGTH; i++)
	{
		ptr = 10 * ptr + buf.bufferBlock[bufferNum].value[i] - '0';
	}
	this->child.pop_back(ptr);
	position += POINTERLENGTH;
	for (int i = 0; i < recordCount; i++)
	{
		KeyType K;
		K = getValue<KeyType>(this->bufferNum,position,this->columnLength);
		this->key.push_back(K);
		position += this->columnLength;

		int ptrChild = getPointer(position);
		position += POINTERLENGTH;
		this->child.push_back(ptrChild);
		this->recordNum++;
	}
	buf.bufferBlock[this->bufferNum].Lock = 1;
}

template <typename KeyType>
KeyType getValue(int bufferNum, int position, int length)
{
	KeyType K=KeyType();
	if (typeid(KeyType) == typeid(int))
	{
		for (int i = position; i < position + length; i++)
		{
			K += buf.bufferBlock[bufferNum].value[i];
		}
	}
	else if (typeid(KeyType) == typeid(string))
	{
		for (int i = position; i < position + length; i++)
		{
			K += buf.bufferBlock[bufferNum].value[i];
		}
	}
	else if (typeid(KeyType) == typeid(float))
	{
		unsigned int num=0;
		for (int i = position; i < position + length; i++)
		{
			num += buf.bufferBlock[bufferNum].value[i];
		}
		unsigned int* Pnum = &num;
		float* PF;
		PF = reinterpret_cast<float*>(Pnum);
		K = *PF;
	}
	else
	{
		cout << "invaild type  impossible in get value!!!" << endl;
		exit(1);
	}
	return K;
}

template <typename KeyType>
void writeValue(int bufferNum, int position, int length, KeyType K)
{
	if (typeid(KeyType).name()=="int")
	{
		char ch[4];
		ch[3] = K;
		ch[2] = K >> 8;
		ch[1] = K >> 16;
		ch[0] = K >> 24;
		strncpy(buf.bufferBlock[bufferNum].value + position, ch, 4);
	}
	else if (typeid(KeyType).name() == "string")
	{
		for (int i = position; i < position + length; i++)
		{
			char ch = K[i - position];
			strncpy(buf.bufferBlock[bufferNum].value + i, &ch, 1);
		}
	}
	else if (typeid(KeyType).name() == "float")
	{
		char ch[4];
		ch[3] = K;
		ch[2] = K >> 8;
		ch[1] = K >> 16;
		ch[0] = K >> 24;
		strncpy(buf.bufferBlock[bufferNum].value + position, ch, 4);
	}
	else
	{
		cout << "invaild type  impossible in write value!!!" << endl;
		exit(1);
	}
	return;
}



template <typename KeyType>
void Branch<KeyType>::writeBack()//������ڵ�����ݴ����ڴ�buffer�е�һ��block
{
	//isRoot
	if (this->isRoot) buf.bufferBlock[bufferNum].value[0] = 'R';
	else buf.bufferBlock[bufferNum].value[0] = '_';
	//is not a Leaf
	buf.bufferBlock[bufferNum].value[1] = '_';
	//recordNum
	char tmpt[5];
	itoa(recordNum, tmpt, 10);
	string strRecordNum = tmpt;
	while (strRecordNum.length() < 4)
		strRecordNum = '0' + strRecordNum;
	strncpy(buf.bufferBlock[bufferNum].value + 2, strRecordNum.c_str(), 4);
	//father
	int position = 6;
	writePointer(position, this->father);
	
	//nodelist
	position = 6 + POINTERLENGTH;	//ǰ��ļ�λ�����洢index�������Ϣ��
	writePointer(position, this->child[0]);
	for (int cou = 0; cou < this->recordNum;cou++)
	{
		writeValue<KeyType>(this->bufferNum, position, this->columnLength, this->key[cou]);
		position += columnLength;
		writePointer(position, this->child[cou + 1]);
		position += POINTERLENGTH;
	}
	buf.writeBlock(this->bufferNum);
	buf.bufferBlock[this->bufferNum].Lock = 0;
}

//��leaf�ڵ����block��,��0λ�Ƿ�Ϊ'R'�������Ƿ��Ǹ�root,��1λ�Ƿ�Ϊ'L'�������Ƿ��Ǹ�leaf,���block�е�record�ĸ���,���4λ��,��4���ֽڴ洢,Ϊ���block��2~5λ,6~10λΪparentָ��,
//11~15λΪ��һ��Ҷ��ָ��,16~20Ϊ��һ��Ҷ��ָ��,21λ��ʼ��һ����һ����key(columnLength)+offsetInFile(5)+offsetInBlock(5)(ָ������¼���ļ��е���һ��block,���block�еĵڼ���λ��)���,ÿһ��ռ��columnLength+5λ+5λ(�����λָ��С��λ���ֽ�)

template <typename KeyType> 
class Leaf : public fatherNode<KeyType>//���洢��buffer[bufferNum]��
{
public:
	POINTER nextleaf;	//block pointer
	POINTER lastleaf;	//block pointer
	vector<KeyType> key;
	vector<recordPosition> POS;
	int degree;
public:
	Leaf():recordNum(-1), bufferNum(-1){}
	Leaf(int _bufferNum)//this kind of leaf is added when old leaf is needed to be splited
	{	
		this->bufferNum = _bufferNum;
		this->POS.clear();
		this->key.clear();
		this->columnLength = 0;
		this->isRoot = 0;
		this->recordNum = 0;
		this->father = -1;
		this->nextleaf = -1;
		this->lastleaf = -1;
		this->degree = 0;
	}
	Leaf(int _bufferNum, const Index& indexinfor);
	~Leaf()
	{
		this->POS.clear();
		this->key.clear();
	}

public:
	bool search(KeyType mykey, int& offset);
	void writeBack();
};



template <typename KeyType>
Leaf<KeyType>::Leaf(int _bufferNum, const Index& indexinfor) :fatherNode<KeyType>(_bufferNum)//��BM��������block�ж������ݹ���leaf�ڵ�,һ���ڵ㼴һ��nodelist
{
	POINTER ptr = 0;
	this->bufferNum = _bufferNum;
	this->isRoot = (buf.bufferBlock[this->bufferNum].value[0] == 'R');
	int recordCount = getRecordNum(_bufferNum);
	this->recordNum = 0;
	this->father = getPointer(6);//6~10λΪparentָ��
	this->lastleaf = getPointer(11);
	this->nextleaf = getPointer(16);
	this->columnLength = indexinfor.columnLength;	//���������Ժ�����������Ҫ�õ���
	this->degree = indexinfor.degree;
	//cout << "recordCount = "<< recordCount << endl;
	int position = 6 + POINTERLENGTH * 3;	//ǰ��ļ�λ�����洢index�������Ϣ��
	for (int i = 0; i < recordCount; i++)
	{
		KeyType K;
		K = getValue<KeyType>(this->bufferNum, position, this->columnLength);
		this->key.push_back(K);
		position += this->columnLength;

		recordPosition R;
		R.blockNum = getPointer(position);
		position += POINTERLENGTH;
		R.blockPosition = getPointer(position);
		position += POINTERLENGTH;
		this->POS.push_back(R);
		this->recordNum++;

	}
	buf.bufferBlock[this->bufferNum].Lock = 1;
}



template <typename KeyType>
void Leaf<KeyType>::writeBack()//������ڵ�����ݴ����ڴ�buffer�е�һ��block
{
	//isRoot
	if (this->isRoot) buf.bufferBlock[bufferNum].value[0] = 'R';
	else buf.bufferBlock[bufferNum].value[0] = '_';
	//isLeaf
	buf.bufferBlock[bufferNum].value[1] = 'L';
	//recordNum
	char tmpt[5];
	itoa(recordNum, tmpt, 10);
	string str = tmpt;
	while (str.length() < 4)
		str = '0' + str;
	int position = 2;
	strncpy(buf.bufferBlock[bufferNum].value + position, str.c_str(), 4);
	position += 4;

	writePointer(position, this->father);
	position += POINTERLENGTH;
	writePointer(position, this->lastleaf);
	position += POINTERLENGTH;
	writePointer(position, this->nextleaf);
	position += POINTERLENGTH;
	//nodelist

	for (int cou = 0; cou < this->recordNum; cou++)
	{
		writeValue<KeyType>(this->bufferNum, position, this->columnLength, this->key[cou]);
		position += columnLength;
		writePointer(position, this->POS[cou].blockNum);
		position += POINTERLENGTH;
		writePointer(position, this->POS[cou].blockPosition);
		position += POINTERLENGTH;
	}
	buf.bufferBlock[this->bufferNum].Lock = 0;
}

template <typename KeyType>
bool Leaf<KeyType>::search(KeyType mykey, int& offset)
{
	offset = 0;
	int cou;
	int count = this->key.size();
	if (count == 0)
	{
		offset = 0;
		return 0;
	}
	else if (count <= 20)// sequential search
	{
		for (cou = 0; cou < count; cou++)
		{
			if (mykey < key[cou])
			{
				offset = cou;
				return 0;
			}
			else if (mykey == key[cou])
			{
				offset = cou;
				return 1;
			}
			else
				continue;
		}
		offset = count;
		return 0;
	}
	else// too many key, binary search. 2* log(n,2) < (1+n)/2
	{
		int start = 0;
		int tail = count - 1;
		int mid;
		if (mykey < key[0])
		{
			offset = 0;
			return 0
		}
		else if (mykey>key[count - 1])
		{
			offset = count;
			return 0;
		}
		while ((start + 1) < tail)
		{
			mid = (start + tail) / 2;
			if (mykey < key[mid])
			{
				tail = mid;
			}
			else if (mykey == key[mid])
			{
				offset = mid;
				return 1;
			}
			else
			{
				start = mid;
			}
		}
		if (mykey == key[start])
		{
			offset = start;
			return 1;
		}
		else if (mykey == key[tail])
		{
			offset = tail;
			return 1;
		}
		else
		{
			offset = tail;
			return 0;
		}


	}

	return 0;
}
//**********************BplusTree***************************//















#endif