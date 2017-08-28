#include <iostream>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#define INVALID 0
#define VALID 1
#define DIRTY 2

#define INITIATE 0
#define READMISS 1
#define WRITEMISS 2
#define HIT 3

#define LRU 0
#define FIFO 1

#define NON_INCLUSIVE 0
#define INCLUSIVE 1
#define EXCLUSIVE 2

int repl_policy = 0;
int inclusion = 0;
unsigned long l2_size = 0;

using namespace std;

class CacheLine {
private:
	unsigned long tag, index;
	unsigned long assoc, block_num, check_state;
public:
	CacheLine(unsigned long assoc) {
		this->assoc = assoc;
		this->check_state = INVALID;
		this->block_num = 0;
	}

	void setTag(unsigned long tag){
		this->tag = tag;
	}

	unsigned long getTag() const{
		return this->tag;
	}

	void setIndex(unsigned long index) {
		this->index = index;
	}

	unsigned long getIndex() const{
		return this->index;
	}

	void setBlockNum(unsigned long block_num) {
		this->block_num = block_num;
	}

	unsigned long getBlockNum() const {
		return this->block_num;
	}

	bool checkValidity() const {
		return (check_state != INVALID);
	}

	bool checkDirty() const {
		return (check_state == DIRTY);
	}

	void setCheckState (unsigned long state) {
		this->check_state = state;
	}

	unsigned long getCheckState() const {
		return check_state;
	}

	~CacheLine() {}
};

class Cache {
private:
	unsigned long cache_size, block_size, assoc;
	unsigned long num_set, block_offset_bits;

	unsigned long read, read_miss, write, write_miss;
	unsigned long check_miss_type;

	CacheLine*** cache_line;
	Cache* lower_cache;

	int count;
	bool evicted;
	unsigned long evicted_address;

	unsigned long num_writeBack;
	bool writeBack;

public:
	Cache(unsigned long cache_size, unsigned long block_size, unsigned long assoc) {
		if (assoc == 0) {
			cout << "\nError: Configuration Setting. \nAssociativity cannot be Zero." << endl;
			exit(1);
		}
		this->count = 0;
		this->cache_size = cache_size;
		this->block_size = block_size;
		this->assoc = assoc;
		this->num_set = cache_size / (block_size * assoc);
		this->block_offset_bits = (unsigned long) log2(block_size);

		read = write = read_miss = write_miss = 0;
		num_writeBack = 0;
		evicted_address = 0;

		this->cache_line = new CacheLine**[num_set];

		for (int i = 0; i < (int)num_set; i++) {
			cache_line[i] = new CacheLine*[assoc];
			for (int j = 0; j < (int)assoc; j++) {
				cache_line[i][j] = new CacheLine(assoc);
			}
		}
		this->lower_cache = NULL;
	}

	CacheLine* findCacheBlock(unsigned long address) {
		unsigned long tag = getTag(address);
		CacheLine** cache_line = this->cache_line[getIndex(address)];

		for (int i = 0; i < (int) assoc; i++) {
			if ((tag == cache_line[i]->getTag()) && cache_line[i]->checkValidity())
				return cache_line[i];
		}
		return NULL;
	}

	void incrementLRU(CacheLine* block){
		block->setBlockNum(count++);
	}

	CacheLine* fillCacheBlock(unsigned long address) {	
		//int index = getIndex(address);
		CacheLine* free_block = getFreeBlock(address);

		if (evicted) // set victim address function
			evicted_address = getAddress(free_block->getTag());

		if (free_block != NULL && free_block->checkDirty()){
			num_writeBack++;
			writeBack = true;
		}

		free_block->setTag(getTag(address));
		free_block->setCheckState(VALID);
		incrementLRU(free_block);
		return free_block;
	}

	CacheLine* getFreeBlock(unsigned long address) {
		CacheLine* free_block = NULL;
		int index = getIndex(address);

		for (int i = 0; i < (int) assoc; i++) {
			if (!cache_line[index][i]->checkValidity())
				return cache_line[index][i];
		}
		
		if (free_block == NULL) {
			evicted = true;
			free_block = cache_line[index][0];
			for (int i = 1; i < (int) assoc; i++) {
				if (cache_line[index][i]->getBlockNum() < free_block->getBlockNum()) {
					free_block = cache_line[index][i];
				}
			}
		}
		return free_block;
	}

