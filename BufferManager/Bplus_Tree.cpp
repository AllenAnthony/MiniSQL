#include"Bplus_Tree.h"
using namespace std;

/*
template <typename KeyType> class TreeNode;
template <typename KeyType> class BplusTree;

static BufferManager BM;
typedef int offset; // the value of the tree node
typedef unsigned int size_t;


template <typename KeyType>
class TreeNode
{
private:
	unsigned int degree;

public:
	vector<KeyType> key;
	vector<TreeNode*> child;
	vector<offset> value;

	size_t count;  // the count of key

	TreeNode* parent;
	TreeNode* nextleaf;

	bool isleaf;

public:
	TreeNode(int degree, bool newLeaf = false);
	~TreeNode();

	bool isRoot();
	bool search(KeyType key, size_t &index);//在这个节点中搜索key，若存在则返回这个的key的编号，若不存在则返回这个key肯能存在的子节点的指针编号，返回通过index实现
	TreeNode* splite(KeyType &key);//根据key分裂这个节点，key将上移至parent节点，这个节点分为两个节点，返回那个新节点
	size_t add_branch(KeyType &key); //add the key in the branch and return the position
	size_t add_leaf(KeyType &key, offset val); // add a key-value in the leaf node and return the position
	bool remove(size_t index);




#ifdef _DEBUG
public:
	void debug_print();
#endif
};


template <typename KeyType>
class BplusTree
{
private:
	typedef TreeNode<KeyType>* Node;

	struct SearchNodeParse
	{
		Node PN; // a pointer pointering to the node containing the key
		size_t index; // the position of the key
		bool found; // the flag that whether the key is found.
	};

private:
	string filename;
	Node root;
	Node leafhead; // the head of the leaf node
	size_t keycount;
	size_t level;
	size_t nodecount;
	fileNode* file; // the filenode of this tree
	int keysize; // the size of key
	int degree;


private:
	void init_tree();// init the tree
	bool adjustafterinsert(Node pnode);
	bool adjustafterdelete(Node pnode);
	void FindToLeaf(Node pnode, KeyType key, SearchNodeParse &snp);

public:
	BplusTree(string filename, int degree);
	~BplusTree();

	offset search(KeyType& key); // search the value of specific key
	bool insertkey(KeyType &key, offset myval);
	bool deletekey(KeyType &key);

	void DropTree(Node node);
	void ReadFromDiskAll();
	void WriteBackToDiskAll();
	void ReadFromDisk(blockNode* BN);


#ifdef _DEBUG
public:
	void debug_print();

	void debug_print_node(Node pnode);
#endif

};
*/

/* the implement of BplusTree function */
//******** The definition of the functions of the class TreeNode **********

/**
* Constructor: create the tree node.
*
* @param int the degreee
* @param bool the flag that whether the node is a tree node or not
*
*/
template <typename KeyType>
TreeNode<KeyType>::TreeNode(int degree, bool isleaf = false) :degree(degree), isleaf(isleaf), count(0), parent(NULL), nextleaf(NULL)
{
	key.resize(degree, KeyType());
	
	if (isleaf)
		value.resize(degree, offset());
	else
		child.resize(degree + 1, NULL);

}

/**
* @Deconstructor
*
*/
template <typename KeyType>
TreeNode<KeyType>::~TreeNode()
{

}

/**
* Test if this node is the root or not.
*
* @return bool the flag that whether this node is the root or not
*
*/
template <typename KeyType>
bool TreeNode<KeyType>::isRoot()
{
	return (parent == NULL);
}


