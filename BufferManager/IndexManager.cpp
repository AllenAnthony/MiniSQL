#include"IndexManager.h"
#include<iterator>



int OutGetPointer(int bufferNum,int pos)//一个pointer最大5位数,有5个字节存储,从位置11开始储存
{
	int ptr = 0;
	for (int i = pos; i<pos + POINTERLENGTH; i++){
		ptr = 10 * ptr + buf.bufferBlock[bufferNum].value[i] - '0';
	}
	return ptr;
}
void OutSetPointer(int bufferNum, int position, int pointer)
{
	char tmpf[6];
	_itoa_s(pointer, tmpf, 10);
	string str = tmpf;
	while (str.length() < 5)
		str = '0' + str;
	strncpy(buf.bufferBlock[bufferNum].value + position, str.c_str(), 5);
	return;
}

template <class KeyType>
void dropIndex(Index& indexinfor);

template <typename KeyType>
class IndexManager
{
public:
	Index treeIndex;
	Table treeTable;
	Branch<KeyType> rootB;
	Leaf<KeyType> rootL;
	string indexname;
	bool isLeaf;
public:
	IndexManager(const Index& _treeIndex, const Table& _treeTable);
	void creatIndex();
	void InitializeRoot();
	int getKeyPosition();
	~IndexManager(){};

public:
	bool search(const KeyType& mykey,int& offset,int bufferNum);
	bool findToLeaf(const KeyType& mykey, int& offset, int& bufferNum);

	recordPosition selectEqual(KeyType mykey);
	vector<recordPosition>& selectBetween(KeyType mykey1, KeyType mykey2);


	bool insertIndex(KeyType mykey, int blockNum, int blockOffset);//需要维护block的writeen
	bool insertBranch(KeyType mykey, int child,int bufferNum, int offset);
	bool insertBranch(KeyType mykey, int child, int branchBlock);
	bool insertLeaf(KeyType mykey, int blockNum, int blockOffset, int bufferNum,int offset);//将一个值插入到buffer中的一个叶子中的第offset的前一个位置
	bool insertLeaf(KeyType mykey, int blockNum, int blockOffset, int leafBlock);//将一个值插入到index文件中的第leafBlock中去

	void adjustafterinsert(int bufferNum);

	bool deleteIndex(KeyType mykey);
	bool deleteBranch(KeyType mykey, int branchBlock);
	bool deleteBranch(KeyType mykey, int bufferNum, int offset);
	bool deleteLeaf(KeyType mykey, int leafBlock);
	bool deleteLeaf(KeyType mykey, int bufferNum, int offset);

	void adjustafterdelete(int bufferNum);
	void deleteNode(int bufferNum);
};


template <typename KeyType>
IndexManager<KeyType>::IndexManager(const Index& _treeIndex, const Table& _treeTable)//需要维护：blockNum;degree;leafhead;
{
	treeIndex = _treeIndex;
	treeTable = _treeTable;
	char ch;
	indexname = treeIndex.indexName + ".index";
    treeIndex.degree = (BLOCKSIZE - 30) / (this->columnLength + 2 * POINTERLENGTH);//recordNum最大degree-1个
	fstream  fp(indexname.c_str(), ios::in | ios::out | ios::binary);
	if (fp.eof())
	{
		fp.close();
		creatIndex();
		return;
	}

	
	int bufferNum=buf.getEmptyBufferExceptFilename(indexname);
	buf.readBlock(indexname, 0, bufferNum);

	if (buf.bufferBlock[bufferNum].value[0] != 'R')
	{
		cout << "indexfile " << indexname << " exist error!!!" << endl;
		exit(1);
	}
	else
	{
		buf.bufferBlock[bufferNum].Lock = 1;
		if (buf.bufferBlock[bufferNum].value[1] == 'L')
		{
			this->rootL(bufferNum, this->treeIndex);
			this->isLeaf = 1;
		}
		else
		{
			this->rootB(bufferNum, this->treeIndex);
			this->isLeaf = 0;
		}
	}
}

//在这个节点中搜索key，若存在则返回这个的key的编号，若不存在则返回这个key肯能存在的子节点的指针编号，返回通过offset实现
template <typename KeyType>
bool IndexManager<KeyType>::search(const KeyType& mykey, int& offset, int bufferNum)
{
	bool res;
	if (buf.bufferBlock[bufferNum].value[1] == 'L')
	{
		Leaf<KeyType> node(bufferNum, this->treeIndex);
		res = node.search(mykey, offset);
	}
	else
	{
		Branch<KeyType> node(bufferNum, this->treeIndex);
		res = node.search(mykey, offset);
	}

	return res;
}

//在这棵树中搜索key，若存在则返回这个叶子节点在内存中的bufferNum和这个key在这个节点中是第几个
template <class KeyType>
bool IndexManager<KeyType>::findToLeaf(const KeyType& mykey, int& offset, int& bufferNum)//在这棵树中搜索key，若存在则返回这个叶子节点在内存中的bufferNum和这个key在这个节点中是第几个，若不存在则返回这个key肯能存在的子节点的指针编号，返回通过index实现
{
	POINTER myChild;
	if (this->search(mykey, offset, bufferNum))
	{
		if (buf.bufferBlock[bufferNum].value[1] == 'L')
		{
			bufferNum = bufferNum;
			return 1;
		}
		else
		{
			myChild = OutGetPointer(bufferNum, 16 + offset*(treeIndex.columnLength + 5) + treeIndex.columnLength);
			bufferNum = buf.getBlockNum(this->indexname, myChild);
			while (buf.bufferBlock[bufferNum].value[1] != 'L')
			{
				myChild = OutGetPointer(bufferNum, 11);
				bufferNum = buf.getBlockNum(this->indexname, myChild);
			}
			offset = 0;
			return 1;

		}
	}
	else
	{
		if (buf.bufferBlock[bufferNum].value[1] == 'L')
		{
			bufferNum = bufferNum;
			return 0;
		}
		else
		{
			myChild = OutGetPointer(bufferNum, 16 + (offset-1)*(treeIndex.columnLength + 5) + treeIndex.columnLength);
			bufferNum = buf.getBlockNum(this->indexname, myChild);
			return IndexManager<KeyType>::findToLeaf(mykey, offset, bufferNum);
		}
	}
}

