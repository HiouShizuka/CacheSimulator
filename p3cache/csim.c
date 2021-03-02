#include "cachelab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void helper(){  
	printf("Help info: see handout\n");
}

void print_verbose(char * line, char * instr, int hit, int miss, int eviction){
	if(miss) printf("miss ");
	if(eviction) printf("eviction ");
	if(hit>0) printf("hit ");
	if(--hit>0) printf("hit");
	printf("\n");
}

int * parse_args(int argc, char * argv[]){
	int* res = calloc(4, sizeof(int));
	for(int i=0; i<argc; i++){
		if(strcmp(argv[i], "-h") == 0){
			helper();
		}
		else if(strcmp(argv[i], "-v") == 0){
			res[0] = 1;
		}
		else if(strcmp(argv[i], "-s") == 0){
			res[1] = atoi(argv[i+1]);
		}
		else if(strcmp(argv[i], "-E") == 0){
			res[2] = atoi(argv[i+1]);
		}
		else if(strcmp(argv[i], "-b") == 0){
			res[3] = atoi(argv[i+1]);
		}
	}
	return res;
}

int * process_block(char * instr, long ** cache, int ** lru, int set, int associativity, long mask, long pos, int op){
	int hits = 0, misses = 0, evictions = 0;

	long tag = pos >> set;
	int set_num = (int)(pos & mask);

	// 1. check if tag exists in the set
	int set_full = 1;
	for(int i=0; i<associativity; i++){
		if(cache[set_num][i] == tag && lru[set_num][i] != 0){
			hits = 1;
			lru[set_num][i] = op;
		} 
		if(lru[set_num][i] == 0){
			set_full = 0;
		} 
	}

	// 2. if tag not exists in the set, put it into set (evict lru block if necessary)
	if(hits == 0){
		misses = 1;
		if(set_full){
			evictions = 1;
			int least = op, evict_index = -1;
			for(int i=0; i<associativity; i++){
				if(lru[set_num][i] < least){
					least = lru[set_num][i];
					evict_index = i;
				}
			}
			cache[set_num][evict_index] = tag;
			lru[set_num][evict_index] = op;
		}
		else{
			for(int i=0; i<associativity; i++){
				if(cache[set_num][i] == 0 && lru[set_num][i] == 0){
					cache[set_num][i] = tag;
					lru[set_num][i] = op;
					break;
				}
			}
		}
	}

	if(strcmp(instr, "M") == 0) hits += 1;

	int * res = (int *)calloc(3, sizeof(int));
	res[0] = hits;
	res[1] = misses;
	res[2] = evictions;
	return res;

}

char * substring(char * str, int start){
	char * substr = (char *)calloc(strlen(str), sizeof(char));
	for(int i=start; i<strlen(str)-1; i++){
		substr[i-start] = str[i];
	}
	substr[strlen(str)-start-1] = '\0';
	return substr;
}

int * process_line(int verbose, char * line, long ** cache, int ** lru, int set, int associativity, int block, int op, long mask){
	if(line[0] != ' ') return NULL; //ignore instructions

    if(verbose) printf("%s ", substring(line, 1)); 

    char * instr = strtok(line, " "); 
    char * token = strtok(NULL, " "); 
    char * addr_str = strtok(token, ",");

    long addr = (long)strtol(addr_str, NULL, 16) >> block;

    int * res = process_block(instr, cache, lru, set, associativity, mask, addr, op);
    
    if(verbose) print_verbose(line, instr, res[0], res[1], res[2]);
   
	return res;
}

void simulate(int verbose, int set, int associativity, int block, FILE * fptr){

	int hits = 0, misses = 0, evictions = 0, op = 1;

	/* matrix cache stores tags, matrix lru stores the latest time of use for each tag. */
	long ** cache = (long **)calloc(2<<set, sizeof(long*));
	int ** lru = (int **)calloc(2<<set, sizeof(int*));
	for(int i=0; i<2<<set; i++){
		cache[i] = (long *)calloc(associativity, sizeof(long));
		lru[i] = (int *)calloc(associativity, sizeof(int));
	}

	/* mask to get set number*/
	long mask = 1;
    for(int i=0; i<set-1; i++){
    	mask = mask << 1;
    	mask += 1;
    }

    /* process each line*/
	char * line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, fptr)) != -1) {
        int * result = process_line(verbose, line, cache, lru, set, associativity, block, op, mask);
        if(result != NULL){
        	hits += result[0];
        	misses += result[1];
        	evictions += result[2];
        	op += 1;
        }
        free(result);
    }

    for(int i=0; i<set; i++){
    	free(cache[i]);
    	free(lru[i]);
    }
    free(cache);
    free(lru);

    printSummary(hits, misses, evictions);
}

int main(int argc, char * argv[]){
	int* args = parse_args(argc, argv);
	FILE * fptr;
	if((fptr = fopen(argv[argc-1], "r")) == NULL){
		printf("File not found.\n");
		exit(1);
	}
	simulate(args[0], args[1], args[2], args[3], fptr);
	free(args);
	fclose(fptr);
	return 0;
}

