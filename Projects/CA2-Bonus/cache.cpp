#include "cache.h"

cache::cache()
{
	for (int i = 0; i < L1_CACHE_SETS; i++)
		L1[i].valid = false;
	for (int i = 0; i < L2_CACHE_SETS; i++)
		for (int j = 0; j < L2_CACHE_WAYS; j++)
			L2[i][j].valid = false;
	for (int i = 0; i < VICTIM_CACHE_WAYS; i++)
		VC[i].valid = false;

	this->myStat.missL1 = 0;
	this->myStat.missL2 = 0;
	this->myStat.hitL1 = 0;
	this->myStat.hitL2 = 0;
	this->myStat.accL1 = 0;
	this->myStat.accL2 = 0;
	this->myStat.VmissL1 = 0;
	this->myStat.VhitL1 = 0;
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
	// cerr << "myStat.missL1: " << myStat.missL1 << " myStat.hitL1: " << myStat.hitL1 << " myStat.missL2: " << myStat.missL2 << "  myStat.hitL2: " << myStat.hitL2 << endl;
	// cerr << "myStat.VmissL1: " << myStat.VmissL1 << "myStat.VhitL1: " << myStat.VhitL1 << endl;
	float temp;
	switch (statReq)
	{
	case 0:
		//	temp = ((float)myStat.missL1 / ((float)myStat.missL1 + (float)myStat.hitL1));
		temp = (((float)myStat.missL1 - (float)myStat.VhitL1) / ((float)myStat.missL1 + (float)myStat.hitL1));
		break;
	case 1:
		temp = ((float)myStat.missL2 / ((float)myStat.missL2 + (float)myStat.hitL2));
		break;
	default:
		cerr << "unsupported" << endl;
	}

	// cerr << temp << endl;
	return temp;
}

void cache::printLRU(int currIndex, bool flag)
{
	if (flag)
		cerr << endl
			 << "ROW " << currIndex << " BEFORE # LRU: way";
	else
		cerr << endl
			 << "ROW " << currIndex << " AFTER # LRU: way";

	for (int tmpways = 0; tmpways < L2_CACHE_WAYS; tmpways++)
	{
		cerr << "(V " << L2[currIndex][tmpways].valid << " W " << tmpways << " L " << L2[currIndex][tmpways].lru_position << ") | ";
	}
	cerr << endl;
}

void cache::updateLRU(int currIndex, int evictedLRUVal) // for L2
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
				minLRU_pos = w; // finds where lru = 0
			}
		}
		updateLRU(currIndex, minLRU_val); // same as updateLRU(currIndex, 0);
		L2[currIndex][minLRU_pos].lru_position = L2_CACHE_WAYS - 1;
		// printLRU(currIndex, false);
		status = true;
	}
	// cerr << "Evict (Row " << currIndex << " Way " << minLRU_pos << " )";
	return minLRU_pos;
}

void cache::hitInL2(int currIndex, int ways, int curAddr)
{
	int evictedLRUVal = 0;

	// temporarily store L1 info
	struct cacheBlock tmpStore;
	tmpStore.valid = L1[currIndex].valid;
	tmpStore.tag = L1[currIndex].tag;
	tmpStore.data = L1[currIndex].data;
	tmpStore.index = currIndex;
	cerr << "Copy L2-->L1 ";
	// copy L2 to L1
	L1[currIndex].valid = true;
	L1[currIndex].tag = L2[currIndex][ways].tag;
	L1[currIndex].data = L2[currIndex][ways].data;
	L1[currIndex].index = L2[currIndex][ways].index;
	evictedLRUVal = L2[currIndex][ways].lru_position;
	L2[currIndex][ways].valid = false;

	if (tmpStore.valid) // move L1 stuff to VC
	{
		bool emptyspot = false;
		for (int i = 0; i < VICTIM_CACHE_WAYS; i++)
		{
			if (!(VC[i].valid)) // empty spot found in VC, put L1 stuff in VC
			{
				cerr << "| Found Empty spot in VCache loc " << i;
				// copy L1 stuff to VC
				VC[i].tag = tmpStore.tag;
				VC[i].data = tmpStore.data;
				VC[i].index = tmpStore.index;
				updateLRU_VC(0);
				VC[i].valid = true;
				VC[i].lru_position = VICTIM_CACHE_WAYS - 1;
				emptyspot = true;
				break;
			}
		}
		if (!emptyspot) // empty spot not found in VC, evict oldest, put L1 stuff in VC and VC stuff in L2
		{
			int oldest = VICTIM_CACHE_WAYS - 1;
			int old_pos = 0;
			for (int i = 0; i < VICTIM_CACHE_WAYS; i++)
			{
				if ((VC[i].valid) && (VC[i].lru_position < oldest)) // oldest in VC
				{
					oldest = VC[i].lru_position;
					old_pos = i;
				}
			}
			cerr << " | Found OLD Victim Cache Position " << old_pos << " LRU " << oldest << " | Copy VC-->L2 addr " << curAddr;
			// copy VC stuff to L2
			L2[currIndex][ways].tag = VC[old_pos].tag;
			L2[currIndex][ways].data = VC[old_pos].data;
			L2[currIndex][ways].index = VC[old_pos].index;
			updateLRU(currIndex, evictedLRUVal); // update l2 lru
			L2[currIndex][ways].lru_position = L2_CACHE_WAYS - 1;
			L2[currIndex][ways].valid = true;
			cerr << "Evict L2 with addr " << (L2[currIndex][ways].tag << 4 | L2[currIndex][ways].index) << endl;
			// copy L1 stuff (temp stored) to VC
			VC[old_pos].tag = tmpStore.tag;
			VC[old_pos].data = tmpStore.data;
			VC[old_pos].index = tmpStore.index;
			updateLRU_VC(0);
			VC[old_pos].valid = true;
			VC[old_pos].lru_position = VICTIM_CACHE_WAYS - 1;
			cerr << "put L1evicted to VC" << (tmpStore.tag << 4 | tmpStore.index) << endl;
		}
	}
}

