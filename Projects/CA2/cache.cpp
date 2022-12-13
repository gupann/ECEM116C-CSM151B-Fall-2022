#include "cache.h"
cache::cache()
{
	for (int i = 0; i < L1_CACHE_SETS; i++)
		L1[i].valid = false;
	for (int i = 0; i < L2_CACHE_SETS; i++)
		for (int j = 0; j < L2_CACHE_WAYS; j++)
			L2[i][j].valid = false;

	this->myStat.missL1 = 0;
	this->myStat.missL2 = 0;
	this->myStat.hitL1 = 0;
	this->myStat.hitL2 = 0;
	this->myStat.accL1 = 0;
	this->myStat.accL2 = 0;
}
void cache::controller(bool MemR, bool MemW, int *data, int adr, int *myMem)
{

	// add your code here
	if (MemR)
		load_word(*data, adr, myMem);
	if (MemW)
		store_word(*data, adr, myMem);
}
float cache::getStats(int statReq)
{
	// cout << "myStat.missL1: " << myStat.missL1 << " myStat.hitL1: " << myStat.hitL1 << " myStat.missL2: " << myStat.missL2 << "  myStat.hitL2: " << myStat.hitL2 << endl;
	float temp;
	switch (statReq)
	{
	case 0:
		temp = ((float)myStat.missL1 / ((float)myStat.missL1 + (float)myStat.hitL1));
		break;
	case 1:
		temp = ((float)myStat.missL2 / ((float)myStat.missL2 + (float)myStat.hitL2));
		break;
	default:
		cout << "unsupported" << endl;
	}

	// cout << temp << endl;
	return temp;
}

void cache::printLRU(int currIndex, bool flag)
{
	if (flag)
		cout << endl
			 << "ROW " << currIndex << " BEFORE # LRU: way";
	else
		cout << endl
			 << "ROW " << currIndex << " AFTER # LRU: way";

	for (int tmpways = 0; tmpways < L2_CACHE_WAYS; tmpways++)
	{
		cout << "(V " << L2[currIndex][tmpways].valid << " W " << tmpways << " L " << L2[currIndex][tmpways].lru_position << ") | ";
	}
	cout << endl;
}

void cache::updateLRU(int currIndex, int evictedLRUVal)
{
	// printLRU(currIndex, true);
	for (int w = 0; w < L2_CACHE_WAYS; w++)
	{
		if ((L2[currIndex][w].valid) && (L2[currIndex][w].lru_position > evictedLRUVal))
			L2[currIndex][w].lru_position--; // decrement lru by one
	}
}

int cache::findNewL2Spot(int currIndex)
{
	bool status = false;
	int w;
	int minLRU_pos = 0;
	// if empty spot available in L2, put there, else evict oldest and put there
	for (w = 0; w < L2_CACHE_WAYS; w++)
	{
		if (!(L2[currIndex][w].valid)) // empty spot found
		{
			updateLRU(currIndex, 0);
			L2[currIndex][w].lru_position = L2_CACHE_WAYS - 1;
			L2[currIndex][w].valid = 1;
			// printLRU(currIndex, false);
			status = true;
			minLRU_pos = w;
			break;
		}
	}
	if (!status) // empty spot not found, evict oldest and place there
	{
		int minLRU_val = L2_CACHE_WAYS - 1;
		for (w = 0; w < L2_CACHE_WAYS; w++)
		{
			if (L2[currIndex][w].lru_position < minLRU_val)
			{
				minLRU_val = L2[currIndex][w].lru_position;
				minLRU_pos = w;
			}
		}
		updateLRU(currIndex, minLRU_val);
		L2[currIndex][minLRU_pos].lru_position = L2_CACHE_WAYS - 1;
		// printLRU(currIndex, false);
		status = true;
	}
	// cout << "Evict (Row " << currIndex << " Way " << minLRU_pos << " )";
	return minLRU_pos;
}

