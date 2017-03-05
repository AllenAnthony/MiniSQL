#include "BufferManager.h"



int BufferManager::getIfIsInBuffer(string filename, int blockOffset)//在BM中寻找在文件filename中第blockOffset个block若找到则返回内存中的第几块，若找不到则返回-1
{
	int cou;
	for (cou = 0; cou < MAXBLOCKNUMBER; cou++)
	{
		if (bufferBlock[cou].filename == filename && bufferBlock[cou].blockOffset == blockOffset)
			return cou;
	}

	return -1;
}

void BufferManager::readBlock(string filename, int blockOffset, int bufferNum)//将文件filename中第blockOffset个block写入内存buffer中的第bufferNum块
{
	bufferBlock[bufferNum].Valid = 1;
	bufferBlock[bufferNum].Written = 0;
	bufferBlock[bufferNum].filename = filename;
	bufferBlock[bufferNum].blockOffset = blockOffset;
	fstream fin(filename.c_str(), ios::in | ios::out);
	fin.seekg(BLOCKSIZE*blockOffset, ios::beg);
	fin.read(bufferBlock[bufferNum].value, BLOCKSIZE);
	fin.close();
	return;
}

int BufferManager::getEmptyBufferExceptFilename(string filename)
{
	int bufferNUM = -1;
	int highestLRUvalue = -1;
	int cou;

	for (cou = 0; cou < MAXBLOCKNUMBER; cou++)
	{
		if (!bufferBlock[cou].Valid)
		{
			bufferBlock[cou].initialize();
			bufferBlock[cou].Valid = 1;
			return cou;
		}
		if ((highestLRUvalue < bufferBlock[cou].LRUvalue)  && (bufferBlock[cou].filename != filename) && (!bufferBlock[cou].Lock))
		{
			highestLRUvalue = bufferBlock[cou].LRUvalue;
			bufferNUM = cou;
		}

	}
	if (bufferNUM == -1)
	{
		cout << "All the buffers in the database system are used up by "<<filename<<". Sorry about that!" << endl;
	    exit(0);
	}

	flashBack(bufferNUM);
	bufferBlock[bufferNUM].initialize();
	bufferBlock[bufferNUM].Valid = 1;
	bufferBlock[bufferNUM].Lock = 0;
	return bufferNUM;
}

int BufferManager::getEmptyBuffer()//在内存中找出一个空的block,若找不到则将一个block写入文件,将其清空后返回
{
	int bufferNUM = 0;
	int highestLRUvalue = bufferBlock[0].LRUvalue;
	int cou;

	for (cou = 0; cou < MAXBLOCKNUMBER; cou++)
	{
		if (!bufferBlock[cou].Valid)
		{
			bufferBlock[cou].initialize();
			bufferBlock[cou].Valid = 1;
			return cou;
		}
		if ((highestLRUvalue < bufferBlock[cou].LRUvalue) && (!bufferBlock[cou].Lock))
		{
			highestLRUvalue = bufferBlock[cou].LRUvalue;
			bufferNUM = cou;
		}

	}
	flashBack(bufferNUM);
	bufferBlock[bufferNUM].initialize();
	bufferBlock[bufferNUM].Valid = 1;
	bufferBlock[bufferNUM].Lock = 0;
	return bufferNUM;
}

int BufferManager::getBlockNum(string filename, int blockOffset)//在BM中寻找在文件filename中第blockOffset个block若找到则返回内存中的第几块，若找不到则将其读入在返回内存中的第几块
{
	int bufferNum = getIfIsInBuffer(filename, blockOffset);
	if (bufferNum == -1)
	{
		bufferNum = getEmptyBufferExceptFilename(filename);
		readBlock(filename, blockOffset, bufferNum);
	}

	return bufferNum;
}

void BufferManager::writeBlock(int bufferNum)//表示这个block被更改
{
	bufferBlock[bufferNum].Written = 1;
	bufferBlock[bufferNum].Valid = 1;
	useBlock(bufferNum);
	return;
}

void BufferManager::useBlock(int bufferNum)//this LRU algorithm is quite expensive
{
	int cou;
	for (cou = 0; cou < MAXBLOCKNUMBER; cou++)
	{
		if (cou == bufferNum)
		{
			bufferBlock[bufferNum].LRUvalue = 0;
			bufferBlock[bufferNum].Valid = 1;
		}
		else
			bufferBlock[bufferNum].LRUvalue++;
	}
	return;
}

