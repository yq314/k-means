/*
 * kmeans.c
 *
 *  Created on: Mar 22, 2013
 *      Author: qingye
 */

#include "kmeans.h"

/*
 * Print the usage of this programme
 */
void help(){
	printf("Usage: \n");
	printf("<-i inputFileName>	:	input data file path and name\n");
	printf("[-k k-means]		:	the number of k, should be larger than 0, default 9\n");
	printf("[-h]			:	print this help\n");
}

/*
 * Parse the command line arguments and setup the options
 *
 * This function will change the value of inputFileName and k
 *
 * @param argc			int		number of arguments
 * @param argv			char**	list of arguments
 * @param inputFileName	char**	input file path and name
 * @param k				int*	k-means
 *
 * @return void
 */
void getCmdOptions(int argc, char **argv, char **inputFileName, int *k){
	int c;
	opterr = 0;

	while((c = getopt(argc, argv, "i:k:h")) != -1){
		switch(c){
			case 'i':
				*inputFileName = (char *)malloc(strlen(optarg) * sizeof(optarg));
				strcpy(*inputFileName, optarg);
				break;
			case 'k':
				*k = atoi(optarg);
				break;
			case 'h':
				help();
				exit(0);
				break;
			default:
				printf("Illegal argument: %c\n", c);
		}
	}

	if(k == NULL || *k <= 0){
		*k = 9;
	}

	if(*inputFileName == NULL || strlen(*inputFileName) == 0){
		help();
		exit(0);
	}
}

/*
 * Reads the data points from input file
 *
 * This function will change the value of count
 *
 * @param fileName	char*			the file path and name to be read
 * @param count		int*			number of file lines
 *
 * @return data		Point*	array storing the points
 *
 */
Point *readData(char *fileName, int *count){
	FILE *pRead;
	Point *data = (Point *) malloc(sizeof(Point));
	float nX, nY;

	if((pRead = fopen(fileName, "r")) == NULL){
		printf("Fail to open file: %s", fileName);
		exit(-1);
	}

	*count = 0;
	while(fscanf(pRead, "%f %f\n", &nX, &nY) != EOF){
		data = (Point *) realloc(data, (*count+1) * sizeof(Point));
		data[*count].x = nX;
		data[*count].y = nY;
		++ *count;
	}
	fclose(pRead);

	return data;
}

/*
 * writes the labels and centroids into corresponding files
 *
 * @param labels	int*	The array storing cluster labels for each point
 * @param size		int		The size of data
 * @param centroids	Point*	The array storing k centroids
 * @param k			int		k-means
 *
 * @return void
 */
void writeToFile(int *labels, int size, Point *centroids, int k){
	char *outLabelFileName = "labels.txt";
	char *outCntrdFileName = "centroids.txt";
	FILE *pWrite;
	int i;

	/* write labels into file */
	if((pWrite = fopen(outLabelFileName, "w")) == NULL){
		printf("Fail to open output file: %s\n", outLabelFileName);
		exit(-1);
	}

	for(i = 0; i < size; i++){
		fprintf(pWrite, "%d\n", labels[i]);
	}

	fclose(pWrite);

	printf("Successfully wrote %d labels into file: %s\n", size, outLabelFileName);

	/* write centroids into file */
	if((pWrite = fopen(outCntrdFileName, "w")) == NULL){
		printf("Fail to open output file: %s\n", outCntrdFileName);
		exit(-1);
	}

	for(i = 0; i < k; i++){
		fprintf(pWrite, "%f %f\n", centroids[i].x, centroids[i].y);
	}

	fclose(pWrite);

	printf("Successfully wrote %d centroids into file: %s\n", k, outCntrdFileName);
}

/*
 * Initialize the centroids, randomly select initial points
 *
 * @param data	Point*	array of input points
 * @param size	int		number of points
 * @param k		int		number of clusters
 *
 * @return Point* array of centroids
 *
 */
Point *initialCentroids(Point *data, int size, int k){
	Point *c = (Point *) calloc(k, sizeof(Point));
	int i,j;

	if(k > size){
		k = size;
	}

	for(i = j = 0; i < k; i++){
		/*
		 * pick the first point from k chunks,
		 * it's not real random, but acceptable
		 */
		j += size/k;
		c[i].x = data[j].x;
		c[i].y = data[j].y;
	}
	return c;
}

/*
 * Sum the points, used in MPI_Reduce
 */
void sumPoint(void *in, void *inout, int *len, MPI_Datatype *dptr){
	int i;
	Point *p1 = (Point *) in;
	Point *p2 = (Point *) inout;
	Point *p = p2;

	for(i = 0; i < *len; i++){
		p2->x = p1->x + p2->x;
		p2->y = p1->y + p2->y;
		p1++;
		p2++;
	}

	inout = p;
}

/*
 * Main function
 */