template <class KeyType>
void IndexManager<KeyType>::InitializeRoot()
{
	int bufferNum =buf.addBlockInFile(this->treeIndex);
	this->isLeaf = 1;
	this->rootL(bufferNum);
	this->rootL.columnLength = treeIndex.columnLength;
	this->rootL.isRoot = 1;
	this->rootL.degree = treeIndex.degree;
	this->rootL.writeBack();
	buf.bufferBlock[bufferNum].Lock = 1;
	buf.flashBack(bufferNum);
	return;
}
//把一个节点从buffer读到节点中只需要node(bufferNum)即可，写回去只要node.writeBack()即可，将一个buffer写到文件只要处理Lock和buf.flashBack(bufferNum)即可
//把文件中的block写到buffer中只要getBlockNum(string filename, int blockOffset)即可，若要在文件中新加block只要addBlockInFile()在处理新加index节点的isRoot和columnLength即可

//每次把节点写回buffer中之后要考虑是否还要用这个节点来决定是否锁住这个buffer
template <typename KeyType>
bool IndexManager<KeyType>::insertIndex(KeyType mykey, int blockNum, int blockOffset)
{
	if (this->rootL.key.empty() && this->rootB.key.empty())
		this->InitializeRoot();

	int offset;
	int bufferNum=0;
	if (this->findToLeaf(mykey, offset, bufferNum))
	{
		this->insertLeaf(mykey, blockNum, blockOffset, bufferNum, offset);
		int recordNum = getRecordNum(bufferNum);
		if (recordNum == treeIndex.degree)
		{
			this->adjustafterinsert(bufferNum);
		}
		return 1;
	}
	else
	{
		cout << "Error:in insert key to index: the duplicated key!" << endl;
		return 0;
	}
}

int getRecordNum(int bufferNum)
{
	int recordNum = 0;
	for (int i = 2; i<6; i++){
		if (buf.bufferBlock[bufferNum].value[i] == EMPTY) break;
		recordNum = 10 * recordNum + buf.bufferBlock[bufferNum].value[i] - '0';
	}
	return recordNum;

}

template <class KeyType>
bool IndexManager<KeyType>::insertLeaf(KeyType mykey, int blockNum, int blockOffset, int bufferNum, int offset)
{
	int cou;
	if (buf.bufferBlock[bufferNum].value[1] != 'L')
	{
		cout << "Error:insertLeaf(const KeyType &mykey,const offset val) is a function for leaf nodes" << endl;
		return 0;
	}

	Leaf<KeyType> myleaf(bufferNum, this->treeIndex);
	if (myleaf.recordNum == 0)
	{
		myleaf.key.clear();
		myleaf.key.push_back(mykey);
		myleaf.POS.clear();
		myleaf.POS.push_back(recordPosition(blockNum, blockOffset));
		myleaf.recordNum++;
		buf.bufferBlock[bufferNum].Written();
		return 1;
	}
	else
	{
		typename vector<KeyType>::iterator kit = myleaf.key.begin();
		vector<recordPosition>::iterator pit = myleaf.POS.begin();
		kit += offset;
		pit += offset;
		myleaf.key.insert(kit, mykey);
		myleaf.POS.insert(pit, recordPosition(blockNum, blockOffset));
		myleaf.recordNum++;
		myleaf.writeBack();
		
		if (offset == 0)
		{
			int NUM;
			KeyType oldkey = myleaf.key[1];
			int parent = myleaf.father;
			while (parent >= 0 && parent <= (this->treeIndex.degree - 1) && buf.bufferBlock[bufferNum].blockOffset == OutGetPointer(NUM = buf.getBlockNum(this->indexname, parent), 11))
			{
				parent = OutGetPointer(NUM, 6);
			    bufferNum = NUM;
			}
			if (parent >= 0 && parent <= (this->treeIndex.degree - 1))
			{
				int set;
				if (this->search(mykey,set,bufferNum))
					writeValue<KeyType>(bufferNum, 16 + set*(5 + this->treeIndex.columnLength), this->treeIndex.columnLength, mykey);
				else
					cout << "Error:in insert key to leaf: fail to update the ancestor node!" << endl;
			}
				
		}
		buf.bufferBlock[bufferNum].Written();
		if (getRecordNum(myleaf.bufferNum) >= (this->treeIndex.degree - 1))
			this->adjustafterinsert(myleaf.bufferNum);
		return 1;
	}
	
}

template <class KeyType>
bool IndexManager<KeyType>::insertLeaf(KeyType mykey, int blockNum, int blockOffset, int leafBlock)
{
	int bufferNum = buf.getBlockNum(this->indexname, leafBlock);
	int offset;
	if (search(mykey, offset, bufferNum))
	{
		cout << "Error:in insert key to branch: the duplicated key!" << endl;
		return 0;
	}
	else
	{
		this->insertLeaf<KeyType>(mykey, blockNum, blockOffset, bufferNum, offset);
	}
	return 1;
}

