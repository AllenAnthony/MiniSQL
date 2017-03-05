#ifndef __BplusTree
#define __BplusTree
#include <vector>
#include <stdio.h>
#include <string.h>
#include "BufferManager.h"
#include "Minisql.h"
#include <string>
using namespace std;

//�������ǹ涨degree>=4,���ڵ�degree<=3ʱ���������Ч�ʲ���,����Ĭ��degree>=4

template <typename KeyType> class TreeNode;
template <typename KeyType> class BplusTree;

static BufferManager BM;
typedef int offset; // the value of the tree node
typedef unsigned int size_t;



//**********************TreeNode***************************//
template <typename KeyType>
class TreeNode
{
private:
	unsigned int degree;

public:
	vector<KeyType> key;
	vector<TreeNode*> child;
	vector<offset> value;

	size_t count;  // the count of keys
	TreeNode<KeyType>* parent;
	TreeNode<KeyType>* nextleaf;

	bool isleaf;

public:
	TreeNode(int degree, bool newLeaf = false);
	~TreeNode();

	bool isRoot();
	bool search(const KeyType& mykey, size_t &index);//������ڵ�������key���������򷵻������key�ı�ţ����������򷵻����key���ܴ��ڵ��ӽڵ��ָ���ţ�����ͨ��indexʵ��
	TreeNode<KeyType>* splite(KeyType &mykey);//����key��������ڵ㣬key��������parent�ڵ㣬����ڵ��Ϊ�����ڵ㣬�����Ǹ��½ڵ�
	size_t add_branch(const KeyType &mykey); //add the key in the branch and return the position
	size_t add_leaf(const KeyType &mykey, const offset val); // add a key-value in the leaf node and return the position
	bool remove(const size_t index);
	



#ifdef _DEBUG
public:
	void debug_print();
#endif
};



//**********************BplusTree***************************//
template <typename KeyType>
class BplusTree
{
private:
	typedef TreeNode<KeyType>* Node;

	template <typename KeyType>
	struct SearchNodeParse
	{
		TreeNode<KeyType>* PN; // a pointer pointering to the node containing the key
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
	void FindToLeaf(Node pnode, const KeyType& mykey, struct SearchNodeParse<KeyType>& snp);

public:
	BplusTree(string filename, int degree);
	~BplusTree();

	offset searchval(const KeyType& mykey); // search the value of specific key
	bool insertkey(const KeyType &mykey, offset val);
	bool deletekey(const KeyType &mykey);

	void DropTree(Node node);
	void ReadFromDiskAll();
	void WriteBackToDiskAll();
	void ReadFromDisk(blockNode* BN);


#ifdef _DEBUG
public:
	void debug_print();

	void debug_print_node(Node pNode);
#endif

};

#endif