	void accessCache(char operation, unsigned long address, bool l2_ex) {
		evicted = false;
		writeBack = false;
		check_miss_type = INITIATE;

		if (operation == 'r')
			read++;
		else
			write++;

		CacheLine* cache_line = findCacheBlock(address);

		if (cache_line == NULL) {
			if (operation == 'w') {
				write_miss++;
				check_miss_type = WRITEMISS;
			}
			else {
				read_miss++;
				check_miss_type = READMISS;
			}

			if (l2_ex)
				cache_line = fillCacheBlock(address);
		}
		else {
			check_miss_type = HIT;
			if (repl_policy == LRU)
				incrementLRU(cache_line);
		}

		if (operation == 'w')
			cache_line->setCheckState(DIRTY);
	}

	unsigned long getIndex(unsigned long address) const {
		unsigned long index, tagslack;
		tagslack = (unsigned long) (num_set - 1);
		index = getTag(address) & tagslack;
		assert(index < num_set);
		return index;
	}

	unsigned long getTag(unsigned long address) const {
		return (address >> this->block_offset_bits);
	}

	unsigned long getAddress(unsigned long tag) const {
		return (tag << this->block_offset_bits);
	}

	void lowerCache(Cache* lower_cache){
		this->lower_cache = lower_cache;
	}

	unsigned long getEvictedAddress() const {
		return evicted_address;
	}

	bool CheckWriteBack() const {
		return writeBack;
	}

	unsigned long getMissType() const {
		return check_miss_type;
	}

	void Invalidate (unsigned long address) {
		CacheLine* Invalidate_cache;
		Invalidate_cache = findCacheBlock(address);

		if (Invalidate_cache){
			if (inclusion == EXCLUSIVE)
				Invalidate_cache->setCheckState(INVALID);

			if (inclusion == INCLUSIVE && lower_cache) {
				if (Invalidate_cache->checkDirty()) {
					writeBack = true;
					num_writeBack++;
					lower_cache->writeBack = true;
					lower_cache->num_writeBack++;
				}
				Invalidate_cache->setCheckState(INVALID);
			}
		}
	}

	bool checkEvicted() {
		return evicted;
	}

	float CalculateMissRates() {
		float rate;
		if (read || write) {
			rate = (float)(read_miss + write_miss)/(read + write);
			return rate;
		}
		else
			return 0;
	}

	unsigned long calculateMemTraffic() {
		unsigned long memTraffic;
		Cache* which_Cache = (l2_size) ? this->lower_cache : this;
		memTraffic = which_Cache->read_miss + which_Cache->write_miss + which_Cache->num_writeBack;
		return memTraffic;
	}

	void displayStat() {
		printf("===== Simulation results (raw) =====\n");
		printf("a. number of L1 reads: 		%lu\n", read);
		printf("b. number of L1 read misses:	%lu\n", read_miss);
		printf("c. number of L1 writes: 	%lu\n", write);
		printf("d. number of L1 write misses:	%lu\n", write_miss);
		printf("e. L1 miss rate: 		%.6f\n", CalculateMissRates());
		printf("f. number of L1 writebacks:	%lu\n", num_writeBack);
		if (l2_size) {
			printf("g. number of L2 reads:  	%lu\n", lower_cache->read);
			printf("h. number of L2 read misses:	%lu\n", lower_cache->read_miss);
			printf("i. number of L2 writes:		%lu\n", lower_cache->write);
			printf("j. number of L2 write misses:	%lu\n", lower_cache->write_miss);
			printf("k. L2 miss rate: 		%.6f\n", lower_cache->CalculateMissRates());
			printf("l. number of L2 writebacks:	%lu\n", lower_cache->num_writeBack);
		} else {
			printf("g. number of L2 reads:  	%d\n", 0);
			printf("h. number of L2 read misses:	%d\n", 0);
			printf("i. number of L2 writes:		%d\n", 0);
			printf("j. number of L2 write misses:	%d\n", 0);
			printf("k. L2 miss rate: 		%d\n", 0);
			printf("l. number of L2 writebacks:	%d\n", 0);
		}
			printf("m. total memory traffic: 	%lu\n", calculateMemTraffic());

	}

