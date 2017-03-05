#ifndef _INDEX_H
#define _INDEX_H
#include "BufferManager.h"
#include "SQL.h"
#include <list>
#define POINTERLENGTH 5
extern BufferManager buf;

class IndexLeaf{//a leaf,多个indexbranch构成一个B+数的节点,一个B+树的节点为一个block
public:
	string key;
	int offsetInFile;
	int offsetInBlock;
	IndexLeaf():key(""), offsetInFile(0), offsetInBlock(0){}
	IndexLeaf(string k, int oif, int oib):key(k), offsetInFile(oif), offsetInBlock(oib){}
};

class IndexBranch{//not a leaf, normal node,多个indexbranch构成一个B+数的节点,一个B+树的节点为一个block
public:
	string key;
	int ptrChild;	//block pointer,胭脂斋批：这里所谓的指针其实就是block在文件中的偏移,索引储存在index文件中,由叶子节点组成,每个leaf就是一个block,这里指block的编号
	IndexBranch():key(""), ptrChild(0){}
	IndexBranch(string k, int ptrC):key(k), ptrChild(ptrC){}
};

class BPlusTree{
public:
	bool isRoot;
	int bufferNum;
	int ptrFather;		//block pointer, if is root, this pointer is useless
	int recordNum;
	int columnLength;  //index的key的数据长
	BPlusTree(){}
	BPlusTree(int vbufNum): bufferNum(vbufNum), recordNum(0){}
	int getPtr(int pos){//一个pointer最大5位数,有5个字节存储,从位置11开始储存
		int ptr = 0;
		for(int i = pos; i<pos + POINTERLENGTH ; i++){
			ptr = 10 * ptr + buf.bufferBlock[bufferNum].values[i] - '0';
		}
		return ptr;
	}
	int getRecordNum(){//这个block中的record的个数,最大4位数,由4个字节存储,为这个block的2~5位
		int recordNum = 0;
		for(int i = 2; i<6; i++){
			if(buf.bufferBlock[bufferNum].values[i] == EMPTY) break;
			recordNum = 10 * recordNum + buf.bufferBlock[bufferNum].values[i] - '0';
		}
		return recordNum;
	}
};
//在branch节点这个block中,第0位是否为'R'代表这是否是个root,第1位是否为'L'代表这是否是个leaf,这个block中的record的个数,最大4位数,由4个字节存储,为这个block的2~5位,6~10位为parent指针,
//从11位开始由一个有一个的key+child_pointer组成,每一个占据columnLength+5位(这里的位指最小单位：字节)
//unleaf node
class Branch: public BPlusTree
{
public:
	list<IndexBranch> nodelist;
	Branch(){}
	Branch(int vbufNum): BPlusTree(vbufNum){}//this is for new added brach
	Branch(int vbufNum, const Index& indexinfor){//从这个block中读出数据构成branch节点,一个节点即一个nodelist
		bufferNum = vbufNum;
		isRoot = ( buf.bufferBlock[bufferNum].values[0] == 'R' );//第一位是否为'R'代表这是否是个root
		int recordCount = getRecordNum();
		recordNum = 0;//recordNum will increase when function insert is called, and finally as large as recordCount
		ptrFather = getPtr(6);//6~10位为parent指针
		columnLength = indexinfor.columnLength;
		int position = 6 + POINTERLENGTH;
		for(int i = 0; i < recordCount; i++)
		{
			string key = "";
			for(int i = position; i < position + columnLength; i++){
				if(buf.bufferBlock[bufferNum].values[i] == EMPTY) break;
				else key += buf.bufferBlock[bufferNum].values[i];
			}
			position += columnLength;
			int ptrChild = getPtr(position);
			position += POINTERLENGTH;
			IndexBranch node(key, ptrChild);
			insert(node);
		
		}
	}
	~Branch(){//将这个节点的数据存入内存buffer中的一个block
		//isRoot
		if(isRoot) buf.bufferBlock[bufferNum].values[0] = 'R';
		else buf.bufferBlock[bufferNum].values[0] = '_';
		//is not a Leaf
		buf.bufferBlock[bufferNum].values[1] = '_';
		//recordNum
		char tmpt[5];
		itoa(recordNum, tmpt, 10);
		string strRecordNum = tmpt;
		while(strRecordNum.length() < 4)
			strRecordNum = '0' + strRecordNum;
		strncpy(buf.bufferBlock[bufferNum].values + 2, strRecordNum.c_str() , 4);
		
		//nodelist
		if(nodelist.size() == 0){
			cout << "Oh, no no no!! That's impossible." << endl;
			exit(0);
		}

		list<IndexBranch>::iterator i;
		int position = 6 + POINTERLENGTH;	//前面的几位用来存储index的相关信息了
		for(i = nodelist.begin(); i!=nodelist.end(); i++)
		{
			string key = (*i).key;
			while(key.length() <columnLength)
				key += EMPTY;
			strncpy(buf.bufferBlock[bufferNum].values + position, key.c_str(), columnLength);
			position += columnLength;
			
			char tmpt[5];
			itoa((*i).ptrChild, tmpt, 10);
			string ptrChild = tmpt;
			while(ptrChild.length() < POINTERLENGTH)
				ptrChild = '0' + ptrChild;
			strncpy(buf.bufferBlock[bufferNum].values + position, ptrChild.c_str(), POINTERLENGTH);
			position += POINTERLENGTH;
		}
	}
	void insert(IndexBranch node){
		recordNum++;
		list<IndexBranch>::iterator i = nodelist.begin();
		if(nodelist.size() == 0)
			nodelist.insert(i, node);
		else{
			for(i = nodelist.begin(); i!=nodelist.end(); i++)
				if((*i).key > node.key) break;
			nodelist.insert(i, node);//从小到大插入
		}
	}
	