int main(int argc, char **argv){

	char *inputFileName;
	int size;	/* line count of input data */
	Point *data;	/* input data points */
	Point *centroids; /* centroids */
	Point *tempC; /* temporary centroids array */
	Point *globalC; /* temporary global centroids array for MPI_Reduce */
	int *labels; /* label of clusters for each point */
	int *counts; /* number of points per cluster */
	int *globalCounts; /* global number of points per cluster for MPI_Reduce */
	int k, i, j, done, loops;
	float tempX, tempY, minDist, dist;

	/*defination for MPI*/
	int id; /* current process id */
	int p; /* number of processors */
	int chunkSize;
	double elapsed;
	MPI_Status status;
	MPI_Op MPI_Sum_point;
	Point *partialData;
	int *partialLabels;

	MPI_Init(&argc, &argv);
	MPI_Barrier(MPI_COMM_WORLD);
	elapsed = - MPI_Wtime();
	MPI_Comm_rank (MPI_COMM_WORLD, &id);
	MPI_Comm_size (MPI_COMM_WORLD, &p);

	/* Create MPI_POINT struct */
	MPI_Datatype MPI_POINT;
	MPI_Datatype type = MPI_FLOAT;
	int blockLen = 2;
	MPI_Aint displacement = 0;
	MPI_Type_create_struct(1, &blockLen, &displacement, &type, &MPI_POINT);
	MPI_Type_commit(&MPI_POINT);

	if(id == ROOT){
		k = 0;
		getCmdOptions(argc, argv, &inputFileName, &k);
		data = readData(inputFileName, &size);
		centroids = initialCentroids(data, size, k);

		/* sending data to slave processors */
		for(i = 1; i < p; i++){
			chunkSize = BLOCK_SIZE(i, p, size);
			printf("Sending %d data to process %d\n", chunkSize, i);
			MPI_Send(&chunkSize, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
			MPI_Send(&k, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
			MPI_Send(centroids, k, MPI_POINT, i, 0, MPI_COMM_WORLD);
			MPI_Send(data + BLOCK_LOW(i, p, size), chunkSize, MPI_POINT, i, 0, MPI_COMM_WORLD);
		}
		printf("All data sent.\n");

		chunkSize = BLOCK_SIZE(ROOT, p, size);
		partialData = (Point *) realloc(data, chunkSize * sizeof(Point));
	} else {
		/* Recieving data from root processor */
		MPI_Recv(&chunkSize, 1, MPI_INT, ROOT, 0, MPI_COMM_WORLD, &status);
		MPI_Recv(&k, 1, MPI_INT, ROOT, 0, MPI_COMM_WORLD, &status);
		centroids = (Point *) calloc(k, sizeof(Point));
		MPI_Recv(centroids, k, MPI_POINT, ROOT, 0, MPI_COMM_WORLD, &status);
		partialData = (Point *) calloc(chunkSize, sizeof(Point));
		MPI_Recv(partialData, chunkSize, MPI_POINT, ROOT, 0, MPI_COMM_WORLD, &status);
		printf("Process %d recieved %d data\n", id, chunkSize);
	}

	partialLabels = (int *) calloc(chunkSize, sizeof(int));
	counts = (int *) calloc(k, sizeof(int));
	globalCounts = (int *) calloc(k, sizeof(int));
	tempC = (Point *) calloc(k, sizeof(Point));
	globalC = (Point *) calloc(k, sizeof(Point));
	MPI_Op_create(sumPoint, TRUE, &MPI_Sum_point);
	done = TRUE;
	loops = 0;
	do{
		/* initialize the helper arrays */
		for(i = 0; i < k; i++){
			counts[i] = 0;
			tempC[i].x = 0;
			tempC[i].y = 0;
		}

		for(i = 0; i < chunkSize; i++){
			minDist = FLT_MAX;
			for(j = 0; j < k; j++){
				/* no need to compute the sqrt, we just need the value for comparison */
				dist = pow(partialData[i].x - centroids[j].x, 2) +
						pow(partialData[i].y - centroids[j].y, 2);
				if(dist < minDist){
					minDist = dist;
					partialLabels[i] = j;
				}
			}

			++counts[partialLabels[i]];

			/*
			 * simply add on the x and y of each point,
			 * for further computation of new centroid
			 */
			tempC[partialLabels[i]].x += partialData[i].x;
			tempC[partialLabels[i]].y += partialData[i].y;
		}
		/* reduce the temporary centroids and counts */
		MPI_Reduce(tempC, globalC, k, MPI_POINT, MPI_Sum_point, ROOT, MPI_COMM_WORLD);
		MPI_Reduce(counts, globalCounts, k, MPI_INT, MPI_SUM, ROOT, MPI_COMM_WORLD);

		if(id == ROOT){
			/* compute and broadcast the new centroid */
			done = TRUE;
			for(i = 0; i < k; i++){
				tempX = globalC[i].x / globalCounts[i];
				tempY = globalC[i].y / globalCounts[i];
				if(centroids[i].x != tempX || centroids[i].y != tempY){
					done = FALSE; /* quit the loop until no change */
					centroids[i].x = tempX;
					centroids[i].y = tempY;
				}
			}
			++loops;
		} else {
			done = FALSE;
		}

		MPI_Bcast(centroids, k, MPI_POINT, ROOT, MPI_COMM_WORLD);
		MPI_Bcast(&done, 1, MPI_INT, ROOT, MPI_COMM_WORLD);

	} while(!done);

	if(id == ROOT){
		labels = partialLabels;
		/* gather labels in root process */
		for(i = 1; i < p; i++){
			MPI_Recv(labels + BLOCK_LOW(i, p, size), chunkSize, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
			printf("Recieved %d labels from %d.\n", chunkSize, i);
		}

		writeToFile(labels, size, centroids, k);
	} else {
		printf("Process %d sending %d labels to root.\n", id, chunkSize);
		MPI_Send(partialLabels, chunkSize, MPI_INT, ROOT, 0, MPI_COMM_WORLD);
	}

	/*  Clean up */
	free(inputFileName);
	free(data);
	free(centroids);
	free(counts);
	free(globalCounts);
	free(tempC);
	free(globalC);

	MPI_Barrier(MPI_COMM_WORLD);
	elapsed += MPI_Wtime();
	if(id == ROOT){
		printf("%d points assigned to %d clusters in %.2f s.\n", size, k, elapsed);
	}

	MPI_Finalize();
	return 0;
}