/**
* Search the key in the node
*
* @param KeyType
* @param size_t return the position of the node by reference
*
* @return bool the flag that whether the key exists in the node
*
*/
template <typename KeyType>
bool TreeNode<KeyType>::search(const KeyType& mykey, size_t &index)//在这个节点中搜索key，若存在则返回这个的key的编号，若不存在则返回这个key可能存在的子节点的指针编号，返回通过index实现
{
	int cou = 0;
	if (count == 0)
	{
		index = 0;
		return 0;
	}
	else if (count <= 20)// sequential search
	{
		for (cou = 0; cou < count; cou++)
		{
			if (mykey < key[cou])
			{
				index = cou;
				return 0;
			}
			else if (mykey == key[cou])
			{
				index = cou;
				return 1;
			}
			else
				continue;
		}
		index = count;
		return 0;
	}
	else// too many key, binary search. 2* log(n,2) < (1+n)/2
	{
		int start = 0;
		int tail = count - 1;
		int mid;
		if (mykey < key[0])
		{
			index = 0;
			return 0
		}
		else if (mykey>key[count - 1])
		{
			index = count;
			return 0;
		}
		while ((start + 1) < tail )
		{
			mid = (start + tail) / 2;
			if (mykey < key[mid])
			{
				tail = mid;
			}
			else if (mykey == key[mid])
			{
				index = mid;
				return 1;
			}
			else
			{
				start = mid;
			}
		}
		if (mykey == key[start])
		{
			index = start;
			return 1;
		}
		else if (mykey == key[tail])
		{
			index = tail;
			return 1;
		}
		else
		{
			index = tail;
			return 0;
		}


	}

	return 0;
}

//从中间分裂，中间值存入mykey，mykey将上移至parent节点(不由这个函数完成,若为leaf则新节点包含这个mykey,
//若为branch,则两个节点将都不包含mykey,这个函数也不提供是否需要分裂的判断)，这个节点分为两个节点，返回那个新节点
template <typename KeyType>
TreeNode<KeyType>* TreeNode<KeyType>::splite(KeyType &mykey)
{
	int cou;
	TreeNode* newnode = new TreeNode(this->degree, this->isleaf);
	if (newnode == NULL)
	{
		cout << "Problems in allocate momeory of TreeNode in splite node of " << mykey << endl;
		exit(1);
	}
	size_t mininum;

	if (this->isleaf)
	{
		mininum = degree / 2;
		mykey = key[mininum];
		for (cou = mininum; cou < count; cou++)
		{
			newnode->key[cou - mininum] = this->key[cou];
			this->key[cou] = KeyType();

			newnode->value[cou - mininum] = this->value[cou];
			this->value[cou] = offset();
		}		
		newnode->count = this->count - mininum;
		this->count = mininum;

		newnode->parent = this->parent;
		newnode->nextleaf = this->nextleaf;
		this->nextleaf = newnode;
	}
	else
	{
		mininum = (degree-1) / 2;
		mykey = key[mininum];
		for (cou = mininum+1; cou < this->count; cou++)
		{
			newnode->key[cou - mininum-1] = this->key[cou];
			this->key[cou] = KeyType();

			newnode->child[cou - mininum-1] = this->child[cou];
			this->child[cou] = NULL;
		}
		this->key[mininum] = KeyType();
		newnode->child[this->count - mininum - 1] = this->child[this->count];
		this->child[this->count] = NULL;s

		newnode->count = this->count - mininum;
		this->count = mininum;

		newnode->parent = this->parent;
		newnode->nextleaf = this->nextleaf;
		this->nextleaf = newnode;
	}

	return newnode;
}

/**
* Add the key in the branch node and return the position added.
*
* @param KeyType &
*
* @return size_t the position to insert
*
*/
template <typename KeyType>
size_t TreeNode<KeyType>::add_branch(const KeyType &mykey) //add the mykey in the branch and return the position
{
	if (isLeaf)
	{
		cout << "Error:add_branch(const KeyType &mykey) is a function for branch nodes" << endl;
		return -1;
	}
	if (count == 0)
	{
		key[0] = mykey;
		child[0] = NULL;
		child[1] = NULL;
		count++;
		return 0;
	}
	else //count > 0
	{
		size_t index = 0; // record the index of the tree
		bool exist = search(mykey, index);
		if (exist)
		{
			cout << "Error:In add(Keytype &mykey),mykey has already in the tree!" << endl;
			exit(1);
		}
		else // add the mykey into the node
		{
			for (size_t i = count; i > index; i--)
				key[i] = key[i - 1];
			key[index] = mykey;

			for (size_t i = count + 1; i > index + 1; i--)
				child[i] = child[i - 1];
			child[index + 1] = NULL; // this child will link to another node
			count++;

			if (index == 0)
			{
				int set;
				TreeNode<KeyType>* tmp = this;
				KeyType oldkey = tmp->key[1];
				while (tmp->parent && tmp == tmp->parent->child[0])
					tmp = tmp->parent;

				if (tmp->parent)
				{
					if (tmp->parent->search(oldkey, set))
						tmp->parent->key[set] = mykey;
					else
						cout << "Error:in insert key to branch: fail to update the ancestor node!" << endl;
				}
			}

			return index;
		}
	}
}

