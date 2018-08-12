#include "ForestFire.h"
#include "mpi.h"

int tMax;//Burn time of the largest cluster
float pTree;//pTree <  tMax
float pIgnite; //pIgnite < pTree
char dirName[25];

int main(int argc, char *argv[]){
	pTree = 0.001;
	pIgnite = 0.0001;
	int* forest;
	int numranks, rank;//variables to hold the number of ranks and rank of each individual node

	//we are now using MPI to parallelize the code. This block of code is the MPI initialization
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numranks);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if(rank == 0){//Rank 0 is the master rank and will handle image generation. This is why only it creates the forest
	 forest = generateForest();
	}

	//The following block of code scatters the data of the matrix evenly to each of the nodes.
	//The master node (node 0) will get none of the data because it will only be handling the image generation.
	int* chunk = malloc((SIZE_X/(numranks - 1) + 2) * SIZE_Y * sizeof(int));
	int* count = malloc(numranks * sizeof(int));
	int* offSet = malloc(numranks * sizeof(int));
	int chunkSize = SIZE_X*SIZE_Y/(numranks -1);
	count[0] = 0;
	offSet[0] = 0;
	for(int i = 1; i < numranks; i++)
	{
		count[i] = chunkSize;
		offSet[i] = chunkSize * (i-1);
	}
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Scatterv(forest, count, offSet, MPI_INT, chunk, chunkSize, MPI_INT, 0, MPI_COMM_WORLD);

	float start = MPI_Wtime();
	run(forest, chunk, rank, (numranks - 1), chunkSize, count, offSet);
	float end = MPI_Wtime();
	printf("Rank %d time: %f \n", rank, end - start)
	MPI_Finalize();
}

void saveForestToImage(int* forest, int gen) {
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
	for(int z = 0; z <SIZE_X * SIZE_Y; z++){
			switch(forest[z]){
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

void run(int* whole, int* forest, int rank, int numranks, int size, int* counts, int* offSets){

		int gen = 0;
		int* bufferChunk = malloc((SIZE_X/numranks + 2) * SIZE_Y * sizeof(int));
		copyForest(forest, bufferChunk, size);
		int sendSize;
		if(rank == 0)
		sendSize = 0;
		else
		sendSize = size;

	while (gen < MAX_GEN){

		//The entire image is gathered into node 0 at the beginning of each generation to create the image.
		MPI_Gatherv(bufferChunk, sendSize, MPI_INT,
                whole, counts, offSets,
                MPI_INT, 0, MPI_COMM_WORLD);

		if(rank == 0)
		{
		saveForestToImage(whole, gen);
		}
		else{
			//The following block is the MPI halo exchange.
			//Each node was given extra space in its buffer to represent the neighbors of the edges.
			//It will receive the data from the previous and next node
			//to allow the edges of the node's chunk to function correctly
			int next = rank + 1;
			int prev = rank -1;
			if(next>numranks) next=MPI_PROC_NULL;
			if(prev<1) prev=MPI_PROC_NULL;
			if(rank % 2 != 0)
			{
				MPI_Sendrecv (&forest[((SIZE_X/numranks + 2) * SIZE_Y) - 2 * SIZE_X],SIZE_X,MPI_INT,next,0,&forest[(SIZE_X/numranks + 2) * SIZE_Y - SIZE_X],SIZE_X,MPI_INT,next,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
				MPI_Sendrecv (&forest[SIZE_X],SIZE_X,MPI_INT,prev,0,&forest[0],SIZE_X,MPI_INT,prev,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
			}
			else
			{
				MPI_Sendrecv (&forest[SIZE_X],SIZE_X,MPI_INT,prev,0,&forest[0],SIZE_X,MPI_INT,prev,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
				MPI_Sendrecv (&forest[(SIZE_X/numranks + 2) * SIZE_Y - 2 * SIZE_X],SIZE_X,MPI_INT,next,0,&forest[(SIZE_X/numranks + 2) * SIZE_Y - SIZE_X],SIZE_X,MPI_INT,next,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
			}


		for (int z = SIZE_X; z < (SIZE_X/numranks + 2) * SIZE_Y - SIZE_X; z++){
				switch (forest[z]){
					case EMPTY:
						//pTree chance cell = tree
						if(isSuccess(pTree)){
							bufferChunk[z] = TREE;
						}
					break;
					case TREE: //tree
						// pIgnite chance cell is burning or if one neighbor is burning cell = burning
						if(isSuccess(pIgnite) || isNeighborBurning(forest, z)){
							bufferChunk[z] = BURNING;
						}
					break;
					case BURNING:
						bufferChunk[z] = EMPTY;
					break;
				}

		}
		copyForest(bufferChunk, forest, size);
	}
		MPI_Barrier(MPI_COMM_WORLD);
		gen++;
	}
}

int* allocForest(){
	int* forest = malloc(SIZE_X * SIZE_Y * sizeof(int));
	return forest;
}

int* generateForest(){
	int* forest = allocForest();
	for(int z = 0; z < SIZE_X * SIZE_Y; z++){
		int rowZ = z/SIZE_X;
		int colZ = z%SIZE_X;
		if((rowZ >= 500 && rowZ <= 675 && colZ >= 500 && colZ <= 675))
			forest[z] = 1;
	}
	forest[587587] = 2;
	return forest;
}

void copyForest(int* from, int* to, int chunkSize){
	for(int z = 0; z < chunkSize; z++){
			to[z] = from[z];
	}
}

bool isNeighborBurning(int* forest, int z){
	int above = z - SIZE_X;
	int below = z + SIZE_X;
	int right = z - 1;
	int left = z + 1;
	return (above >= 0 && forest[above] == BURNING) ||
		   (below < SIZE_X && forest[below] == BURNING) ||
		   (right >= 0 && forest[right] == BURNING) ||
		   (left < SIZE_Y && forest[left] == BURNING);
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
