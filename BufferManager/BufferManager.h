#pragma once
#ifndef _BUHHER_H
#define _BUFFER_H
#include "SQL.h"
#include "Macro.h"
#include <string> 
#include <fstream>
#include <vector>
#include<iostream>

/*文档：我们把bufferManger了里面的接口函数都改成private的，并把recordManger和
indexManager定义为friend，这样就可以保证对只能通过bufferManager和indexManger
来读取buffer的值啦 */
class BufferManager//一个buffermanager中有多个1024个buffer(即读到内存里的block),每个block 4k
{

public:
	block bufferBlock[MAXBLOCKNUMBER];


public:
	void flashBack(int bufferNum);//把这个block更新到文件中
	BufferManager()
	{
		for (int i = 0; i < MAXBLOCKNUMBER; i++)
			bufferBlock[i].initialize();
	}
	~BufferManager()
	{
		for (int i = 0; i < MAXBLOCKNUMBER; i++)
			flashBack(i);
	}

public:
	

	int getBlockNum(string filename, int blockOffset);//在BM中寻找在文件filename中第blockOffset个block若找到则返回内存中的第几块，若找不到则将其读入在返回内存中的第几块

	void readBlock(string filename, int blockOffset, int bufferNum);//将文件filename中第blockOffset个block写入内存buffer中的第bufferNum块

	void writeBlock(int bufferNum);//表示这个block被更改

	void useBlock(int bufferNum);//this LRU algorithm is quite expensive

	int getEmptyBuffer();//在内存中找出一个空的block,若找不到则将一个block写入文件,将其清空后返回

	int getEmptyBufferExceptFilename(string filename);//buffer with the destine filename is not suppose to be flashback//在内存中找出一个空的block,若找不到则将一个block(不能是存放filename文件内容的block)写入文件,将其清空后返回

	insertPos getInsertPosition(Table& tableinfor);//获得在这个文件中加入下一条记录的block的位置和在这个block中的position//To increase efficiency, we always insert values from the back of the file

	int addBlockInFile(Table& tableinfor);//add one more block in file for the table//在table文件末尾添加一个block并将其放入内存中编辑，返回内存buffer中的第bufferNum块

	int addBlockInFile(Index& indexinfor);//add one more block in file for the index//在index文件末尾添加一个block并将其放入内存中编辑，返回内存buffer中的第bufferNum块

	int getIfIsInBuffer(string filename, int blockOffset);//在BM中寻找在文件filename中第blockOffset个block若找到则返回内存中的第几块，若找不到则返回-1

	void scanIn(Table tableinfo);//this is dangerous because

	void setInvalid(string filename);//when a file is deleted, a table or an index, all the value in buffer should be set invalid

public:
	/*This function can show the values in the buffer, which is for debug only
	Take care when using. If the size of buffer is very big, your computer may
	crash down*/
	void ShowBuffer(int from, int to);

	void ShowBuffer(int bufferNum);


};

void BufferManager::flashBack(int bufferNum)//把这个block更新到文件中
{

	if (!bufferBlock[bufferNum].Written)
		return;

	string filename = bufferBlock[bufferNum].filename;
	fstream fout(filename.c_str(), ios::in | ios::out);
	fout.seekp(BLOCKSIZE*bufferBlock[bufferNum].blockOffset, ios::beg);
	fout.write(bufferBlock[bufferNum].value, BLOCKSIZE);
	bufferBlock[bufferNum].initialize();
	fout.close();

}
#endif