/**
* Add the key in the leaf node and return the position added.
*
* @param Keytype &
* @param offset the value
*
* @return size_t the position to insert
*
*/
template <typename KeyType>
size_t TreeNode<KeyType>::add_leaf(const KeyType &mykey, const offset val) // add a key-value in the leaf node and return the position
{
	int cou;
	if (!isLeaf)
	{
		cout << "Error:add_leaf(const KeyType &mykey,const offset val) is a function for leaf nodes" << endl;
		return -1;
	}
	if (this->count == 0)
	{
		key[0] = mykey;
		value[0] = val;
		count = 1;
		return 0;

	}
	else
	{
		size_t index = 0;
		bool exist = search(mykey, index);
		if (exist)
		{
			cout << "Error:In add_leaf(const Keytype &key, offset val),key has already in the tree!" << endl;
			exit(1);
		}
		else // add the key into the node
		{
			for (cou = count; cou > index; cou++)
			{
				this->key[cou] = this->key[cou - 1];
				this->value[cou] = this->value[cou - 1];

			}
			this->key[index] = mykey;
			this->value[index] = val;
			count++;
			if (index == 0)
			{
				int set;
				TreeNode<KeyType>* tmp = this;
				KeyType oldkey = tmp->key[1];
				while (tmp->parent && tmp == tmp->parent->child[0])
					tmp = tmp->parent;

				if (tmp->parent)
				{
					if (tmp->parent->search(oldkey, set))
						tmp->parent->key[set] = mykey;
					else
						cout << "Error:in insert key to leaf: fail to update the ancestor node!" << endl;
				}
			}
			return index;
		}
	}
}

template <typename KeyType>
bool TreeNode<KeyType>::remove(const size_t index)//如果是leaf就删去key[index],value[index]，如果是branch节点就删去key[index],child[index+1].
{
	int cou;
	if (index >= this->count)
	{
		cout << "Error:In remove(size_t index), index is more than count!" << endl;
		return false;
	}
	else if (this->isleaf)
	{
		KeyType oldkey = this->key[index];
		for (cou = index; cou < count-1; cou++)
		{
			this->key[cou] = this->key[cou + 1];
			this->value[cou] = this->value[cou + 1];
		}
		this->key[count - 1] = KeyType();
		this->value[count - 1] = offset();
		count--;

		/*if (index == 0)
		{
			offset set;
			TreeNode<KeyType>* tmp = this;
			while (tmp->parent && tmp == tmp->parent->child[0])
				tmp = tmp->parent;

			if (tmp->parent)
			{
				if (tmp->parent->search(oldkey, set))
					tmp->parent->key[set] = this->key[index];
				else
					cout << "Error:in insert key to leaf: fail to update the ancestor node!" << endl;
			}
		}*/
	}
	else
	{
		for (cou = index; cou < count - 1; cou++)
		{
			this->key[cou] = this->key[cou + 1];
			this->child[cou+1] = this->child[cou + 2];
		}
		this->key[count - 1] = KeyType();
		this->child[count] = NULL;
		count--;
	}
	return 1;
}