	IndexBranch pop(){
		recordNum--;
		IndexBranch tmpt = nodelist.back();
		nodelist.pop_back();
		return tmpt;
	}
	
	IndexBranch getfront(){
		return nodelist.front();
	}
};


//在leaf节点这个block中,第0位是否为'R'代表这是否是个root,第1位是否为'L'代表这是否是个leaf,这个block中的record的个数,最大4位数,由4个字节存储,为这个block的2~5位,6~10位为parent指针,
//11~15位为上一个叶子指针,16~20为下一个叶子指针,21位开始由一个有一个的key(columnLength)+offsetInFile(5)+offsetInBlock(5)(指这条记录在文件中的那一个block,这个block中的第几个位置)组成,每一个占据columnLength+5位+5位(这里的位指最小单位：字节)
class Leaf: public BPlusTree//将存储在buffer[bufferNum]中
{
public:	
	int nextSibling;	//block pointer
	int lastSibling;	//block pointer
	list<IndexLeaf> nodelist;
	Leaf(int vbufNum){	//this kind of leaf is added when old leaf is needed to be splited
		bufferNum = vbufNum;
		recordNum = 0;
		nextSibling = lastSibling = 0;
	}
	Leaf(int vbufNum, const Index& indexinfor){//从BM的这个这个block中读出数据构成leaf节点,一个节点即一个nodelist
		bufferNum = vbufNum;
		isRoot = ( buf.bufferBlock[bufferNum].values[0] == 'R' );
		int recordCount = getRecordNum();
		recordNum = 0;
		ptrFather = getPtr(6);
		lastSibling = getPtr(6 + POINTERLENGTH);
		nextSibling = getPtr(6 + POINTERLENGTH*2);
		columnLength = indexinfor.columnLength;	//保存起来以后析构函数还要用到的

		//cout << "recordCount = "<< recordCount << endl;
		int position = 6 + POINTERLENGTH*3;	//前面的几位用来存储index的相关信息了
		for(int i = 0; i < recordCount; i++)
		{	
			string key = "";
			for(int i = position; i < position + columnLength; i++){
				if(buf.bufferBlock[bufferNum].values[i] == EMPTY) break;
				else key += buf.bufferBlock[bufferNum].values[i];
			}
			position += columnLength;
			//cout << "get offsetInFile" << endl;
			int offsetInFile = getPtr(position);
			//cout << "get offsetInBlock" << endl;
			position += POINTERLENGTH;
			int offsetInBlock  = getPtr(position);
			position += POINTERLENGTH;
			IndexLeaf node(key, offsetInFile, offsetInBlock);
			insert(node);
		}
	}
	~Leaf(){//将这个节点的数据存入内存buffer中的一个block
		//isRoot
		if(isRoot) buf.bufferBlock[bufferNum].values[0] = 'R';
		else buf.bufferBlock[bufferNum].values[0] = '_';
		//isLeaf
		buf.bufferBlock[bufferNum].values[1] = 'L';
		//recordNum
		char tmpt[5];
		itoa(recordNum, tmpt, 10);
		string strRecordNum = tmpt;
		while(strRecordNum.length() < 4)
			strRecordNum = '0' + strRecordNum;
		int position = 2;
		strncpy(buf.bufferBlock[bufferNum].values + position, strRecordNum.c_str() , 4);
		position += 4;
		
		itoa(ptrFather, tmpt, 10);
		string strptrFather = tmpt;
		while(strptrFather.length() < POINTERLENGTH)
			strptrFather = '0' + strptrFather;
		strncpy(buf.bufferBlock[bufferNum].values + position, strptrFather.c_str() , POINTERLENGTH);
		position += POINTERLENGTH;
		
		itoa(lastSibling, tmpt, 10);
		string strLastSibling = tmpt;
		while(strLastSibling.length() < POINTERLENGTH)
			strLastSibling = '0' + strLastSibling;
		strncpy(buf.bufferBlock[bufferNum].values + position, strLastSibling.c_str() , POINTERLENGTH);
		position += POINTERLENGTH;
		
		itoa(nextSibling, tmpt, 10);
		string strNextSibling = tmpt;
		while(strNextSibling.length() < POINTERLENGTH)
			strNextSibling = '0' + strNextSibling;
		strncpy(buf.bufferBlock[bufferNum].values + position, strNextSibling.c_str() , POINTERLENGTH);
		position += POINTERLENGTH;
		
		//nodelist
		if(nodelist.size() == 0){
			cout << "Oh, no no no!! That's impossible." << endl;
			exit(0);
		}

		list<IndexLeaf>::iterator i;
		for(i = nodelist.begin(); i!=nodelist.end(); i++)
		{
			string key = (*i).key;
			while(key.length() <columnLength)
				key += EMPTY;
			strncpy(buf.bufferBlock[bufferNum].values + position, key.c_str(), columnLength);
			position += columnLength;

			itoa((*i).offsetInFile, tmpt, 10);
			string offsetInFile = tmpt;
			while(offsetInFile.length() < POINTERLENGTH)
				offsetInFile = '0' + offsetInFile;
			strncpy(buf.bufferBlock[bufferNum].values + position, offsetInFile.c_str(), POINTERLENGTH);
			position += POINTERLENGTH;

			itoa((*i).offsetInBlock, tmpt, 10);
			string offsetInBlock = tmpt;
			while(offsetInBlock.length() < POINTERLENGTH)
				offsetInBlock = '0' + offsetInBlock;
			strncpy(buf.bufferBlock[bufferNum].values + position, offsetInBlock.c_str(), POINTERLENGTH);
			position += POINTERLENGTH;
			//cout << key<< "\t" <<offsetInFile<<"\t"<< offsetInFile<< endl;
		}
	}