void cache::missInL2(int currIndex, int currTag, int ways)
{
	int evictedLRUVal = 0;

	// temporarily store L1 info
	struct cacheBlock tmpStore;
	tmpStore.valid = L1[currIndex].valid;
	tmpStore.tag = L1[currIndex].tag;
	tmpStore.data = L1[currIndex].data;
	tmpStore.index = currIndex;

	if (tmpStore.valid) // move L1 stuff to VC
	{
		cerr << " | Valid L1 foundi | ";
		bool emptyspot = false;
		for (int i = 0; i < VICTIM_CACHE_WAYS; i++)
		{
			if (!(VC[i].valid)) // empty spot found in VC, put L1 stuff in VC
			{
				// copy L1 stuff to VC
				VC[i].tag = tmpStore.tag;
				VC[i].data = tmpStore.data;
				VC[i].index = tmpStore.index;
				updateLRU_VC(0);
				VC[i].valid = true;
				VC[i].lru_position = VICTIM_CACHE_WAYS - 1;
				emptyspot = true;
				cerr << "empty spot found in Victim Cache loc " << i << " Copy L1";
				break;
			}
		}
		if (!emptyspot) // empty spot not found in VC, evict oldest, put L1 stuff in VC and VC stuff in L2
		{
			int oldest = VICTIM_CACHE_WAYS - 1;
			int pos = 0;
			printLRU_VC(true);
			for (int i = 0; i < VICTIM_CACHE_WAYS; i++)
			{
				if ((VC[i].valid) && (VC[i].lru_position < oldest)) // oldest in VC
				{
					oldest = VC[i].lru_position;
					pos = i;
				}
			}
			cerr << "====VC index " << VC[pos].index << " addr " << (VC[pos].tag << 4 | VC[pos].index) << endl;
			// copy VC stuff to L2
			int replacedWay = findNewL2Spot(VC[pos].index);
			cerr << "evict oldest victim cache pos " << pos << " LRU_val " << oldest << " to L2 cache(row  " << VC[pos].index << ",way " << replacedWay << endl;
			L2[VC[pos].index][replacedWay].tag = VC[pos].tag;
			L2[VC[pos].index][replacedWay].data = VC[pos].data;
			L2[VC[pos].index][replacedWay].index = VC[pos].index;
			L2[VC[pos].index][replacedWay].valid = true;
			// lru posn already updated

			// copy L1 stuff (temp stored) to VC
			VC[pos].tag = tmpStore.tag;
			VC[pos].data = tmpStore.data;
			VC[pos].index = tmpStore.index;
			updateLRU_VC(0);
			VC[pos].valid = true;
			VC[pos].lru_position = VICTIM_CACHE_WAYS - 1;
		}
	}
}

bool cache::foundInVC(int currIndex, int currTag, struct cacheBlock &VCinfo, int &vcWay)
{

	for (int i = 0; i < VICTIM_CACHE_WAYS; i++)
	{
		if ((VC[i].valid) && (VC[i].tag == currTag) && (VC[i].index == currIndex))
		{
			// cerr << "HIT in VC" << endl;
			// Cache hit in VC
			vcWay = i;
			VCinfo.lru_position = VC[i].lru_position;
			VCinfo.data = VC[i].data;
			return true;
		}
	}
	return false;
}