insertPos BufferManager::getInsertPosition(Table& TabInf)//获得在这个文件中加入下一条记录的block在文件中的位置和在这个block中的position
{
	insertPos POS;
	if (TabInf.blockNum == 0)
	{
		POS.BLOCKNUM = addBlockInFile(TabInf);
		POS.position = 0;
		return POS;
	}

	string filename = TabInf.tableName + ".table";
	int length = TabInf.totalLength + 1;//在前面加多一位来判断这条记录是否被删除了
	int blockOffset = TabInf.blockNum - 1;//get the block offset in file of the last block
	int bufferNum = getBlockNum(filename, blockOffset);

	const int recordNum = BLOCKSIZE / length;
	for (int offset = 0; offset < recordNum; offset++){
		int position = offset * length;
		if (bufferBlock[bufferNum].value[position] == EMPTY){//find an empty space
			POS.BLOCKNUM = bufferNum;
			POS.position = position;
			return POS;
		}
	}
	//if the program run till here, the last block is full, therefor one more block is added
	POS.BLOCKNUM = addBlockInFile(TabInf);
	POS.position = 0;
	return POS;
}

int BufferManager::addBlockInFile(Table& TabInf)//add one more block in file for the table//在table文件末尾添加一个block并将其放入内存中编辑，返回内存buffer中的第bufferNum块
{
	int bufferNum = getEmptyBuffer();
	bufferBlock[bufferNum].initialize();
	bufferBlock[bufferNum].Valid = 1;
	bufferBlock[bufferNum].Written = 1;
	bufferBlock[bufferNum].filename = TabInf.tableName + ".table";
	bufferBlock[bufferNum].blockOffset = TabInf.blockNum++;
	return bufferNum;
}

int BufferManager::addBlockInFile(Index& IndInf)//add one more block in file for the index//在index文件末尾添加一个block并将其放入内存中编辑，返回内存buffer中的第bufferNum块
{
	string filename = IndInf.indexName + ".index";
	int bufferNum = getEmptyBufferExceptFilename(filename);
	bufferBlock[bufferNum].initialize();
	bufferBlock[bufferNum].Valid = 1;
	bufferBlock[bufferNum].Written = 1;
	bufferBlock[bufferNum].filename = filename;
	bufferBlock[bufferNum].blockOffset = IndInf.blockNum++;
	return bufferNum;
}

void BufferManager::scanIn(Table TabInf)//this is dangerous because
{
	string filename = TabInf.tableName + ".table";
	fstream  fin(filename.c_str(), ios::in);
	for (int blockOffset = 0; blockOffset < TabInf.blockNum; blockOffset++){
		if (getIfIsInBuffer(filename, blockOffset) == -1){	//indicate that the data is not read in buffer yet
			int bufferNum = getEmptyBufferExceptFilename(filename);
			readBlock(filename, blockOffset, bufferNum);
		}
	}
	fin.close();
}

void BufferManager::setInvalid(string filename)//when a file is deleted, a table or an index, all the value in buffer should be set invalid
{
	for (int i = 0; i < MAXBLOCKNUMBER; i++)
	{
		if (bufferBlock[i].filename == filename){
			bufferBlock[i].Lock = 0;

			bufferBlock[i].Valid = 0;
			bufferBlock[i].Written = 0;
			this->flashBack(i);
		}
	}
}

void BufferManager::ShowBuffer(int from, int to)
{
	const int MAX = 30;
	if (!(0 <= from && from <= to && to < MAXBLOCKNUMBER)){
		cout << "不存在这样的buffer" << endl;
		return;
	}
	if ((to - from) > MAX)
	{
		cout << "一次最多显示" << MAX << "个buffer(s)" << endl;
		return;
	}
	for (int i = from; i <= to; i++){
		ShowBuffer(i);
	}
}

void BufferManager::ShowBuffer(int bufferNum)
{
	cout << "BlockNum: " << bufferNum << endl;
	cout << "IsWritten: " << bufferBlock[bufferNum].Written << endl;
	cout << "IsValid: " << bufferBlock[bufferNum].Valid << endl;
	cout << "Filename: " << bufferBlock[bufferNum].filename << endl;
	cout << "blockOffset: " << bufferBlock[bufferNum].blockOffset << endl;
	cout <<"content: "<<endl<< bufferBlock[bufferNum].value << endl;
}