template <class KeyType>
bool IndexManager<KeyType>::insertBranch(KeyType mykey, int child, int bufferNum, int offset)
{
	int cou;
	if (buf.bufferBlock[bufferNum].value[1] != '_')
	{
		cout << "Error:insertBranch(const KeyType &mykey,const offset val) is a function for Branch nodes" << endl;
		return 0;
	}
	
	Branch<KeyType> mybranch(bufferNum, this->treeIndex);
	if (mybranch.recordNum == 0)
	{
		mybranch.key.clear();
		mybranch.key.push_back(mykey);
		mybranch.child.push_back(child);
		mybranch.recordNum++;
		buf.bufferBlock[bufferNum].Written();
		return 1;
	}
	else
	{
		typename vector<KeyType>::iterator kit = mybranch.key.begin();
		vector<POINTER>::iterator cit = mybranch.child.begin();
		kit += offset;
		cit += offset;
		mybranch.key.insert(kit, mykey);
		mybranch.child.insert(cit, child);
		mybranch.recordNum++;
		mybranch.writeBack();

		if (getRecordNum(mybranch.bufferNum) >= (this->treeIndex.degree - 1))
			this->adjustafterinsert(mybranch.bufferNum);
		return 1;
	}
}

template <class KeyType>
bool IndexManager<KeyType>::insertBranch(KeyType mykey, int child, int branchBlock)
{
	int bufferNum = buf.getBlockNum(this->indexname, branchBlock);
	int offset;
	if (search(mykey,offset,bufferNum))
	{
		cout << "Error:in insert key to branch: the duplicated key!" << endl;
		return 0;
	}
	else
	{
		this->insertBranch<KeyType>(mykey, child, bufferNum, offset);
		
	}
	return 1;
}

template <class KeyType>
void IndexManager<KeyType>::adjustafterinsert(int bufferNum)
{
	KeyType mykey;
	if (getRecordNum(bufferNum) <= (this->treeIndex.degree - 1))//record number is too great, need to split
		return;

	if (buf.bufferBlock[bufferNum].value[1] == 'L')
	{
		Leaf<KeyType> leaf(bufferNum,this->treeIndex);
		if (leaf.isRoot)
		{//this leaf is a root
			int fbufferNum = buf.addBlockInFile(this->treeIndex);	//find a new place for old leaf
			int sbufferNum = buf.addBlockInFile(this->treeIndex);	// buffer number for sibling 

			Branch<KeyType> branchRoot(fbufferNum);	//new root, which is branch indeed
			Leaf<KeyType> leafadd(sbufferNum);	//sibling

			//is root
			branchRoot.isRoot = 1;
			leafadd.isRoot = 0;
			leaf.isRoot = 0;

			branchRoot.father = -1;
			leafadd.father = leaf.father = buf.bufferBlock[fbufferNum].blockOffset;
			branchRoot.columnLength = leafadd.columnLength = leaf.columnLength;
			branchRoot.degree = leafadd.degree = leaf.degree;

			//link the newly added leaf block in the link list of leaf
			leafadd.lastleaf = buf.bufferBlock[leaf.bufferNum].blockOffset;
			leaf.lastleaf = -1;
			leafadd.nextleaf = -1;
			leaf.nextleaf = buf.bufferBlock[leafadd.bufferNum].blockOffset;
			branchRoot.child.push_back(buf.bufferBlock[leaf.bufferNum].blockOffset);
			branchRoot.child.push_back(buf.bufferBlock[leafadd.bufferNum].blockOffset);
			branchRoot.recordNum++;
			KeyType ktmp;
			recordPosition rtmp;
			while (leafadd.key.size() < leaf.key.size())
			{
				ktmp = leaf.key.back();
				rtmp = leaf.POS.back();
				leaf.key.pop_back();
				leaf.POS.pop_back();
				leafadd.key.insert(leafadd.key.begin(), ktmp);
				leafadd.POS.insert(leafadd.POS.begin(), rtmp);
				leaf.recordNum--;
				leafadd.recordNum++;
			}
			branchRoot.key.push_back(ktmp);

			this->rootB = branchRoot;
			this->isLeaf = 0;
			buf.bufferBlock[leaf.bufferNum].blockOffset = buf.bufferBlock[this->rootB.bufferNum].blockOffset;
			buf.bufferBlock[this->rootB.bufferNum].blockOffset = 0;
			this->rootB.writeBack();
			leaf.writeBack();
			leafadd.writeBack();
			this->treeIndex.blockNum += 2;
		}
		else
		{//this leaf is not a root, we have to cascade up
			int sbufferNum = buf.addBlockInFile(this->treeIndex);
			Leaf<KeyType> leafadd(sbufferNum);

			leafadd.isRoot = 0;

			leafadd.father = leaf.father;
			leafadd.columnLength = leaf.columnLength;
			leafadd.degree = leaf.degree;

			//link the newly added leaf block in the link list of leaf
			leafadd.lastleaf = buf.bufferBlock[leaf.bufferNum].blockOffset;		
			leafadd.nextleaf = leaf.nextleaf;
			leaf.nextleaf = buf.bufferBlock[leafadd.bufferNum].blockOffset;

			KeyType ktmp;
			recordPosition rtmp;
			while (leafadd.key.size() < leaf.key.size())
			{
				ktmp = leaf.key.back();
				rtmp = leaf.POS.back();
				leaf.key.pop_back();
				leaf.POS.pop_back();
				leafadd.key.insert(leafadd.key.begin(), ktmp);
				leafadd.POS.insert(leafadd.POS.begin(), rtmp);
				leaf.recordNum--;
				leafadd.recordNum++;
			}
			leaf.writeBack();
			leafadd.writeBack();

			this->insertBranch(ktmp, buf.bufferBlock[leafadd.bufferNum].blockOffset, leaf.father);
			this->treeIndex.blockNum++;
		}
	}
	else//需要调整的节点是个branch
	{
		Branch<KeyType> branch(bufferNum, this->treeIndex);
		if (branch.isRoot)//this branch is a root
		{
			int fbufferNum = buf.addBlockInFile(this->treeIndex);	//find a new place for old branch
			int sbufferNum = buf.addBlockInFile(this->treeIndex);	// buffer number for sibling 

			Branch<KeyType> branchRoot(fbufferNum);	//new root, which is branch indeed
			Branch<KeyType> branchadd(sbufferNum);	//sibling

			//is root
			branchRoot.isRoot = 1;
			branchadd.isRoot = 0;
			branch.isRoot = 0;

			branchRoot.father = -1;
			branchadd.father = branch.father = buf.bufferBlock[fbufferNum].blockOffset;
			branchRoot.columnLength = branchadd.columnLength = branch.columnLength;
			branchRoot.degree = branchadd.degree = branch.degree;

			//link the newly added branch block in the link list of branch
			branchRoot.child.push_back(buf.bufferBlock[branch.bufferNum].blockOffset);
			branchRoot.child.push_back(buf.bufferBlock[branchadd.bufferNum].blockOffset);
			branchRoot.recordNum++;
			KeyType ktmp;
			POINTER ctmp;
			while (branchadd.key.size() < branch.key.size())
			{
				ktmp = branch.key.back();
				ctmp = branch.child.back();
				branch.key.pop_back();
				branch.child.pop_back();
				branchadd.key.insert(branchadd.key.begin(), ktmp);
				branchadd.child.insert(branchadd.child.begin(), ctmp);
				branch.recordNum--;
				branchadd.recordNum++;
			}
			branchadd.recordNum--;
			branchadd.key.erase(branchadd.key.begin());
			branchRoot.key.push_back(ktmp);

			this->rootB = branchRoot;
			this->isLeaf = 0;
			buf.bufferBlock[branch.bufferNum].blockOffset = buf.bufferBlock[this->rootB.bufferNum].blockOffset;
			buf.bufferBlock[this->rootB.bufferNum].blockOffset = 0;
			this->rootB.writeBack();
			branch.writeBack();
			branchadd.writeBack();
			this->treeIndex.blockNum += 2;
		}
		else//this branch is not a root, we have to cascade up
		{
			int sbufferNum = buf.addBlockInFile(this->treeIndex);
			Branch<KeyType> branchadd(sbufferNum);

			branchadd.isRoot = 0;

			branchadd.father = branch.father;
			branchadd.columnLength = branch.columnLength;
			branchadd.degree = branch.degree;

			//link the newly added branch block in the link list of branch

			KeyType ktmp;
			POINTER ctmp;
			while (branchadd.key.size() < branch.key.size())
			{
				ktmp = branch.key.back();
				ctmp = branch.child.back();
				branch.key.pop_back();
				branch.child.pop_back();
				branchadd.key.insert(branchadd.key.begin(), ktmp);
				branchadd.child.insert(branchadd.child.begin(), ctmp);
				branch.recordNum--;
				branchadd.recordNum++;
			}
			branchadd.recordNum--;
			branchadd.key.erase(branchadd.key.begin());
			branch.writeBack();
			branchadd.writeBack();

			this->insertBranch(ktmp, buf.bufferBlock[branchadd.bufferNum].blockOffset, branch.father);
			this->treeIndex.blockNum++;
		}
	}

}

