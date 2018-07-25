#include "ForestFire.h"

int tMax;//Burn time of the largest cluster
float pTree;//pTree <  tMax
float pIgnite; //pIgnite < pTree
char dirName[25];

int main(int argc, char *argv[]){
	pTree = 0.05;
	pIgnite = 0.04;
	int** forest = generateForest();
	run(forest);
}

void saveForestToImage(int** forest, int gen) {
	if(gen == 0){
		getDirName(dirName);
		mkdir(dirName, 0777);
	}
	FILE* file;
	char fileName[50];
	sprintf(fileName, "%s/%s%d.ppm", dirName, "ForestFireModel", gen);
	
    file = fopen(fileName, "wb");

    fprintf(file, "P6\n");
    fprintf(file, "%d %d\n", SIZE_X, SIZE_Y);
    fprintf(file, "255\n");
	for(int x = 0; x < SIZE_X; x++){
		for(int y = 0; y < SIZE_Y; y++){
			switch(forest[x][y]){
				case EMPTY:
					fprintf(file, "%c%c%c", 0, 0, 0);
				break;
				case TREE:
					fprintf(file, "%c%c%c", 34, 139, 34);
				break;
				case BURNING:
					fprintf(file, "%c%c%c", 242, 125, 12);
				break;
			}
		}
	}
    fclose(file);
}

void getDirName(char* dirName){
	struct stat st = {0};
	char* baseDirName = "ForestFireModel";
	int index = 0;
	do{
		sprintf(dirName, "%s%d", baseDirName, index);
		index++;
	}while(stat(dirName, &st) != -1);
}

void run(int** forest){
	int** bufferForest = allocForest();
	copyForest(forest, bufferForest);
	int gen = 0;
	while (gen < MAX_GEN){
		printForest(forest, gen);
		saveForestToImage(forest, gen);
		for (int x = 0; x < SIZE_X; x++){
			for (int y = 0; y < SIZE_Y; y++) {
				switch (forest[x][y]){
					case EMPTY: 
						//pTree chance cell = tree
						if(isSuccess(pTree)){
							bufferForest[x][y] = TREE;
						}
					break;
					case TREE: //tree
						// pIgnite chance cell is burning or if one neighbor is burning cell = burning
						if(isSuccess(pIgnite) || isNeighborBurning(forest, x, y)){
							bufferForest[x][y] = BURNING;
						}
					break;
					case BURNING: 
						bufferForest[x][y] = EMPTY;
					break;
				}
			}
		}
		copyForest(bufferForest, forest);
		gen++;
	}
}

int** allocForest(){
	int** forest = malloc(SIZE_X * sizeof(*forest));
	for (int x = 0; x < SIZE_X; x++) {
		forest[x] = malloc(SIZE_Y * sizeof(*forest[x]));
	}
	return forest;
}

int** generateForest(){
	int** forest = allocForest();
	for(int x = 0; x < SIZE_X; x++){
		for(int y = 0; y < SIZE_Y; y++){
			forest[x][y] = 0;
		}
	}
	forest[0][1] = 1;
	forest[2][1] = 1;
	forest[3][1] = 1;
	forest[2][2] = 1;
	forest[3][2] = 1;
	return forest;
}

void copyForest(int** from, int** to){
	for(int x = 0; x < SIZE_X; x++){
		for(int y = 0; y < SIZE_Y; y++){
			to[x][y] = from[x][y];
		}
	}
}

bool isNeighborBurning(int** forest, int x, int y){
	int above = x - 1;
	int below = x + 1;
	int right = y - 1;
	int left = y + 1;
	return (above >= 0 && forest[above][y] == BURNING) ||
		   (below < SIZE_X && forest[below][y] == BURNING) ||
		   (right >= 0 && forest[x][right] == BURNING) ||
		   (left < SIZE_Y && forest[x][left] == BURNING);
}

bool isSuccess(float probability){
	float randVal = (float)rand() / (RAND_MAX);
	return randVal <= probability;	
}

void printForest(int** forest, int gen){
	printf("Generation: %d\n", gen);
	for(int x = 0; x < SIZE_X; x++){
		for(int y = 0; y < SIZE_Y; y++){
			printf("%d", forest[x][y]);
		}
		printf("\n");
	}
	printf("\n");
}