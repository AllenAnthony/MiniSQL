#ifndef _MACRO_H
#define _MACRO_H


#define INTLENGTH 11
#define FLOATLENGTH 10
#define INTLEN		11
#define FLOATLEN	10

#define ISPRIMARYKEY 1
#define NOTPRIMARYKEY 0
#define ISUNIQUE 1
#define NOTUNIQUE 0


#define MAXBLOCKNUMBER 1024	//should be 10000
#define BLOCKSIZE 4096	//should be 4096,4k
#define EMPTY '@'
#define END '@'
#define NOTEMPTY '1'
#define DELETED '@'


#define MAXPRIMARYKEYLENGTH  25    //should change sometime

#define UNKNOW		12
#define SELERR		13
#define	CREINDERR	14
#define CRETABERR	15
#define DELETEERR	16
#define INSERTERR	17
#define DRPTABERR	18
#define DRPINDERR	19 
#define EXEFILERR	20
#define NOPRIKEY    27
#define VOIDPRI		21
#define VOIDUNI		22
#define CHARBOUD    23
#define TABLEERROR  24
#define COLUMNERROR 25
#define INSERTNUMBERERROR 26
#define SELECT_NOWHERE_CAULSE  29
#define SELECT_WHERE_CAULSE    28
#define TABLEEXISTED  30
#define INDEXERROR    31
#define INDEXEROR     32

#define SELECT		0
#define CRETAB		1
#define	CREIND		2
#define	DRPTAB		3
#define DRPIND		4
#define DELETE		5
#define INSERT		6
#define QUIT		7
#define EXEFILE		8 

#define COMLEN		400 
#define INPUTLEN	200 
#define WORDLEN		100
#define VALLEN		300
#define NAMELEN		100
#endif