template <class KeyType>
void IndexManager<KeyType>::creatIndex()
{
	InitializeRoot();

	//retrieve datas of the table and form a B+ Tree
	string tablename = this->treeTable.tableName + ".table";
	string stringrow;
	KeyType mykey;

	int length = tableinfor.totalLength + 1;//table中的record会加多一位来判断这条记录是否被删除了,而index中的record不会
	const int recordNum = BLOCKSIZE / length;

	//read datas from the record and sort it into a B+ Tree and store it
	for (int blockOffset = 0; blockOffset < this->treeTable.blockNum; blockOffset++){
		int bufferNum = buf.getBlockNum(tablename, blockOffset);
		for (int offset = 0; offset < recordNum; offset++){
			int position = offset * length;
			stringrow = buf.bufferBlock[bufferNum].getvalue(position, position + length);
			if (stringrow.c_str()[0] == EMPTY) continue;//inticate that this row of record have been deleted
			stringrow.erase(stringrow.begin());	//把第一位去掉//现在stringrow代表一条记录
			mykey = getValue<KeyType>(bufferNum, position + getKeyPosition(), this->treeIndex.columnLength);
			insertIndex(mykey, blockOffset, offset);
		}
	}
}

template <class KeyType>
int IndexManager<KeyType>::getKeyPosition()//从记录row中提取key
{
	int s_pos = 0, f_pos = 0;	//start position & finish position
	for (int i = 0; i <= this->treeIndex.column; i++){
		s_pos = f_pos;
		f_pos += this->treeTable.attribute[i].length;
	}
	return s_pos;
}

template <class KeyType>
void dropIndex(Index& indexinfor)
{
	string filename = indexinfor.index_name + ".index";
    buf.setInvalid(filename);
	remove(filename);
}


template <class KeyType>
bool IndexManager<KeyType>::deleteIndex(KeyType mykey)
{
	if (this->rootB.key.empty() && this->rootL.key.empty())
	{
		cout << "ERROR: In deleteKey, no nodes in the tree " << this->indexname << "!" << endl;
		return 0;
	}
	int offset;
	int bufferNum;
	if (this->isLeaf)
		bufferNum = this->rootL.bufferNum;
	else
		bufferNum = this->rootB.bufferNum;

	if (!findToLeaf(mykey, int& offset, int& bufferNum))
	{
		cout << "ERROR: In deleteKey, no key in the tree " << this->indexname << "!" << endl;
		return 0;
	}

	if (buf.bufferBlock[bufferNum].value[1] == 'L')
	{
		this->deleteLeaf(mykey, bufferNum, offset);
		return 1;
	}
	else
	{
		this->deleteBranch(mykey, bufferNum, offset);
		return 1;
	}
}