	void insert(IndexLeaf node){
		recordNum++;
		//cout << "onece" << endl;
		list<IndexLeaf>::iterator i = nodelist.begin();
		if(nodelist.size() == 0){
			nodelist.insert(i, node);
			return;
		}
		else{
			for(i = nodelist.begin(); i!=nodelist.end(); i++)
				if((*i).key > node.key) break;
		}
		nodelist.insert(i, node);//从小到大插入
	}
	IndexLeaf pop(){
		recordNum--;
		IndexLeaf tmpt = nodelist.back();
		nodelist.pop_back();
		return tmpt;
	}
	IndexLeaf getfront(){//this is the smallest of all the keys of the list
		return nodelist.front();
	}
};


//每条记录前面会加一位用来指示这条是否被删除,若为EMPTY则代表已被删除
class IndexManager{
public:
	void createIndex(const Table& tableinfor, Index& indexinfor){
		//create a new file
		string filename = indexinfor.index_name + ".index";
		fstream  fout(filename.c_str(), ios::out);
		fout.close();
		//create a root for the tree
		int blockNum = buf.getEmptyBuffer();
		buf.bufferBlock[blockNum].filename = filename;
		buf.bufferBlock[blockNum].blockOffset = 0;
		buf.bufferBlock[blockNum].isWritten = 1;
		buf.bufferBlock[blockNum].isValid = 1;
		buf.bufferBlock[blockNum].values[0] = 'R';//block的第一位标记是否是根 
		buf.bufferBlock[blockNum].values[1] = 'L';//block的第二位标记是否是叶节点
		//接下来四位标记这个节点有多少条记录，一个block里面的记录大死不会超过9999条吧！ 
		memset( buf.bufferBlock[blockNum].values + 2, '0' , 4);//现在记录条数为零 
		//接下来3*LENGTHBlockPtr位存放三个指针，一个父指针，两个兄弟指针
		for(int i = 0; i < 3; i++)
			memset( buf.bufferBlock[blockNum].values+6 + POINTERLENGTH*i, '0' , POINTERLENGTH);
		indexinfor.blockNum++;
		
		//retrieve datas of the table and form a B+ Tree
		filename = tableinfor.name + ".table";
		string stringrow;
		string key;

		int length = tableinfor.totalLength + 1;//table中的record会加多一位来判断这条记录是否被删除了,而index中的record不会
		const int recordNum = BLOCKSIZE / length;
		
		//read datas from the record and sort it into a B+ Tree and store it
		for(int blockOffset = 0; blockOffset < tableinfor.blockNum; blockOffset++){
			int bufferNum = buf.getIfIsInBuffer(filename, blockOffset);
			if(bufferNum == -1){
				bufferNum = buf.getEmptyBuffer();
				buf.readBlock(filename, blockOffset, bufferNum);
			}
			for(int offset = 0; offset < recordNum; offset ++){
				int position  = offset * length;
				stringrow = buf.bufferBlock[bufferNum].getvalues(position, position + length);
				if(stringrow.c_str()[0] == EMPTY) continue;//inticate that this row of record have been deleted
				stringrow.erase(stringrow.begin());	//把第一位去掉//现在stringrow代表一条记录
				key = getColumnValue(tableinfor, indexinfor, stringrow);
				IndexLeaf node(key, blockOffset, offset);
				insertValue(indexinfor, node);
			}
		}
	}

