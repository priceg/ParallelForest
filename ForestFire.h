#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define SIZE_X 4
#define SIZE_Y 4

#define MAX_GEN 25

#define EMPTY 0
#define TREE 1
#define BURNING 2

//Runs the simulation on the given forest
void run(int** forest);

//Allocates memory for forest
int** allocForest();

//Generates a new forest 
int** generateForest();

//Copies value from one forest to another
void copyForest(int** from, int** to);

//Returns if true if any of a cells neighbors are burning
bool isNeighborBurning(int** forest, int x, int y);

//Returns true if the trail is a success using the given probability
bool isSuccess(float probability);

//Prints the given forest to the console
void printForest(int** forest, int gen);