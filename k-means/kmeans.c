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

	if(strlen(*inputFileName) == 0){
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
 * @return data		struct point*	array storing the points
 *
 */
struct point *readData(char *fileName, int *count){
	FILE *pRead;
	struct point *data = (struct point *) malloc(sizeof(struct point));
	float nX, nY;

	if((pRead = fopen(fileName, "r")) == NULL){
		printf("Fail to open file: %s", fileName);
		exit(-1);
	}

	*count = 0;
	while(fscanf(pRead, "%f %f\n", &nX, &nY) != EOF){
		data = (struct point *) realloc(data, (*count+1) * sizeof(struct point));
		data[*count].x = nX;
		data[*count].y = nY;
		++ *count;
	}
	fclose(pRead);

	return data;
}

/*
 * k-means algorithm inplementation
 *
 * This function will change the value of centroids
 *
 * @param data		struct point*		the input data array
 * @param size		int					the size of input data
 * @param k			int					k-means
 * @param centroids	struct point*		array storing the k centroids
 *
 * @return labels	int*	an array storing the label of each point
 *
 */
int *kmeans(struct point *data, int size, int k, struct point *centroids){
	int *labels = (int *) calloc(size, sizeof(int));
	int i, j, done, loops;
	float min_dist, dist;
	float tempX, tempY;
	if(!centroids){
		centroids = (struct point *) calloc(k, sizeof(struct point));
	}
	struct point *tempC = (struct point *) calloc(k, sizeof(struct point)); /*temporary centroids*/
	int *counts = (int *) calloc(k, sizeof(int));	/*counts of each cluster*/

	/* initialization: randomly set k centroids */
	if(k > size){
		k = size;
	}
	for(i = j = 0; i < k; i++){
		/*
		 * pick the first point from k chunks,
		 * it's not real random, but acceptable
		 */
		j += size/k;
		centroids[i].x = data[j].x;
		centroids[i].y = data[j].y;
	}

	/* loop to determine the clusters */
	done = 1;
	loops = 0;
	do{
		/* initialize the helper arrays */
		for(i = 0; i < k; i++){
			counts[i] = 0;
			tempC[i].x = 0;
			tempC[i].y = 0;
		}

		for(i = 0; i < size; i++){
			min_dist = FLT_MAX;
			/* compute the distance between the point and each centroid*/
			for(j = 0; j < k; j++){
				/* no need to compute the sqrt, we just need the value for comparison */
				dist = pow(data[i].x - centroids[j].x, 2) +
						pow(data[i].y - centroids[j].y, 2);
				if(dist < min_dist){
					min_dist = dist;
					labels[i] = j;
				}
			}

			++counts[labels[i]];

			/*
			 * simply add on the x and y of each point,
			 * for further computation of new centroid
			 */
			tempC[labels[i]].x += data[i].x;
			tempC[labels[i]].y += data[i].y;
		}

		/* update the centroids */
		done = 1;
		for(i = 0; i < k; i++){
			tempX = tempC[i].x/counts[i];
			tempY = tempC[i].y/counts[i];
			if(centroids[i].x != tempX || centroids[i].y != tempY){
				done = 0; /* quit the loop until no change */
				centroids[i].x = tempX;
				centroids[i].y = tempY;
			}
		}

		++loops;
	}while(!done);

	printf("Iterated %d loops.\n", loops);

	/*  Clean up */
	free(tempC);
	free(counts);

	return labels;
}

/*
 * writes the labels and centroids into corresponding files
 *
 * @param labels	int*	The array storing cluster labels for each point
 * @param size		int		The size of data
 * @param centroids	struct point*	The array storing k centroids
 * @param k			int		k-means
 *
 * @return void
 */
void writeToFile(int *labels, int size, struct point *centroids, int k){
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
 * Main function
 */
int main(int argc, char **argv){

	char *inputFileName;
	int size;	/* line count of input data*/
	struct point *data;	/* input data points*/
	struct point *centroids;
	int *labels;
	int k = 0;
	time_t start, end;
	start = clock();

	getCmdOptions(argc, argv, &inputFileName, &k);

	data = readData(inputFileName, &size);

	centroids = (struct point *) calloc(k, sizeof(struct point));
	labels = kmeans(data, size, k, centroids);

	writeToFile(labels, size, centroids, k);

	/*  Clean up */
	free(inputFileName);

	if(data){
		free(data);
	}

	if(centroids){
		free(centroids);
	}

	end = clock();
	printf("%d points assigned to %d clusters in %.2f s.\n", size, k, (double)(end - start)/CLOCKS_PER_SEC);

}