void cache::printLRU_VC(bool flag)
{
	if (flag)
		cerr << "BEFORE VC LRU" << endl;
	else
		cerr << "AFTER VC LRU | ";

	for (int w = 0; w < VICTIM_CACHE_WAYS; w++)
	{
		cerr << "pos = " << w << " | valiad " << VC[w].valid << " LRU_Val " << VC[w].lru_position << endl;
	}
}

void cache::updateLRU_VC(int evictedLRUVal)
{
	for (int w = 0; w < VICTIM_CACHE_WAYS; w++)
	{
		if ((VC[w].valid) && (VC[w].lru_position > evictedLRUVal))
			VC[w].lru_position--; // decrement lru by one
	}
}

void cache::cacheDump()
{
#if 0
	int set;
	cerr << "L1 cache Dump.." << endl;
	for (set = 0; set < L1_CACHE_SETS; set++)
	{
		// cerr << "row " << set << " valid " << L1[set].valid << " addr " << (L1[set].tag << 4) <<  endl ;
		cerr << "(" << set << "," << L1[set].valid << "," << (L1[set].tag << 4 | set) << ")";
	}
	cerr << endl;
	cerr << "Victim cache Dump.." << endl;
	for (int i = 0; i < VICTIM_CACHE_WAYS; i++)
	{
		cerr << "(" << i << "," << VC[i].valid << "," << (VC[i].tag << 4 | VC[i].index) << ")";
	}
	cerr << endl;
	for (set = 0; set < L2_CACHE_SETS; set++)
	{
		cerr << "Row " << set << endl;
		for (int w = 0; w < L2_CACHE_WAYS; w++)
		{
			cerr << "(" << w << "," << L2[set][w].valid << "," << (L2[set][w].tag << 4 | set) << ")";
		}
		cerr << endl;
	}
#endif
}

// Handles the load word process for each cache type.
void cache::load_word(int &currData, int currAdr, int *myMem)
{
	int ways, status = 0;
	int currTag = currAdr >> 4;
	int currIndex = currAdr & 0xF;
	struct cacheBlock VCinfo; // refers to the VC block containing required info
	int vcWay, set;
	int evictedLRUVal = 0;
	cerr << "============================START Load========================================" << endl;
	cerr << "currAdr: " << currAdr << " currData: " << currData << "  currIndex: " << currIndex << " currTag: " << currTag << endl;

	// cacheDump();

	if ((L1[currIndex].valid) && (L1[currIndex].tag == currTag))
	{
		cerr << "=====HIT in L1 | " << endl;
		// Cache hit in L1
		currData = L1[currIndex].data;
		myStat.hitL1++;
		// cerr << "function: " << __func__ << "  line: " << __LINE__ << endl;
	}
	else
	{
		cerr << "MISS in L1 |" << endl;
		// Cache miss in L1, check victim cache
		myStat.missL1++;
		// status = 0;

		if (foundInVC(currIndex, currTag, VCinfo, vcWay))
		{
			// cache hit in VC
			myStat.VhitL1++;
			cerr << " | HIT in VC pos " << vcWay << " | Copying VC Add " << currAdr << "-->L1 | ";
			// temp store L1 stuff
			struct cacheBlock tmpStore;
			tmpStore.valid = L1[currIndex].valid;
			tmpStore.tag = L1[currIndex].tag;
			tmpStore.data = L1[currIndex].data;
			tmpStore.index = currIndex;

			// move VC stuff to L1
			L1[currIndex].valid = true;
			L1[currIndex].tag = VCinfo.tag;
			L1[currIndex].data = VCinfo.data;
			L1[currIndex].index = currIndex;
			currData = VCinfo.data;
			VC[vcWay].valid = false;

			// if L1 spot is valid
			// move temp store stuff to VC
			if (tmpStore.valid)
			{
				cerr << " | Valid L1,swap to VC " << endl;
				VC[vcWay].valid = true;
				VC[vcWay].tag = tmpStore.tag;
				VC[vcWay].data = tmpStore.data;
				VC[vcWay].index = tmpStore.index;
			}

			// updating LRU in VC
			evictedLRUVal = VCinfo.lru_position;
			updateLRU_VC(evictedLRUVal);
			VC[vcWay].lru_position = VICTIM_CACHE_WAYS - 1;
		}
		else
		{
			// Cache miss in VC, check L2
			cerr << " | MISS in VC " << endl;
			myStat.VmissL1++;
			for (set = 0; set < L2_CACHE_SETS; set++)
			{
				for (ways = 0; ways < L2_CACHE_WAYS; ways++)
				{
					if ((L2[set][ways].valid) && (L2[set][ways].tag == currTag))
					{
						// Cache hit in L2
						cerr << "HIT in L2 Row " << set << " ways " << ways << "L " << L2[set][ways].lru_position << endl;
						myStat.hitL2++;
						currData = L2[set][ways].data; // load data
						hitInL2(set, ways, currAdr);
						status = 1;
						break;
					}
				}
			}
			// cerr << "function: " << __func__ << "  line: " << __LINE__ << endl;

			if (!status) // status = 0
			{
				cerr << "MISS in L2 |  " << endl;
				//  cerr << "HIT in MEMORY" << endl;

				// Cache miss in L2, check main memory
				myStat.missL2++;
				currData = myMem[currAdr]; // load data

				// move from mem to L1, if l1 full --> VC, if VC full --> L2, if L2 full --> evict
				missInL2(currIndex, currTag, ways);
				// load value from main memory to l1
				currData = L1[currIndex].data = myMem[currAdr];
				L1[currIndex].tag = currTag;
				L1[currIndex].valid = true;
				cerr << "Copy from Memory to L1" << endl;
			}
			////
		}
	}
	cacheDump();
	cerr << "=========================END Load===========================================" << endl;
}