template <class KeyType>
bool IndexManager<KeyType>::deleteBranch(KeyType mykey, int branchBlock)
{
	int bufferNum = buf.getBlockNum(this->indexname, branchBlock);
	int offset;
	if (search(mykey, offset, bufferNum))
	{
		cout << "Error:in insert key to branch: the duplicated key!" << endl;
		return 0;
	}
	else
	{
		this->deleteBranch<KeyType>(mykey, bufferNum, offset);
	}
	return 1;
}

template <class KeyType>
bool IndexManager<KeyType>::deleteBranch(KeyType mykey, int bufferNum, int offset)
{
	int cou;
	if (buf.bufferBlock[bufferNum].value[1] == 'L')
	{
		cout << "Error:insertBranch(const KeyType &mykey,const offset val) is a function for branch nodes" << endl;
		return 0;
	}

	Branch<KeyType> mybranch(bufferNum, this->treeIndex);
	if (mybranch.key.size()<(offset + 1) || mybranch.key[offset] != mykey)
	{
		cout << "Error:In remove(size_t index), can not find the key!" << endl;
		return 0;
	}

	typename vector<KeyType>::iterator kit = mybranch.key.begin();
	vector<POINTER>::iterator cit = mybranch.child.begin();
	kit += offset;
	cit += offset;
	mybranch.key.erase(kit);
	mybranch.child.erase(cit);
	mybranch.recordNum--;
	mybranch.writeBack();
	if (getRecordNum(mybranch.bufferNum) >= (this->treeIndex.degree - 1))
		this->adjustafterdelete(mybranch.bufferNum);
}

template <class KeyType>
bool IndexManager<KeyType>::deleteLeaf(KeyType mykey, int leafBlock)
{
	int bufferNum = buf.getBlockNum(this->indexname, leafBlock);
	int offset;
	if (search(mykey, offset, bufferNum))
	{
		cout << "Error:in insert key to branch: the duplicated key!" << endl;
		return 0;
	}
	else
	{
		this->deleteLeaf<KeyType>(mykey, bufferNum, offset);
	}
	return 1;
}

template <class KeyType>
void IndexManager<KeyType>::deleteNode(int bufferNum)
{
	int lastblock = buf.getBlockNum(this->indexname, this->treeIndex.blockNum - 1);
	if (lastblock == bufferNum)
	{
		this->treeIndex.blockNum--;
		return;
	}

	if (buf.bufferBlock[bufferNum].value[1] == 'L')//这是个叶子
	{
		
		if (buf.bufferBlock[lastblock].value[1] == 'L')
		{
			if (buf.bufferBlock[lastblock].value[0] == 'R')
				buf.bufferBlock[lastblock].blockOffset = buf.bufferBlock[bufferNum].blockOffset;
			else
			{
				int offset;
				int index;
				Leaf<KeyType> lastleaf(lastblock, this->treeIndex);
				int fbufferNum = buf.getBlockNum(this->indexname, lastleaf.father);
				Branch<KeyType> parent(fbufferNum, this->treeIndex);
				parent.search(lastleaf.key[0], offset);
				if (offset == 0 && parent.key[0] > lastleaf.key[0])
					index = 0;
				else
					index = offset + 1;
				parent.child[index] = buf.bufferBlock[bufferNum].blockOffset;
				parent.writeBack();
				buf.bufferBlock[lastblock].blockOffset = buf.bufferBlock[bufferNum].blockOffset;
				lastleaf.writeBack();
				buf.bufferBlock[bufferNum].initialize();
				this->treeIndex.blockNum--;
			}
		}
		else
		{
			int offset;
			int index;
			Branch<KeyType> lastbranch(lastblock, this->treeIndex);
			int fbufferNum = buf.getBlockNum(this->indexname, lastbranch.father);
			Branch<KeyType> parent(fbufferNum, this->treeIndex);
			parent.search(lastbranch.key[0], offset);
			index = offset;
			parent.child[index] = buf.bufferBlock[bufferNum].blockOffset;
			parent.writeBack();
			vector<int>::iterator it= lastbranch.child.begin();
			for (; it != lastbranch.child.end(); it++)
			{
				int cbufferNum = buf.getBlockNum(this->indexname, *it);
				OutSetPointer(cbufferNum, 6, buf.bufferBlock[bufferNum].blockOffset);
			}
			
			buf.bufferBlock[lastblock].blockOffset = buf.bufferBlock[bufferNum].blockOffset;
			lastbranch.writeBack();
			buf.bufferBlock[bufferNum].initialize();
			this->treeIndex.blockNum--;
		}
			
	}
	else//it's a branch
	{
		
		if (buf.bufferBlock[lastblock].value[1] == 'L')
		{
			if (buf.bufferBlock[lastblock].value[0] == 'R')
				buf.bufferBlock[lastblock].blockOffset = buf.bufferBlock[bufferNum].blockOffset;
			else
			{
				int offset;
				int index;
				Leaf<KeyType> lastleaf(lastblock, this->treeIndex);
				int fbufferNum = buf.getBlockNum(this->indexname, lastleaf.father);
				Branch<KeyType> parent(fbufferNum, this->treeIndex);
				parent.search(lastleaf.key[0], offset);
				if (offset == 0 && parent.key[0] > lastleaf.key[0])
					index = 0;
				else
					index = offset + 1;
				parent.child[index] = buf.bufferBlock[bufferNum].blockOffset;
				parent.writeBack();
				buf.bufferBlock[lastblock].blockOffset = buf.bufferBlock[bufferNum].blockOffset;
				lastleaf.writeBack();
				buf.bufferBlock[bufferNum].initialize();
				this->treeIndex.blockNum--;
			}
		}
		else
		{
			int offset;
			int index;
			Branch<KeyType> lastbranch(lastblock, this->treeIndex);
			int fbufferNum = buf.getBlockNum(this->indexname, lastbranch.father);
			Branch<KeyType> parent(fbufferNum, this->treeIndex);
			parent.search(lastbranch.key[0], offset);
			index = offset;
			parent.child[index] = buf.bufferBlock[bufferNum].blockOffset;
			parent.writeBack();
			vector<int>::iterator it = lastbranch.child.begin();
			for (; it != lastbranch.child.end(); it++)
			{
				int cbufferNum = buf.getBlockNum(this->indexname, *it);
				OutSetPointer(cbufferNum, 6, buf.bufferBlock[bufferNum].blockOffset);
			}

			buf.bufferBlock[lastblock].blockOffset = buf.bufferBlock[bufferNum].blockOffset;
			lastbranch.writeBack();
			buf.bufferBlock[bufferNum].initialize();
			this->treeIndex.blockNum--;
		}
	}
}