#ifdef _DEBUG
/**
* For debug, print the whole tree
*
* @param
*
* @return void
*
*/
template <class KeyType>
void TreeNode<KeyType>::debug_print()
{
	cout << "############DEBUG for node###############" << endl;

	cout << "Address: " << (void*)this << ",count: " << count << ",Parent: " << (void*)parent << ",isleaf: " << isleaf << ",nextNode: " << (void*)nextleaf << endl;
	cout << "KEYS:{";
	for (size_t i = 0; i < count; i++)
	{
		cout << key[i] << " ";
	}
	cout << "}" << endl;
	if (isleaf)
	{
		cout << "VALS:{";
		for (size_t i = 0; i < count; i++)
		{
			cout << value[i] << " ";
		}
		cout << "}" << endl;
	}
	else // nonleaf node
	{
		cout << "CHILDREN:{";
		for (size_t i = 0; i < count + 1; i++)
		{
			cout << (void*)child[i] << " ";
		}

		cout << "}" << endl;
	}
	cout << "#############END OF DEBUG IN NODE########" << endl;
}
#endif

//******** The definition of the functions of the class BplusTree **********

/**
* Constructor: init the tree, allocate the memory of the root and then,if users have created the tree before, read from disk and rebuild it.
*
* @param string m_name
* @param int keysize
* @param int m_degree
*
*/
template <class KeyType>
BplusTree<KeyType>::BplusTree(string filename, int degree):filename(filename), keycount(0), level(0), nodecount(0), root(NULL), leafhead(NULL),file(NULL), degree(degree)
{
	this->keysize = sizeof(KeyType);
	init_tree();
	ReadFromDiskAll();
}

template <class KeyType>
BplusTree<KeyType>::~BplusTree()
{
	DropTree(this->root);
	keycount = 0;
	root = NULL;
	leafhead = NULL;
	nodecount = 0;
	file = NULL;
	level = 0;
}

/**
* Init the tree,allocate memory for the root node.
*
* @return void
*
*/
template <class KeyType>
void BplusTree<KeyType>::init_tree()
{
	root = new TreeNode<KeyType>(degree, true);
	keycount = 0;
	level = 1;
	nodecount = 1;
	leafhead = root;
}

/**
* Drop the tree whose root is the inputed node.
*
* @param Node the pointer pointing to the node
*
* @return void
*
*/
template <class KeyType>
void BplusTree<KeyType>::DropTree(Node node)
{
	if (!node) return;
	if (!node->isleaf) //if it has child
	{
		for (size_t i = 0; i <= node->count; i++)
		{
			DropTree(node->child[i]);
			node->child[i] = NULL;
		}
	}
	delete node;
	nodecount--;
	return;
}

/**
* Read the whole existing tree from the disk.
*
* @return void
*
*/
template <class KeyType>
void BplusTree<KeyType>::ReadFromDiskAll()
{
	file = BM.getFile(filename.c_str());
	blockNode* btmp = BM.getBlockHead(file);
	while (true)
	{
		if (btmp == NULL)
		{
			return;
		}

		ReadFromDisk(btmp);
		if (btmp->ifbottom) break;
		btmp = BM.getNextBlock(file, btmp);
	}

}

/**
* Read a node from the disk.
*
* @param blockNode*
*
* @return void
*
*/
template <class KeyType>
void BplusTree<KeyType>::ReadFromDisk(blockNode* btmp)
{
	int valueSize = sizeof(offset);
	char* indexBegin = BM.get_content(*btmp);
	char* valueBegin = indexBegin + keysize;
	KeyType key;
	offset value;

	while (valueBegin - BM.get_content(*btmp) < BM.get_usingSize(*btmp))
		// there are available position in the block
	{
		key = *(KeyType*)indexBegin;
		value = *(offset*)valueBegin;
		insertkey(key, value);
		valueBegin += keysize + valueSize;
		indexBegin += keysize + valueSize;
	}

}

