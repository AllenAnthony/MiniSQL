#include "BufferManager.h"



int BufferManager::getIfIsInBuffer(string filename, int blockOffset)//��BM��Ѱ�����ļ�filename�е�blockOffset��block���ҵ��򷵻��ڴ��еĵڼ��飬���Ҳ����򷵻�-1
{
	int cou;
	for (cou = 0; cou < MAXBLOCKNUMBER; cou++)
	{
		if (bufferBlock[cou].filename == filename && bufferBlock[cou].blockOffset == blockOffset)
			return cou;
	}

	return -1;
}

void BufferManager::readBlock(string filename, int blockOffset, int bufferNum)//���ļ�filename�е�blockOffset��blockд���ڴ�buffer�еĵ�bufferNum��
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

int BufferManager::getEmptyBuffer()//���ڴ����ҳ�һ���յ�block,���Ҳ�����һ��blockд���ļ�,������պ󷵻�
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

int BufferManager::getBlockNum(string filename, int blockOffset)//��BM��Ѱ�����ļ�filename�е�blockOffset��block���ҵ��򷵻��ڴ��еĵڼ��飬���Ҳ�����������ڷ����ڴ��еĵڼ���
{
	int bufferNum = getIfIsInBuffer(filename, blockOffset);
	if (bufferNum == -1)
	{
		bufferNum = getEmptyBufferExceptFilename(filename);
		readBlock(filename, blockOffset, bufferNum);
	}

	return bufferNum;
}

void BufferManager::writeBlock(int bufferNum)//��ʾ���block������
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

insertPos BufferManager::getInsertPosition(Table& TabInf)//���������ļ��м�����һ����¼��block���ļ��е�λ�ú������block�е�position
{
	insertPos POS;
	if (TabInf.blockNum == 0)
	{
		POS.BLOCKNUM = addBlockInFile(TabInf);
		POS.position = 0;
		return POS;
	}

	string filename = TabInf.tableName + ".table";
	int length = TabInf.totalLength + 1;//��ǰ��Ӷ�һλ���ж�������¼�Ƿ�ɾ����
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

int BufferManager::addBlockInFile(Table& TabInf)//add one more block in file for the table//��table�ļ�ĩβ���һ��block����������ڴ��б༭�������ڴ�buffer�еĵ�bufferNum��
{
	int bufferNum = getEmptyBuffer();
	bufferBlock[bufferNum].initialize();
	bufferBlock[bufferNum].Valid = 1;
	bufferBlock[bufferNum].Written = 1;
	bufferBlock[bufferNum].filename = TabInf.tableName + ".table";
	bufferBlock[bufferNum].blockOffset = TabInf.blockNum++;
	return bufferNum;
}

int BufferManager::addBlockInFile(Index& IndInf)//add one more block in file for the index//��index�ļ�ĩβ���һ��block����������ڴ��б༭�������ڴ�buffer�еĵ�bufferNum��
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
		cout << "������������buffer" << endl;
		return;
	}
	if ((to - from) > MAX)
	{
		cout << "һ�������ʾ" << MAX << "��buffer(s)" << endl;
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