void cache::hitInL2(int currIndex, int &currData, int ways)
{
	int evictedLRUVal = 0;
	// temporarily store L1 info
	struct cacheBlock tmpStore;
	tmpStore.valid = L1[currIndex].valid;
	tmpStore.tag = L1[currIndex].tag;
	tmpStore.data = L1[currIndex].data;

	// copy L2 to L1
	L1[currIndex].valid = true;
	L1[currIndex].tag = L2[currIndex][ways].tag;
	L1[currIndex].data = L2[currIndex][ways].data;
	currData = L2[currIndex][ways].data;
	L2[currIndex][ways].valid = false;

	// get LRU position of evicted L2 value and update LRUs
	evictedLRUVal = L2[currIndex][ways].lru_position;
	updateLRU(currIndex, evictedLRUVal);

	if (tmpStore.valid)
	{
		// copy l1 (temp stored) to l2
		L2[currIndex][ways].valid = tmpStore.valid;
		L2[currIndex][ways].tag = tmpStore.tag;
		L2[currIndex][ways].data = tmpStore.data;
		L2[currIndex][ways].lru_position = L2_CACHE_WAYS - 1;
	}
	// printLRU(currIndex, false);
}
// Handles the load word process for each cache type.
void cache::load_word(int &currData, int currAdr, int *myMem)
{

	int ways, status = 0;
	int currTag = currAdr >> 4;
	int currIndex = currAdr & 0xF;
	// cout << "============================START Load========================================" << endl;
	// cout << "currAdr: " << currAdr << " currData: " << currData << "  currIndex: " << currIndex << " currTag: " << currTag << endl;

	if ((L1[currIndex].valid) && (L1[currIndex].tag == currTag))
	{
		// cout << "HIT in L1" << endl;

		// Cache hit in L1
		currData = L1[currIndex].data;
		myStat.hitL1++;
		// cout << "function: " << __func__ << "  line: " << __LINE__ << endl;
	}
	else
	{
		// cout << "MISS in L1" << endl;

		// Cache miss in L1, check L2
		myStat.missL1++;
		status = 0;
		for (ways = 0; ways < L2_CACHE_WAYS; ways++)
		{
			if ((L2[currIndex][ways].valid) && (L2[currIndex][ways].tag == currTag))
			{
				// Cache hit in L2
				// cout << "HIT in L2 Row " << currIndex << " ways " << ways << "L " << L2[currIndex][ways].lru_position << endl;
				myStat.hitL2++;

				hitInL2(currIndex, currData, ways);
				status = 1;
				break;
			}
		}
		// cout << "function: " << __func__ << "  line: " << __LINE__ << endl;

		if (!status) // status = 0
		{
			// cout << "MISS in L2" << endl;
			//  cout << "HIT in MEMORY" << endl;

			// Cache miss in L2, check main memory
			myStat.missL2++;

			// if l1 index valid is valid, put in l2, if l2 is valid, evict it , then put main mem val in l1
			if (L1[currIndex].valid)
			{
				int replacedWay = findNewL2Spot(currIndex);
				// cout << "Copy Valid L1 tag into L2 (Row " << currIndex << ", Way " << replacedWay << " )" << endl;
				L2[currIndex][replacedWay].tag = L1[currIndex].tag;
				L2[currIndex][replacedWay].data = L1[currIndex].data;
				L2[currIndex][replacedWay].valid = true;
				// lru posn already updated
			}

			// load value from main memory to l1
			currData = L1[currIndex].data = myMem[currAdr];
			L1[currIndex].tag = currTag;
			L1[currIndex].valid = true;
			// cout << "Copy from Memory" << endl;
		}
	}
	// cout << "=========================END Load===========================================" << endl;
}

// Handles the store word process for each cache type.
void cache::store_word(int &currData, int currAdr, int *myMem)
{
	int ways, evictedLRUVal = 0, status = 0;
	int currTag = currAdr >> 4;
	int currIndex = currAdr & 0xF;
	// cout << "============================START Store========================================" << endl;
	// cout << "currAdr: " << currAdr << " currData: " << currData << "  currIndex: " << currIndex << " currTag: " << currTag << endl;

	if ((L1[currIndex].valid) && (L1[currIndex].tag == currTag))
	{
		// cout << "HIT in L1" << endl;
		//  Cache hit in L1
		L1[currIndex].data = currData;
		myMem[currAdr] = currData;
		myStat.hitL1++;
	}
	else
	{
		// cout << "MISS in L1" << endl;

		// Cache miss in L1, check L2
		myStat.missL1++;
		status = 0;
		for (ways = 0; ways < L2_CACHE_WAYS; ways++)
		{
			if ((L2[currIndex][ways].valid) && (L2[currIndex][ways].tag == currTag))
			{
				// Cache hit in L2
				// cout << "HIT in L2 Row " << currIndex << " ways " << ways << "L " << L2[currIndex][ways].lru_position << endl;
				myStat.hitL2++;
				// Update Memory with new data
				myMem[currAdr] = currData;
				hitInL2(currIndex, currData, ways);
				status = 1;
				break;
			}
		}

		if (!status) // status = 0 //
		{
			// cout << "MISS in L2" << endl;

			myStat.missL2++;
			myMem[currAdr] = currData;
			// cout << "HIT IN MEMORY" << endl;
			// cout << "function: " << __func__ << "  line: " << __LINE__ << "  currAdr: " << currAdr << " currData: " << currData << "  currIndex: " << currIndex << " currTag: " << currTag << " myMem[currAdr]: " << myMem[currAdr] << endl;
		}
	}
	// cout << "============================END Store========================================" << endl;
}
