#include <iostream>
#include <bitset>
#include <stdio.h>
#include <stdlib.h>
#include <string>
using namespace std;

#define L1_CACHE_SETS 16
#define L2_CACHE_SETS 16
#define L2_CACHE_WAYS 8
#define VICTIM_CACHE_WAYS 4
#define MEM_SIZE 4096
#define BLOCK_SIZE 4 // bytes per block
#define DM 0
#define SA 1

struct cacheBlock
{
	int tag;		  // you need to compute offset and index to find the tag.
	int lru_position; // for SA only
	int data;		  // the actual data stored in the cache/memory
	bool valid;
	int index;
	// add more things here if needed
};

struct Stat
{
	int missL1;
	int missL2;
	int hitL1;
	int hitL2;
	int accL1;
	int accL2;
	int VmissL1;
	int VhitL1;
	// add more stat if needed. Don't forget to initialize!
};

class cache
{
private:
	cacheBlock L1[L1_CACHE_SETS];				 // 1 set per row.
	cacheBlock L2[L2_CACHE_SETS][L2_CACHE_WAYS]; // x ways per row
	cacheBlock VC[VICTIM_CACHE_WAYS];			 // 4 ways
	Stat myStat;
	void load_word(int &currData, int currAdr, int *myMem);
	void store_word(int &currData, int currAdr, int *myMem);
	// add more things here
public:
	cache();
	void controller(bool MemR, bool MemW, int *data, int adr, int *myMem);
	float getStats(int statReq);
	void printLRU(int currIndex, bool flag);
	void updateLRU(int currIndex, int evictedLRUVal);
	int findNewL2Spot(int currIndex);
	void hitInL2(int currIndex, int ways, int curAddr);
	bool foundInVC(int currIndex, int currTag, struct cacheBlock &VCinfo, int &vcWay);
	void updateLRU_VC(int evictedLRUVal);
	void missInL2(int currIndex, int currTag, int ways);
	void printLRU_VC(bool flag);
	void cacheDump();

	// add more functions here ...
};
