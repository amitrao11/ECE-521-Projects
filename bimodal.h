#ifndef BIMODAL_H
#define BIMODAL_H

#define TAKEN 1
#define NOT_TAKEN 0

#include <stdio.h>
#include <math.h>

using namespace std;

class bimodal{
protected:
	unsigned long index, prediction, misprediction;
	char predicted_branch;
	int mask, size;
	int *predictor_table;

public:
	void init(int i_B) {
		index = 0;
    prediction = misprediction = 0;
		size = (int)pow(2, (double) i_B);
		mask = size - 1;
   	predictor_table = new int[size];

    for(int i = 0; i < size; i++)
      predictor_table[i] = 2;
   	}

  void set_index(unsigned long address) {
    index = address >> 2;
    index = index & mask;
  }

  void access(unsigned long address, char actual_branch) {
   	prediction++;

    set_index(address);
    predicted_branch = predictor_table[index] >= 2 ? 't' : 'n';

   	update_predictor(actual_branch, predicted_branch);
	}

	void update_predictor(char actual_branch, char predicted_branch) {
    	if (actual_branch == 't') {
    		if (predictor_table[index] < 3)
        		predictor_table[index]++;
    
    		if (predicted_branch == 'n'){
        		misprediction++;
        		return;
        	}
    	}

    	else if (actual_branch == 'n') {
      		if (predictor_table[index] > 0)
        		predictor_table[index]--;
      		
      		if (predicted_branch == 't') {
            	misprediction++;
           		return;
        	}
    	}
	}

	int is_taken(unsigned long address) {
    	set_index(address);
      return predictor_table[index] >= 2 ? TAKEN : NOT_TAKEN;
	}

	void print_output() {
		printf("OUTPUT\n");
		printf("number of predictions: %lu\n", prediction);
		printf("number of mispredictions: %lu\n", misprediction);
		printf("misprediction rate:     %.2f%%\n", (float)misprediction*100/prediction);
	}

	void print_stats() {
	    printf("FINAL BIMODAL CONTENTS\n");
    	for (int i = 0; i < size; i++)
        printf("%d\t%d\n", i, predictor_table[i]);
	}
};

#endif
