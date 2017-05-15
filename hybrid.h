#ifndef HYBRID_H
#define HYBRID_H

#define TAKEN 1
#define NOT_TAKEN 0

#include <stdio.h>
#include <math.h>
#include "bimodal.h"
#include "gshare.h"

using namespace std;

class hybrid : public bimodal, public gshare {
protected:
	unsigned long index, prediction, misprediction;
	char predicted_branch;
	int mask, size;
	int bimodal_predict, gshare_predict;
	int *chooser_table;
	bimodal *b;
	gshare *g;

public:
	void init (int i_B, int i_G, int h, int k) {
		index = 0;
		prediction = misprediction = 0;
		
		b = new bimodal;
		b->init(i_B);
		
		g = new gshare;
		g->init(i_G, h);

		size = (int)pow(2, (double) k);
		mask = size - 1;

		chooser_table = new int[size];

		for(int i = 0; i < size; i++)
      		chooser_table[i] = 1;
	}

	void set_index (unsigned long address) {
    	index = address >> 2;
    	index = index & mask;
  	}

  	void check_misprediction(int predicted, char actual_branch) {
  		if ((predicted && actual_branch == 'n') || (!predicted && actual_branch == 't'))
  			misprediction++;
  	}

  	int check_correct_prediction(int predicted, char actual_branch) {
  		return ((predicted && actual_branch == 't') || (!predicted && actual_branch == 'n'));
  	}

  	void access (unsigned long address, char actual_branch) {
  		prediction++;

  		set_index(address);
  		bimodal_predict = b->is_taken(address);
  		gshare_predict = g->is_taken(address);

  		if (chooser_table[index] < 2) {
  			check_misprediction(bimodal_predict, actual_branch);
  			(actual_branch == 't') ? g->update_gbh(1) : g->update_gbh(0);
  			b->access(address, actual_branch);
  		}
  		else {
  			check_misprediction(gshare_predict, actual_branch);
  			g->access(address, actual_branch);
  		}

  		int bimodal_correct = check_correct_prediction(bimodal_predict, actual_branch);
  		int gshare_correct = check_correct_prediction(gshare_predict, actual_branch);

  		if (bimodal_correct && !gshare_correct)
  			if (chooser_table[index] > 0)
  				chooser_table[index]--;

  		if (gshare_correct && !bimodal_correct)
  			if (chooser_table[index] < 3)
  				chooser_table[index]++;
  	}

  	void print_output() {
		printf("OUTPUT\n");
    	printf("number of predictions: %lu\n", prediction);
    	printf("number of mispredictions: %lu\n", misprediction);
    	printf("misprediction rate:     %.2f%%\n", (float)misprediction*100/prediction);
	}

	void print_stats() {
	    printf("FINAL CHOOSER CONTENTS\n");
      	for (int i = 0; i < size; i++)
        	printf("%d\t%d\n", i, chooser_table[i]);

        g->print_stats();
        b->print_stats();
	}
};

#endif