/**
* Write the whole tree data to the disk.
*
* @return void
*
*/
template <class KeyType>
void BplusTree<KeyType>::WriteBackToDiskAll()
{
	blockNode* btmp = BM.getBlockHead(file);
	Node ntmp = leafhead;
	int valueSize = sizeof(offset);
	while (ntmp != NULL)
	{
		BM.set_usingSize(*btmp, 0);
		BM.set_dirty(*btmp);
		for (int i = 0; i < ntmp->count; i++)
		{
			char* key = (char*)&(ntmp->key[i]);
			char* value = (char*)&(ntmp->value[i]);
			memcpy(BM.get_content(*btmp) + BM.get_usingSize(*btmp), key, keysize);
			BM.set_usingSize(*btmp, BM.get_usingSize(*btmp) + keysize);
			memcpy(BM.get_content(*btmp) + BM.get_usingSize(*btmp), value, valueSize);
			BM.set_usingSize(*btmp, BM.get_usingSize(*btmp) + valueSize);
		}

		btmp = BM.getNextBlock(file, btmp);
		ntmp = ntmp->nextleaf;
	}
	while (1)// delete the empty node
	{
		if (btmp->ifbottom)
			break;
		BM.set_usingSize(*btmp, 0);
		BM.set_dirty(*btmp);
		btmp = BM.getNextBlock(file, btmp);
	}

}

#ifdef _DEBUG

template <class KeyType>
void BplusTree<KeyType>::debug_print()
{
	cout << "############DEBUG FOR THE TREE############" << endl;
	cout << "name:" << filename << " root:" << (void*)root << " leafHead:" << (void *)leafhead << " keycount:" << keycount << " level:" << level << " nodeCount:" << nodecount << endl;

	if (root)
		debug_print_node(root);
	cout << "#############END OF DEBUG FOR TREE########" << endl;

}

template <class KeyType>
void BplusTree<KeyType>::debug_print_node(Node pnode)
{
	pnode->debug_print();
	if (!pnode->isleaf)
	for (int i = 0; i < pnode->count + 1; i++)
		debug_print_node(pnode->child[i]);

}

#endif

template <class KeyType>
void BplusTree<KeyType>::FindToLeaf(Node pnode, const KeyType& mykey, struct SearchNodeParse<KeyType>& snp)
{
	int index;
	if (pnode->search(mykey, index))
	{
		if (pnode->isleaf)
		{
			snp.PN = pnode;
			snp.index = index;
			snp.found = 1;
		}
		else
		{
			pnode = pnode->child[index + 1];
			while (!pnode->isleaf)
			{
				pnode = pnode->child[0];
			}
			snp.PN = pnode;
			snp.index = 0;
			snp.found = 1;
		}
	}
	else
	{
		if (pnode->isleaf)
		{
			snp.PN = pnode;
			snp.index = index;
			snp.found = 0;
		}
		else
		{
			BplusTree<KeyType>::FindToLeaf(pnode->child[index],mykey,snp);
		}
	}
}

template <class KeyType>
offset BplusTree<KeyType>::searchval(const KeyType& mykey) // search the value of specific key
{
	if (!this->root)
		return -1;

	struct SearchNodeParse<KeyType> snp;
	FindToLeaf(this->root, mykey, snp);
	if (!snp.found)
		return -1;
	else
		return snp.PN->value[snp.index];
}

template <class KeyType>
bool BplusTree<KeyType>::insertkey(const KeyType& mykey, offset myval)
{
	struct SearchNodeParse<KeyType> snp;
	if (!root)
		this->init_tree();

	FindToLeaf(this->root, mykey, snp);
	if (snp.found)
	{
		cout << "Error:in insert key to index: the duplicated key!" << endl;
		return 0;
	}
	else
	{
		snp.PN->add_leaf(mykey, myval);
		if (snp.pnode->count == degree)
		{
			adjustafterinsert(snp.pnode);
		}
		keycount++;
		return true;
	}

}

template <class KeyType>
bool BplusTree<KeyType>::adjustafterinsert(Node pnode)
{
	KeyType mykey;
	Node newnode = pnode->splite(mykey);
	nodecount++;

	if (pnode->isRoot())
	{
		Node root = new TreeNode<KeyType>(degree, false);
		if (root == NULL)
		{
			cout << "Error: can not allocate memory for the new root in adjustafterinsert" << endl;
			exit(1);
		}
		else
		{
			level++;
			nodecount++;
			this->root = root;
			pnode->parent = root;
			newnode->parent = root;
			root->add_branch(mykey);
			root->child[0] = pnode;
			root->child[1] = newnode;
			leafhead = pnode;
			return true;
		}
	}
	else// if it is not the root
	{
		Node parent = pnode->parent;
		size_t index = parent->add_branch(mykey);

		parent->child[index + 1] = newnode;
		newnode->parent = parent;
		if (parent->count == degree)
			return adjustafterinsert(parent);

		return true;
	}
}

