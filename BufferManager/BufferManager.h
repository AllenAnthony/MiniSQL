#pragma once
#ifndef _BUHHER_H
#define _BUFFER_H
#include "SQL.h"
#include "Macro.h"
#include <string> 
#include <fstream>
#include <vector>
#include<iostream>

/*�ĵ������ǰ�bufferManger������Ľӿں������ĳ�private�ģ�����recordManger��
indexManager����Ϊfriend�������Ϳ��Ա�֤��ֻ��ͨ��bufferManager��indexManger
����ȡbuffer��ֵ�� */
class BufferManager//һ��buffermanager���ж��1024��buffer(�������ڴ����block),ÿ��block 4k
{

public:
	block bufferBlock[MAXBLOCKNUMBER];


public:
	void flashBack(int bufferNum);//�����block���µ��ļ���
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
	

	int getBlockNum(string filename, int blockOffset);//��BM��Ѱ�����ļ�filename�е�blockOffset��block���ҵ��򷵻��ڴ��еĵڼ��飬���Ҳ�����������ڷ����ڴ��еĵڼ���

	void readBlock(string filename, int blockOffset, int bufferNum);//���ļ�filename�е�blockOffset��blockд���ڴ�buffer�еĵ�bufferNum��

	void writeBlock(int bufferNum);//��ʾ���block������

	void useBlock(int bufferNum);//this LRU algorithm is quite expensive

	int getEmptyBuffer();//���ڴ����ҳ�һ���յ�block,���Ҳ�����һ��blockд���ļ�,������պ󷵻�

	int getEmptyBufferExceptFilename(string filename);//buffer with the destine filename is not suppose to be flashback//���ڴ����ҳ�һ���յ�block,���Ҳ�����һ��block(�����Ǵ��filename�ļ����ݵ�block)д���ļ�,������պ󷵻�

	insertPos getInsertPosition(Table& tableinfor);//���������ļ��м�����һ����¼��block��λ�ú������block�е�position//To increase efficiency, we always insert values from the back of the file

	int addBlockInFile(Table& tableinfor);//add one more block in file for the table//��table�ļ�ĩβ���һ��block����������ڴ��б༭�������ڴ�buffer�еĵ�bufferNum��

	int addBlockInFile(Index& indexinfor);//add one more block in file for the index//��index�ļ�ĩβ���һ��block����������ڴ��б༭�������ڴ�buffer�еĵ�bufferNum��

	int getIfIsInBuffer(string filename, int blockOffset);//��BM��Ѱ�����ļ�filename�е�blockOffset��block���ҵ��򷵻��ڴ��еĵڼ��飬���Ҳ����򷵻�-1

	void scanIn(Table tableinfo);//this is dangerous because

	void setInvalid(string filename);//when a file is deleted, a table or an index, all the value in buffer should be set invalid

public:
	/*This function can show the values in the buffer, which is for debug only
	Take care when using. If the size of buffer is very big, your computer may
	crash down*/
	void ShowBuffer(int from, int to);

	void ShowBuffer(int bufferNum);


};

void BufferManager::flashBack(int bufferNum)//�����block���µ��ļ���
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