	IndexBranch insertValue(Index& indexinfor, IndexLeaf node, int blockOffset = 0){
		IndexBranch reBranch;//for return, intial to be empty
		string filename = indexinfor.index_name + ".index";
		int bufferNum = buf.getbufferNum(filename, blockOffset);
		bool isLeaf = ( buf.bufferBlock[bufferNum].values[1] == 'L' );// L for leaf
		if(isLeaf){
			Leaf leaf(bufferNum, indexinfor);
			leaf.insert(node);

			//判断是否要分裂
			const int RecordLength = indexinfor.columnLength + POINTERLENGTH*2;
			const int MaxrecordNum = (BLOCKSIZE-6-POINTERLENGTH*3)/RecordLength;
			if( leaf.recordNum > MaxrecordNum ){//record number is too great, need to split
				if(leaf.isRoot){//this leaf is a root
					int rbufferNum = leaf.bufferNum;	// buffer number for new root
					leaf.bufferNum = buf.addBlockInFile(indexinfor);	//find a new place for old leaf
					int sbufferNum = buf.addBlockInFile(indexinfor);	// buffer number for sibling 
					Branch branchRoot(rbufferNum);	//new root, which is branch indeed
					Leaf leafadd(sbufferNum);	//sibling
					
					//is root
					branchRoot.isRoot = 1;
					leafadd.isRoot = 0;
					leaf.isRoot = 0;

					branchRoot.ptrFather = leafadd.ptrFather = leaf.ptrFather = 0;
					branchRoot.columnLength = leafadd.columnLength = leaf.columnLength;
					//link the newly added leaf block in the link list of leaf
					leafadd.lastSibling = buf.bufferBlock[leaf.bufferNum].blockOffset;
					leaf.nextSibling = buf.bufferBlock[leafadd.bufferNum].blockOffset;
					while(leafadd.nodelist.size() < leaf.nodelist.size()){
						IndexLeaf tnode = leaf.pop();
						leafadd.insert(tnode);
					}

					IndexBranch tmptNode;
					tmptNode.key = leafadd.getfront().key;
					tmptNode.ptrChild = buf.bufferBlock[leafadd.bufferNum].blockOffset;
					branchRoot.insert(tmptNode);
					tmptNode.key = leaf.getfront().key;
					tmptNode.ptrChild = buf.bufferBlock[leaf.bufferNum].blockOffset;
					branchRoot.insert(tmptNode);
					return reBranch;
				}
				else{//this leaf is not a root, we have to cascade up
					int bufferNum = buf.addBlockInFile(indexinfor);
					Leaf leafadd(bufferNum);
					leafadd.isRoot = 0;
					leafadd.ptrFather = leaf.ptrFather;
					leafadd.columnLength = leaf.columnLength;
					
					//link the newly added leaf block in the link list of leaf
					leafadd.nextSibling = leaf.nextSibling;
					leafadd.lastSibling = buf.bufferBlock[leaf.bufferNum].blockOffset;
					leaf.nextSibling = buf.bufferBlock[leafadd.bufferNum].blockOffset;
					if(leafadd.nextSibling != 0){
						int bufferNum = buf.getbufferNum(filename, leafadd.nextSibling);
						Leaf leafnext(bufferNum, indexinfor);
						leafnext.lastSibling = buf.bufferBlock[leafadd.bufferNum].blockOffset;
					}
					while(leafadd.nodelist.size() < leaf.nodelist.size()){
						IndexLeaf tnode = leaf.pop();
						leafadd.insert(tnode);
					}
					reBranch.key = leafadd.getfront().key;
					reBranch.ptrChild = leaf.nextSibling;
					return reBranch;
				}
			}
			else{//not need to split,just return
				return reBranch;
			}

		}
		else{//not a leaf
			Branch branch(bufferNum, indexinfor);
			list<IndexBranch>::iterator i = branch.nodelist.begin();
			if((*i).key > node.key){	//如果新插入的值比最左边的还要小
				(*i).key = node.key;	//就跟新最左边的值
			}
			else{
				for(i = branch.nodelist.begin(); i != branch.nodelist.end(); i++)
					if((*i).key > node.key) break;
				i--;//得到(*i) 左边的指针的位置
			}
			IndexBranch bnode = insertValue(indexinfor, node, (*i).ptrChild);//go down
			
			if(bnode.key == ""){
				return reBranch;
			}
			else{//bnode.key != "", 说明底层的B树快发生了split（分裂），要作出相应的操作
				branch.insert(bnode);
				const int RecordLength = indexinfor.columnLength + POINTERLENGTH;
				const int MaxrecordNum = (BLOCKSIZE-6-POINTERLENGTH) / RecordLength;
				if(branch.recordNum > MaxrecordNum){//need to split up
					if(branch.isRoot){
						int rbufferNum = branch.bufferNum;	// buffer number for new root
						branch.bufferNum = buf.addBlockInFile(indexinfor);	//find a new place for old branch
						int sbufferNum = buf.addBlockInFile(indexinfor);	// buffer number for sibling 
						Branch branchRoot(rbufferNum);	//new root
						Branch branchadd(sbufferNum);	//sibling

						//is root
						branchRoot.isRoot = 1;
						branchadd.isRoot = 0;
						branch.isRoot = 0;

						branchRoot.ptrFather = branchadd.ptrFather = branch.ptrFather = 0;
						branchRoot.columnLength = branchadd.columnLength = branch.columnLength;
						
						while(branchadd.nodelist.size() < branch.nodelist.size()){
							IndexBranch tnode = branch.pop();
							branchadd.insert(tnode);
						}

						IndexBranch tmptNode;
						tmptNode.key = branchadd.getfront().key;
						tmptNode.ptrChild = buf.bufferBlock[branchadd.bufferNum].blockOffset;
						branchRoot.insert(tmptNode);
						tmptNode.key = branch.getfront().key;
						tmptNode.ptrChild = buf.bufferBlock[branch.bufferNum].blockOffset;
						branchRoot.insert(tmptNode);
						return reBranch;//here the function must have already returned to the top lay
					}
					else{//branch is not a root
						int bufferNum = buf.addBlockInFile(indexinfor);
						Branch branchadd(bufferNum);
						branchadd.isRoot = 0;
						branchadd.ptrFather = branch.ptrFather;
						branchadd.columnLength = branch.columnLength;
						
						while(branchadd.nodelist.size() < branch.nodelist.size()){
							IndexBranch tnode = branch.pop();
							branchadd.insert(tnode);
						}
						reBranch.key = branchadd.getfront().key;
						reBranch.ptrChild = buf.bufferBlock[bufferNum].blockOffset;
						return reBranch;
					}
				}
				else{//not need to split,just return
					return reBranch;
				}
			}
		}
		return reBranch;//here is just for safe
	}
	