template <class KeyType>
bool BplusTree<KeyType>::deletekey(const KeyType& mykey)
{
	struct SearchNodeParse<KeyType> snp;
	if (!root)
	{
		cout << "ERROR: In deleteKey, no nodes in the tree " << fileName << "!" << endl;
		return false;
	}
	else
	{
		FindToLeaf(this->root, mykey, snp);
		if (!snp.found)
		{
			cout << "ERROR: In deleteKey, no key in the tree " << filename << "!" << endl;
			return 0;
		}
		else
		{
			if (snp.PN->isRoot())
			{
				snp.PN->remove(snp.index);
				keycount--;
				return adjustafterdelete(snp.PN);
			}
			if (snp.index == 0 && leafhead != snp.PN) // the key exist in the branch.
			{			
				int set;
				TreeNode<KeyType>* tmp = snp.PN;
				while (tmp->parent && tmp == tmp->parent->child[0])
					tmp = tmp->parent;

				if (tmp->parent)
				{
					if (tmp->parent->search(mykey, set))
						tmp->parent->key[set] = snp.PN->key[1];
					else
						cout << "Error:in delete key to leaf: fail to update the ancestor node!" << endl;
				}
				snp.PN->remove(snp.index);
				keycount--;
				return adjustafterdelete(snp.PN);

			}
			else //this key must just exist in the leaf too.
			{
				snp.PN->remove(snp.index);
				keycount--;
				return adjustafterdelete(snp.PN);
			}
		}
	}
}