// Handles the store word process for each cache type.
void cache::store_word(int &currData, int currAdr, int *myMem)
{
	int ways, status = 0;
	int currTag = currAdr >> 4;
	int currIndex = currAdr & 0xF;
	struct cacheBlock VCinfo; // refers to the VC block containing required info
	int vcWay;
	int evictedLRUVal = 0;
	int set;
	cerr << "============================START Store========================================" << endl;
	cerr << "currAdr: " << currAdr << " currData: " << currData << "  currIndex: " << currIndex << " currTag: " << currTag << endl;

	// cacheDump();
	if ((L1[currIndex].valid) && (L1[currIndex].tag == currTag))
	{
		cerr << "HIT in L1" << endl;
		//  Cache hit in L1
		L1[currIndex].data = currData;
		myMem[currAdr] = currData;
		myStat.hitL1++;
	}
	else
	{
		cerr << "MISS in L1" << endl;
		// Cache miss in L1, check victim cache
		myStat.missL1++;
		// status = 0;

		if (foundInVC(currIndex, currTag, VCinfo, vcWay))
		{
			// cache hit in VC
			cerr << "HIT in VC" << endl;
			myStat.VhitL1++;
			myMem[currAdr] = currData; // store in mem

			// temp store L1 stuff
			struct cacheBlock tmpStore;
			tmpStore.valid = L1[currIndex].valid;
			tmpStore.tag = L1[currIndex].tag;
			tmpStore.data = L1[currIndex].data;
			tmpStore.index = currIndex;

			// move VC stuff to L1
			L1[currIndex].valid = true;
			L1[currIndex].tag = VCinfo.tag;
			L1[currIndex].data = currData; // store latest data in L1
			L1[currIndex].index = currIndex;
			VC[vcWay].valid = false;

			// if L1 spot is valid
			// move L1 temp store stuff to VC
			if (tmpStore.valid)
			{
				cerr << "evict L1 data to Victim cache loc " << vcWay;
				VC[vcWay].tag = tmpStore.tag;
				VC[vcWay].data = tmpStore.data;
				VC[vcWay].index = tmpStore.index;
				VC[vcWay].valid = true;
			}

			// updating LRU in VC
			evictedLRUVal = VCinfo.lru_position;
			updateLRU_VC(evictedLRUVal);
			VC[vcWay].lru_position = VICTIM_CACHE_WAYS - 1;
		}
		else
		{
			// Cache miss in VC, check L2
			cerr << "MISS in VC" << endl;
			myStat.VmissL1++;
			for (ways = 0; ways < L2_CACHE_WAYS; ways++)
			{
				if ((L2[currIndex][ways].valid) && (L2[currIndex][ways].tag == currTag))
				{
					// Cache hit in L2
					cerr << "HIT in L2 Row " << currIndex << " ways " << ways << "L " << L2[currIndex][ways].lru_position << endl;
					myStat.hitL2++;
					L2[currIndex][ways].data = currData; // store data
					myMem[currAdr] = currData;
					hitInL2(currIndex, ways, currAdr);
					status = 1;
					break;
				}
			}
			// cerr << "function: " << __func__ << "  line: " << __LINE__ << endl;

			if (!status) // status = 0 //
			{
				cerr << "MISS in L2" << endl;

				myStat.missL2++;
				myMem[currAdr] = currData;
				// cerr << "HIT IN MEMORY" << endl;
				// cerr << "function: " << __func__ << "  line: " << __LINE__ << "  currAdr: " << currAdr << " currData: " << currData << "  currIndex: " << currIndex << " currTag: " << currTag << " myMem[currAdr]: " << myMem[currAdr] << endl;
			}
			////
		}
	}
	cacheDump();
	cerr << "============================END Store========================================" << endl;
}
