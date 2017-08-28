#ifndef GSHARE_H
#define GSHARE_H

#define TAKEN 1
#define NOT_TAKEN 0

#include <stdio.h>
#include <math.h>

class gshare{
protected:
	unsigned long index, prediction, misprediction;
	char predicted_branch;
	int mask1, mask2, global_branch_history, xor_output, size;
	int *predictor_table;
	int i_G, h;

public:
	void init(int i_G, int h) {
		index = 0;
    prediction = misprediction = 0;
		global_branch_history = xor_output = 0;

		this->i_G = i_G;
		this->h = h;

  	size = (double)pow(2, (double)i_G);
  	mask1 = size - 1;
  	mask2 = (double)pow(2, (double)i_G - h) - 1;

		predictor_table = new int[size];
		for(int i = 0; i < size; i++)
			predictor_table[i] = 2;
	}

  void set_index(unsigned long address) {
    unsigned long temp;
    temp = address >> 2;
    temp = temp & mask1;

    if (h){
    xor_output = global_branch_history ^ (temp >> (i_G - h));
    xor_output = xor_output << (i_G - h);

    temp = temp & mask2;
    index = xor_output | temp;
    }
    else
      index = temp;
  }

	void access(unsigned long address, char actual_branch) {
		prediction++;
		
    set_index(address);
    predicted_branch = predictor_table[index] >= 2 ? 't' : 'n';

    update_predictor(actual_branch, predicted_branch);
	}

	void update_predictor(char actual_branch, char predicted_branch) {
    	if(actual_branch == 't'){
      		if(predictor_table[index] < 3)
          		predictor_table[index]++;
      		update_gbh(1);
      		if(predicted_branch == 'n'){
          		misprediction++;
          		return;
      		}
		  }
    	
    	else if(actual_branch == 'n'){
      		if(predictor_table[index] >0)
          		predictor_table[index]--;
      		update_gbh(0);
      		if(predicted_branch == 't'){
          		misprediction++;
          		return;
      		} 
    	}
	}

	void update_gbh(int i){
      if(h){
      	global_branch_history = global_branch_history >> 1; 
      	if(i == 1)
          	global_branch_history = global_branch_history | (1 << (h - 1));
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
	    printf("FINAL GSHARE CONTENTS\n");
      for (int i = 0; i < size; i++)
        printf("%d\t%d\n", i, predictor_table[i]);
	}
};

#endif
