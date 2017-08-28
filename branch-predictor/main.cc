#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>

#include "hybrid.h"

using namespace std;

int main(int argc, char *argv[]) {

	if (argc < 4 || argc > 7) {
		cout << "Error: Invalid number of Arguments." << endl;
		exit(0);
	}

	char *type = argv[1];
	
	int i_B;
	bimodal *b = new bimodal;

	int i_G, h;
	gshare *g = new gshare;

	int k;
	hybrid *hy = new hybrid;

	char *trace_file;

	if (strcmp(type,"bimodal") == 0)
		trace_file = argv[3];
	else if (strcmp(type,"gshare") == 0)
		trace_file = argv[4];
	else if (strcmp(type,"hybrid") == 0)
		trace_file = argv[6];

	FILE *file_name = fopen(trace_file, "r");
	if (file_name == 0) {
		cout << "Error: Trace File not found." << endl;
		exit(1);
	}

	unsigned long address;
	char* actual_branch;

	char read_from_file[20];
	
	if (strcmp(type,"bimodal") == 0) {
		i_B = atoi(argv[2]); 
		b->init(i_B);
		printf("COMMAND\n./sim_bp %s %d %s\n", type, i_B, trace_file);

		while(fgets(read_from_file, 20, file_name) != NULL) {
			char* token = strtok(read_from_file, " ");
			address = strtol(token, NULL, 16);

			token = strtok(NULL, " ");
			actual_branch = token;
 
			b->access(address, actual_branch[0]);
		}
		b->print_output();
		b->print_stats();
	}

	else if (strcmp(type,"gshare") == 0) {
		i_G = atoi(argv[2]);
		h = atoi(argv[3]);
		if(h > i_G){
			cout << "Error: Value of h greater than iG." << endl;
			exit(0);
		}
		g->init(i_G, h);
		printf("COMMAND\n./sim_bp %s %d %d %s\n", type, i_G, h, trace_file);

		while(fgets(read_from_file, 20, file_name) != NULL) {
			char* token = strtok(read_from_file, " ");
			address = strtol(token, NULL, 16);

			token = strtok(NULL, " ");
			actual_branch = token;

			g->access(address, actual_branch[0]);
		}
		g->print_output();
		g->print_stats();
	}
	else if (strcmp(type,"hybrid") == 0) {
		k = atoi(argv[2]);
		i_G = atoi(argv[3]);
		h = atoi(argv[4]);
		if(h > i_G){
			cout << "Error: Value of h greater than iG." << endl;
			exit(0);
		}
		i_B = atoi(argv[5]);
		printf("COMMAND\n./sim_bp %s %d %d %d %d %s\n", type, k, i_G, h, i_B, trace_file);
		hy->init(i_B, i_G, h, k);

		while(fgets(read_from_file, 20, file_name) != NULL) {
			char* token = strtok(read_from_file, " ");
			address = strtol(token, NULL, 16);

			token = strtok(NULL, " ");
			actual_branch = token;
			
			hy->access(address, actual_branch[0]);
		}

		hy->print_output();
		hy->print_stats();
	}
	else {
		cout << "Error: Invalid predictor type." << endl;
		exit(1);
	}

	return 0;
}