template <class KeyType>
bool IndexManager<KeyType>::deleteLeaf(KeyType mykey, int bufferNum, int offset)
{
	int cou;
	if (buf.bufferBlock[bufferNum].value[1] != 'L')
	{
		cout << "Error:insertLeaf(const KeyType &mykey,const offset val) is a function for leaf nodes" << endl;
		return 0;
	}

	Leaf<KeyType> myleaf(bufferNum, this->treeIndex);
	if (myleaf.key.size()<(offset+1) || myleaf.key[offset]!=mykey)
	{
		cout << "Error:In remove(size_t index), can not find the key!" << endl;
		return 0;
	}

	typename vector<KeyType>::iterator kit = myleaf.key.begin();
	vector<recordPosition>::iterator rit = myleaf.POS.begin();
	kit += offset;
	rit += offset;
	myleaf.key.erase(kit);
	myleaf.POS.erase(rit);
	myleaf.recordNum--;
	myleaf.writeBack();
	if (offset == 0)
	{
		int NUM;
		KeyType newkey = myleaf.key[0];
		int parent = myleaf.father;
		while (parent >= 0 && parent <= (this->treeIndex.degree - 1) && buf.bufferBlock[bufferNum].blockOffset == OutGetPointer(NUM = buf.getBlockNum(this->indexname, parent), 11))
		{
			parent = OutGetPointer(NUM, 6);
			bufferNum = NUM;
		}
		if (parent >= 0 && parent <= (this->treeIndex.degree - 1))
		{
			int set;
			if (this->search(mykey, set, bufferNum))
				writeValue<KeyType>(bufferNum, 16 + set*(5 + this->treeIndex.columnLength), this->treeIndex.columnLength, mykey);
			else
				cout << "Error:in insert key to leaf: fail to update the ancestor node!" << endl;
		}
	}
	buf.bufferBlock[bufferNum].Written();
	if (getRecordNum(myleaf.bufferNum) >= (this->treeIndex.degree - 1))
		this->adjustafterdelete(myleaf.bufferNum);
}