	Data selectEqual(const Table& tableinfor, const Index& indexinfor, string key, int blockOffset = 0){//start from the root and look down
		Data datas;
		string filename = indexinfor.index_name + ".index";
		int bufferNum = buf.getbufferNum(filename, blockOffset);
		bool isLeaf = ( buf.bufferBlock[bufferNum].values[1] == 'L' );// L for leaf
		if(isLeaf){
			Leaf leaf(bufferNum, indexinfor);
			list<IndexLeaf>::iterator i = leaf.nodelist.begin();
			for(i = leaf.nodelist.begin(); i!= leaf.nodelist.end(); i++)
				if((*i).key == key){
					filename = indexinfor.table_name + ".table";
					int recordBufferNum = buf.getbufferNum(filename, (*i).offsetInFile);//把记录读进buffer
					int position = (tableinfor.totalLength +1)* ((*i).offsetInBlock);
					string stringrow = buf.bufferBlock[recordBufferNum].getvalues(position, position + tableinfor.totalLength);
					if(stringrow.c_str()[0] != EMPTY){
						stringrow.erase(stringrow.begin());//把第一位去掉
						Row splitedRow = splitRow(tableinfor, stringrow);
						datas.rows.push_back(splitedRow);
						return datas;
					}
				}
		}
		else{	//it is not a leaf
			Branch branch(bufferNum, indexinfor);
			list<IndexBranch>::iterator i = branch.nodelist.begin();
			for(i = branch.nodelist.begin(); i != branch.nodelist.end(); i++){
				if((*i).key > key){
					//cout << (*i).key << "==" << key << endl;
					i--;//得到(*i) 左边的指针的位置
					break;
				}
			}
			if(i == branch.nodelist.end()) i--;
			datas = selectEqual(tableinfor, indexinfor, key, (*i).ptrChild);
		}
		return datas;
	}	