/**
* Adjust the node after deletion. Rrecursively call this function itself if the father node contradicts the rules.
*
* @param Node the pointer pointing to the node
*
* @return bool the flag
*
*/
template <class KeyType>
bool BplusTree<KeyType>::adjustafterdelete(Node pnode)
{
	size_t min_leaf=degree/2;
	size_t min_branch=(degree-1)/2;
	int cou;
	if (((pnode->isleaf) && (pnode->count >= min_leaf)) || ((!pnode->isleaf) && (pnode->count >=min_branch))) // do not need to adjust
	{
		return  true;
	}

	if (pnode->isRoot())
	{
		if (pnode->count > 0)
		{
			return 1;
		}
		else
		{
			if (pnode->isleaf)
			{
				delete pnode;
				this->root = NULL;
				leafhead = NULL;
				level--;
				nodecount--;
				
			}
			else
			{
				root = pnode->child[0];
				root->parent = NULL;
				delete pnode;
				level--;
				nodecount--;
			}
		}
			
	}
	else
	{
		Node par = pnode->parent;
		Node brother = NULL;
		if (pnode->isleaf)
		{
			size_t index = 0;
			par->search(pnode->key[0], index);

			if (index != 0 && index == par->count - 1)//choose the left brother to merge or replace
			{
				brother = par->child[index];
				if (brother->count > min_leaf)// choose the most right key of brother to add to the left hand of the pnode
				{
					for (cou = pnode->count; cou > 0; cou--)
					{
						pnode->key[cou] = pnode->key[cou - 1];
						pnode->value[cou] = pnode->value[cou - 1];
					}
					pnode->key[0] = brother->key[brother->count - 1];
					pnode->value[0] = brother->value[brother->count - 1];
					brother->remove(brother->count - 1);

					pnode->count++;
					par->key[index] = pnode->key[0];
					return true;
				}
				else// merge the node with its left brother
				{
					par->remove(index);

					for (cou = 0; cou < pnode->count; cou++)
					{
						brother->key[cou + brother->count] = pnode->key[cou];
						brother->value[cou + brother->count] = pnode->value[cou];
					}
					brother->count += pnode->count;
					brother->nextleaf = pnode->nextleaf;

					delete pnode;
					nodecount--;

					return adjustafterdelete(par);
				}

			}
			else// choose the right brother to merge or replace
			{
				if (pnode == par->child[0])
					brother = par->child[1];
				else
					brother = par->child[index + 2];
				if (brother->count > min_leaf)//// choose the most left key of right brother to add to the right hand of the node
				{
					pnode->key[pnode->count] = brother->key[0];
					pnode->value[pnode->count] = brother->value[0];
					pnode->count++;
					brother->remove(0);
					if (pnode == par->child[0])
						par->key[0] = brother->key[0];
					else
						par->key[index + 1] = brother->key[0];
					return true;

				}// end add
				else // merge the node with its right brother
				{
					for (cou=0; cou < brother->count; cou++)
					{
						pnode->key[pnode->count + cou] = brother->key[cou];
						pnode->value[pnode->count + cou] = brother->value[cou];
					}
					if (pnode == par->child[0])
						par->remove(0);
					else
						par->remove(index + 1);
					pnode->count += brother->count;
					pnode->nextleaf = brother->nextleaf;
					delete brother;
					nodecount--;

					return adjustafterdelete(par);
				}
			}
		}
		else// branch node
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

			if (index == par->count) // choose the left brother to merge or replace
			{
				brother = par->child[index-1];
				if (brother->count > min_branch) // choose the most right key and child to add to the left hand of the pnode
				{
					for (size_t i = pnode->count; i > 0; i--)
					{
						pnode->child[i+1] = pnode->child[i];
						pnode->key[i] = pnode->key[i - 1];
					}
					pnode->child[1] = pnode->child[0];
					pnode->child[0] = brother->child[brother->count];
					pnode->key[0] = par->key[index];
					pnode->count++;

					par->key[index] = brother->key[brother->count - 1];

					brother->remove(brother->count - 1);
					pnode->child[0]->parent = pnode;
					return true;

				}// end add
				else // merge the node with its brother
				{
					//modify the brother and child
					brother->key[brother->count] = par->key[index]; 
						par->remove(index);
					brother->count++;

					for (int i = 0; i < pnode->count; i++)
					{
						brother->child[brother->count + i] = pnode->child[i];
						brother->key[brother->count + i] = pnode->key[i];
						brother->child[brother->count + i]->parent = brother;
					}
					brother->child[brother->count + pnode->count] = pnode->child[pnode->count];
					brother->child[brother->count + pnode->count]->parent = brother;

					brother->count += pnode->count;

					delete pnode;
					nodecount--;

					return adjustafterdelete(par);
				}

			}// end of the left brother
			else // choose the right brother
			{
				if (par->child[0] == pnode)
					brother = par->child[1];
				else
					brother = par->child[index + 1];
				if (brother->count > min_branch)// choose the most left key and child to add to the right hand of the pnode
				{
					//modifty the pnode and child
					pnode->child[pnode->count + 1] = brother->child[0];
					pnode->key[pnode->count] = par->key[index];//////////////////////////////这里要把parent中的那个key插入进来
					pnode->child[pnode->count + 1]->parent = pnode;
					pnode->count++;
					//modify the father
					par->key[index] = brother->key[0];
					//modify the brother
					brother->remove(0);

					return true;
				}
				else // merge the node with its brother
				{
					//modify the pnode and child
					pnode->key[pnode->count] = par->key[index];

					par->remove(index);

					pnode->count++;

					for (int i = 0; i < brother->count; i++)
					{
						pnode->child[pnode->count + i] = brother->child[i];
						pnode->key[pnode->count + i] = brother->key[i];
						pnode->child[pnode->count + i]->parent = pnode;
					}
					pnode->child[pnode->count + brother->count] = brother->child[brother->count];
					pnode->child[pnode->count + brother->count]->parent = pnode;

					pnode->count += brother->count;


					delete brother;
					nodecount--;

					return adjustafterdelete(par);

				}

			}

		}
		
	}

}