template <class KeyType>
void IndexManager<KeyType>::adjustafterdelete(int bufferNum)//需要维护：blockNum;leafhead;
{
	int min_leaf = this->treeIndex.degree/2;
	int min_branch = (this->treeIndex.degree - 1) / 2;
	if ((buf.bufferBlock[bufferNum].value[1] == 'L' && getRecordNum(bufferNum) >= min_leaf) || (buf.bufferBlock[bufferNum].value[1] != 'L' && getRecordNum(bufferNum) >= min_branch))
		return 1;

	if (buf.bufferBlock[bufferNum].value[1] == 'L')
	{
		Leaf<KeyType> myleaf(bufferNum, this->treeIndex);
		if (myleaf.isRoot)
		{
			if (myleaf.recordNum > 0)
				return 1;
			else
			{
				this->treeIndex.blockNum = 0;
				this->InitializeRoot();
				return 1;
			}
		}
		else
		{
			int offset;
			int fbufferNum = buf.getBlockNum(this->indexname, myleaf.father);
			Branch<KeyType> parent(fbufferNum, this->treeIndex);
			parent.search(myleaf.key[0], offset);
			if (offset != 0 && (offset == parent.recordNum-1))//choose the left brother to merge or replace
			{
				int sbufferNum = buf.getBlockNum(this->indexname, parent.child[offset]);
				Leaf<KeyType> brother(sbufferNum, this->treeIndex);
				if (brother.recordNum > min_leaf)// choose the most right key of brother to add to the left hand of the pnode
				{
					myleaf.key.insert(myleaf.key.begin(), brother.key[brother.recordNum-1]);

					myleaf.POS.insert(myleaf.POS.begin(), brother.POS[brother.recordNum - 1]);
					myleaf.recordNum++;
					brother.recordNum--;
					parent.key[offset] = myleaf.key[0];
					brother.key.pop_back();
					brother.POS.pop_back();
					myleaf.writeBack();
					brother.writeBack();
					parent.writeBack();
				}
				else// merge the node with its left brother
				{
					parent.writeBack();
					this->deleteBranch(myleaf.key[0], parent.bufferNum, parent.recordNum - 1);

					while (!myleaf.key.empty())
					{
						brother.key.push_back(myleaf.key[0]);
						brother.POS.push_back(myleaf.POS[0]);
						myleaf.key.erase(myleaf.key.begin());
						myleaf.POS.erase(myleaf.POS.begin());
						brother.recordNum++;
						myleaf.recordNum--;

					}
					brother.nextleaf = myleaf.nextleaf;
					
					brother.writeBack();
					myleaf.writeBack();
					
					this->deleteNode(myleaf.bufferNum);
					this->adjustafterdelete(parent.bufferNum);
					return 1;
				}

			}
			else//choose the right brother to merge or replace
			{
				int sbufferNum;
				int index;
				if (parent.key[0] > myleaf.key[0])
				{
					index = 1;
				}
				else
				{
					index = offset + 2;
				}
				sbufferNum = buf.getBlockNum(this->indexname, parent.child[index]);
				Leaf<KeyType> brother(sbufferNum, this->treeIndex);
				
				if (brother.recordNum > min_leaf)//// choose the most left key of right brother to add to the right hand of the node
				{
					KeyType mykey=brother.key[0];
					recordPosition myPOS=brother.POS[0];
					int RECORDNUM=myleaf.recordNum;
					this->deleteLeaf(mykey, sbufferNum, 0);
					this->insertLeaf(mykey, myPOS.blockNum, myPOS.blockPosition, sbufferNum, RECORDNUM);

				}// end add
				else // merge the node with its right brother
				{
					parent.writeBack();
					this->deleteBranch(brother.key[0], parent.bufferNum, index - 1);
					while (!brother.key.empty())
					{
						myleaf.key.push_back(brother.key[0]);
						myleaf.POS.push_back(brother.POS[0]);
						brother.key.erase(brother.key.begin());
						brother.POS.erase(brother.POS.begin());
						brother.recordNum--;
						myleaf.recordNum++;

					}
					myleaf.nextleaf = brother.nextleaf;

					brother.writeBack();
					myleaf.writeBack();
					this->deleteNode(brother.bufferNum);
					this->adjustafterdelete(parent.bufferNum);
					return 1;
				}
			}

		}
	}
	else
	{
		Branch<KeyType> mybranch(bufferNum, this->treeIndex);
		if (mybranch.isRoot)
		{
			if (mybranch.recordNum > 0)
				return 1;
			else
			{
				int cbufferNum = buf.getBlockNum(this->indexname, mybranch.child[0]);
				if (buf.bufferBlock[cbufferNum].value[1] == 'L')
				{
					Leaf<KeyType> childleaf(cbufferNum, this->treeIndex);
					buf.bufferBlock[mybranch.bufferNum].blockOffset = buf.bufferBlock[cbufferNum].blockOffset;
					buf.bufferBlock[cbufferNum].blockOffset = 0;
					Branch<KeyType> childbranch(cbufferNum, this->treeIndex);
					childbranch.father = -1;
					childbranch.isRoot = 1;
					this->rootB = childbranch; 
					this->isLeaf = 1;

					childbranch.writeBack();
					mybranch.isRoot = 0;
					mybranch.writeBack();
					this->deleteNode(mybranch.bufferNum);
				}
				else
				{
					buf.bufferBlock[mybranch.bufferNum].blockOffset = buf.bufferBlock[cbufferNum].blockOffset;
					buf.bufferBlock[cbufferNum].blockOffset = 0;
					Branch<KeyType> childbranch(cbufferNum, this->treeIndex);
					childbranch.father = -1;
					childbranch.isRoot = 1;
					vector<int>::iterator it = childbranch.child.begin();
					for (; it != childbranch.child.end(); it++)
					{
						int ccbufferNum = buf.getBlockNum(this->indexname, *it);
						OutSetPointer(ccbufferNum, 6, buf.bufferBlock[cbufferNum].blockOffset);
					}
					this->rootB = childbranch;
					childbranch.writeBack();
					mybranch.isRoot = 0;
					mybranch.writeBack();
					this->deleteNode(mybranch.bufferNum);
				}
			}
		}
		else//调整非根节点的branch节点
		{
			size_t index = 0;
			if (pnode->count == 0)
			{
				for (index = 0; index <= pnode->count; index++)
				{
					if (pnode->parent->child[index] == pnode)
						break;
				}
			}
			else
			{
				par->search(pnode->key[0], index);
			}

			int offset=0;
			int fbufferNum = buf.getBlockNum(this->indexname, mybranch.father);
			Branch<KeyType> parent(fbufferNum, this->treeIndex);
			if (mybranch.recordNum == 0)
			{
				for (offset = 0; offset <= parent.recordNum; offset++)
				{
					if (parent.child[offset] == buf.bufferBlock[mybranch.bufferNum].blockOffset)
						break;
				}
			}
			else
			{
				parent.search(mybranch.key[0], offset);
			}
			
			if (offset != 0 && (offset == parent.recordNum))//choose the left brother to merge or replace
			{
				int sbufferNum = buf.getBlockNum(this->indexname, parent.child[offset-1]);
				Branch<KeyType> brother(sbufferNum, this->treeIndex);
				if (brother.recordNum > min_branch)// choose the most right key of brother to add to the left hand of the pnode
				{
					mybranch.key.insert(mybranch.key.begin(), parent.key[offset-1]);
					int childblock = brother.child[brother.recordNum];
					mybranch.child.insert(mybranch.child.begin(), brother.child[brother.recordNum]);
					mybranch.recordNum++;
					brother.recordNum--;
					parent.key[offset - 1] = brother.key[brother.recordNum - 1];
					brother.key.pop_back();
					brother.child.pop_back();
					int cbufferNum = buf.getBlockNum(this->indexname, childblock);
					OutSetPointer(cbufferNum, 6, buf.bufferBlock[mybranch.bufferNum].blockOffset);

					mybranch.writeBack();
					brother.writeBack();
					parent.writeBack();
				}
				else// merge the node with its left brother
				{
					parent.writeBack();
					KeyType parentKey=parent.key[parent.recordNum-1];
					this->deleteBranch(parentKey, parent.bufferNum, parent.recordNum - 1);

					brother.key.push_back(parentKey);
					int childblock = mybranch.child[0];
					brother.child.push_back(mybranch.child[0]);
					mybranch.child.erase(mybranch.child.begin());
					brother.recordNum++;
					int cbufferNum = buf.getBlockNum(this->indexname, childblock);
					OutSetPointer(cbufferNum, 6, buf.bufferBlock[brother.bufferNum].blockOffset);
					while (!mybranch.key.empty())
					{
						brother.key.push_back(mybranch.key[0]);
						brother.child.push_back(mybranch.child[0]);
						childblock = mybranch.child[0];
						mybranch.key.erase(mybranch.key.begin());
						mybranch.child.erase(mybranch.child.begin());
						
						cbufferNum = buf.getBlockNum(this->indexname, childblock);
						OutSetPointer(cbufferNum, 6, buf.bufferBlock[brother.bufferNum].blockOffset);
						brother.recordNum++;
						mybranch.recordNum--;

					}

					brother.writeBack();
					mybranch.writeBack();

					this->deleteNode(mybranch.bufferNum);
					return 1;
				}

			}
			else//choose the right brother to merge or replace
			{

				int sbufferNum = buf.getBlockNum(this->indexname, parent.child[offset + 1]);
				Branch<KeyType> brother(sbufferNum, this->treeIndex);

				if (brother.recordNum > min_branch)//// choose the most left key of right brother to add to the right hand of the node
				{
					mybranch.key.push_back(parent.key[offset]);
					int childblock = brother.child[0];
					mybranch.child.push_back(brother.child[0]);
					mybranch.recordNum++;
					brother.recordNum--;
					parent.key[offset] = brother.key[0];
					brother.key.erase(brother.key.begin());
					brother.child.erase(brother.key.begin());
					int cbufferNum = buf.getBlockNum(this->indexname, childblock);
					OutSetPointer(cbufferNum, 6, buf.bufferBlock[mybranch.bufferNum].blockOffset);

					mybranch.writeBack();
					brother.writeBack();
					parent.writeBack();

				}// end add
				else // merge the node with its right brother
				{
					parent.writeBack();
					KeyType parentKey = parent.key[offset];
					this->deleteBranch(parentKey, parent.bufferNum, offset);

					mybranch.key.push_back(parentKey);
					int childblock = brother.child[0];
					mybranch.child.push_back(brother.child[0]);
					brother.child.erase(brother.child.begin());
					mybranch.recordNum++;
					int cbufferNum = buf.getBlockNum(this->indexname, childblock);
					OutSetPointer(cbufferNum, 6, buf.bufferBlock[mybranch.bufferNum].blockOffset);

					while (!brother.key.empty())
					{
						mybranch.key.push_back(brother.key[0]);
						mybranch.child.push_back(brother.child[0]);
						childblock = brother.child[0];
						brother.key.erase(brother.key.begin());
						brother.child.erase(brother.child.begin());

						cbufferNum = buf.getBlockNum(this->indexname, childblock);
						OutSetPointer(cbufferNum, 6, buf.bufferBlock[mybranch.bufferNum].blockOffset);
						brother.recordNum--;
						mybranch.recordNum++;

					}

					brother.writeBack();
					mybranch.writeBack();

					this->deleteNode(brother.bufferNum);
					return 1;
				}
			}
		}
	}


}