	Data selectBetween(const Table& tableinfor, const Index& indexinfor, string keyFrom, string keyTo, int blockOffset = 0){
		Data datas;
		string filename = indexinfor.index_name + ".index";
		int bufferNum = buf.getbufferNum(filename, blockOffset);
		bool isLeaf = ( buf.bufferBlock[bufferNum].values[1] == 'L' );// L for leaf
		if(isLeaf){
			do{
				Leaf leaf(bufferNum, indexinfor);
				list<IndexLeaf>::iterator i;
				for(i = leaf.nodelist.begin(); i!= leaf.nodelist.end(); i++){
					if((*i).key >= keyFrom){
						if((*i).key > keyTo){
							return datas;
						}
						filename = indexinfor.table_name + ".table";
						int recordBufferNum = buf.getbufferNum(filename, (*i).offsetInFile);//把记录读进buffer
						int position = (tableinfor.totalLength +1)* ((*i).offsetInBlock);
						string stringrow = buf.bufferBlock[recordBufferNum].getvalues(position, position + tableinfor.totalLength);
						if(stringrow.c_str()[0] != EMPTY){
							stringrow.erase(stringrow.begin());//把第一位去掉
							Row splitedRow = splitRow(tableinfor, stringrow);
							datas.rows.push_back(splitedRow);
						}
					}
				}
				if(leaf.nextSibling != 0){
					filename = indexinfor.index_name + ".index";
					bufferNum = buf.getbufferNum(filename, leaf.nextSibling);
				}
				else return datas;
			}while(1);
		}
		else{//not leaf, go down to the leaf
			Branch branch(bufferNum, indexinfor);
			list<IndexBranch>::iterator i = branch.nodelist.begin();
			if((*i).key > keyFrom){//如果keyFrom 比最小的键值还要小，就在最左边开始找下去
				datas = selectBetween(tableinfor, indexinfor, keyFrom, keyTo, (*i).ptrChild);
				return datas;
			}
			else{//否则就进入循环，找到入口
				for(i = branch.nodelist.begin(); i != branch.nodelist.end(); i++){
					if((*i).key > keyFrom){
						i--;//得到(*i) 左边的指针的位置
						break;
					}
				}
				datas = selectBetween(tableinfor, indexinfor, keyFrom, keyTo, (*i).ptrChild);
				return datas;
			}
		}
		return datas;
	}
private:
	Row splitRow(Table tableinfor, string row){
		Row splitedRow;
		int s_pos = 0, f_pos = 0;//start position & finish position
		for(int i= 0; i < tableinfor.attriNum; i++){
			s_pos = f_pos;
			f_pos += tableinfor.attributes[i].length;
			string col;
			for(int j = s_pos; j < f_pos; j++){
				if(row[j] == EMPTY) break;
				col += row[j];
			}
			splitedRow.columns.push_back(col);
		}
		return splitedRow;
	}

private:
	string getColumnValue(const Table& tableinfor, const Index& indexinfor, string row){//从记录row中提取key
		string colValue;
		int s_pos = 0, f_pos = 0;	//start position & finish position
		for(int i= 0; i <= indexinfor.column; i++){
			s_pos = f_pos;
			f_pos += tableinfor.attributes[i].length;
		}
		for(int j = s_pos; j < f_pos && row[j] != EMPTY; j++)	colValue += row[j];
		return colValue;
	}
	

public:
	void dropIndex(Index& indexinfor){
		string filename = indexinfor.index_name + ".index";
		if( remove(filename.c_str()) != 0 )
			perror( "Error deleting file" );
		else
			buf.setInvalid(filename);
	}

/*	void dropIndex(const Table& tableinfor){
		string filename = tableinfor.name + ".index";
		if( remove(filename.c_str()) != 0 )
			return;//perror( "Error deleting file" );
		else
			buf.setInvalid(filename);
	}
*/
/*	void dropTable(Table& tableinfor){
		string filename = tableinfor.name + ".index";
		buf.setInvalid(filename);
	}
*/
	void deleteValue(){}
};

#endif