	~Cache() {}
};

int main(int argc, char* argv[]){
	unsigned long block_size = atoi(argv[1]);

	if (block_size == 0) {
		cout << "\nError: Block Size cannot be zero" << endl;
		exit(0);
	}

	unsigned long l1_size = atoi(argv[2]);
	unsigned long l1_assoc = atoi(argv[3]);
	l2_size = atoi(argv[4]);
	unsigned long l2_assoc = atoi(argv[5]);
	
	repl_policy = atoi(argv[6]);
	inclusion = atoi(argv[7]);
	char* trace_file = argv[8];

	FILE* file_name = fopen(trace_file, "r");

	if (file_name == 0) {
		cout << "\nError Opening File: Trace File not found." << endl;
		exit(1);
	}
	
	printf("\n===== Simulator configuration =====");
	printf("\nBLOCKSIZE:\t\t%lu",block_size);
	printf("\nL1_SIZE:\t\t%lu",l1_size);
	printf("\nL1_ASSOC:\t\t%lu",l1_assoc);
	printf("\nL2_SIZE:\t\t%lu",l2_size);
	printf("\nL2_ASSOC:\t\t%lu",l2_assoc);

	if (!repl_policy)
		printf("\nREPLACEMENT POLICY:\t%s", "LRU");
	else
		printf("\nREPLACEMENT POLICY:\t%s", "FIFO");

	if (inclusion == 0)
		printf("\nINCLUSION PROPERTY:\t%s","non-inclusive");
	else if (inclusion == 1)
		printf("\nINCLUSION PROPERTY:\t%s","inclusive");
	else
		printf("\nINCLUSION PROPERTY:\t%s","exclusive");

	printf("\ntrace_file:\t\t%s",trace_file);
	printf("\n");

	Cache* cache_l1 = new Cache(l1_size, block_size, l1_assoc);
	Cache* cache_l2;

	if (l2_size) {
		cache_l2 = new Cache(l2_size, block_size, l2_assoc);
		cache_l1->lowerCache(cache_l2);
	}

	unsigned long address = 0;
	char read_from_file[20];


	while(fgets(read_from_file, 20, file_name) != NULL) {
		char* token = strtok(read_from_file, " ");
		char* operation = token;

		token = strtok(NULL, " ");
		address = strtol(token, NULL, 16);

		cache_l1->accessCache(operation[0], address, true);

		if (l2_size) {
			if (inclusion == NON_INCLUSIVE) {
				if (cache_l1->CheckWriteBack())
					cache_l2->accessCache('w', cache_l1->getEvictedAddress(), true);
				if ((cache_l1->getMissType() == READMISS) || (cache_l1->getMissType() == WRITEMISS))
					cache_l2->accessCache('r', address, true);
			}

			if (inclusion == EXCLUSIVE) {
				if (cache_l1->getMissType() == HIT)	cache_l2->Invalidate(address);
				
				if (cache_l1->checkEvicted()) {
					cache_l2->accessCache('w', cache_l1->getEvictedAddress(), true);
					if (!cache_l1->CheckWriteBack())
						cache_l2->findCacheBlock(cache_l1->getEvictedAddress())->setCheckState(VALID);
				}
				
				if ((cache_l1->getMissType() == READMISS) || (cache_l1->getMissType() == WRITEMISS)) {
					cache_l2->accessCache('r', address, false);

					if (cache_l2->getMissType() == HIT) {
						if (cache_l2->findCacheBlock(address)->checkDirty())
							cache_l1->findCacheBlock(address)->setCheckState(DIRTY);
						cache_l2->Invalidate(address);
					}
				}
			}

			if (inclusion == INCLUSIVE){
				if (cache_l1->CheckWriteBack()) {
					cache_l2->accessCache('w', cache_l1->getEvictedAddress(), true);
					if (cache_l2->checkEvicted())
						cache_l1->Invalidate(cache_l2->getEvictedAddress());
				}

				if ((cache_l1->getMissType() == READMISS) || (cache_l1->getMissType() == WRITEMISS)) {
					cache_l2->accessCache('r', address, true);
					if (cache_l2->checkEvicted())
						cache_l1->Invalidate(cache_l2->getEvictedAddress());
				}
			}
		}
	}

	cache_l1->displayStat();

	return 0;
}