template <class KeyType>
recordPosition IndexManager<KeyType>::selectEqual(KeyType mykey)
{
	int offset;
	int bufferNum = 0;
	recordPosition res(-1,0);
	if (this->findToLeaf<KeyType>(mykey, offset, bufferNum))
	{
		Leaf<KeyType> myleaf(bufferNum, this->treeIndex);
		res.blockNum = myleaf.POS[offset].blockNum;
		res.blockPosition = myleaf.POS[offset].blockPosition;
		return res;
	}
	else
		return res;
}

template <class KeyType>
vector<recordPosition>& IndexManager<KeyType>::selectBetween(KeyType mykey1, KeyType mykey2)//大于等于mykey1,小于mykey2
{
	int offset;
	int bufferNum = 0;
	vector<recordPosition> RESULT;
	
	this->findToLeaf<KeyType>(mykey1, offset, bufferNum);
	Leaf<KeyType> myleaf(bufferNum, this->treeIndex);
	while ((offset + 1) <= myleaf.recordNum && myleaf.key[offset]<mykey2)
	{
		recordPosition res;
		res.blockNum = myleaf.POS[offset].blockNum;
		res.blockPosition = myleaf.POS[offset].blockPosition;
		RESULT.push_back(res);
		offset++;
	}
	int nextL = myleaf.nextleaf;
	while (nextL != -1)
	{
		bufferNum = buf.getBlockNum(this->indexname, nextL);
		offset = 0;
		Leaf<KeyType> myleaf(bufferNum, this->treeIndex);
		while ((offset + 1) <= myleaf.recordNum && myleaf.key[offset]<mykey2)
		{
			recordPosition res;
			res.blockNum = myleaf.POS[offset].blockNum;
			res.blockPosition = myleaf.POS[offset].blockPosition;
			RESULT.push_back(res);
			offset++;
		}
		if (myleaf.key[offset] < mykey2)
			nextL = myleaf.nextleaf;
		else
			nextL = -1;

		
	}
	return RESULT;
}























