/*
 * kmeans.c
 *
 *  Created on: Mar 22, 2013
 *      Author: qingye
 */


#include "kmeans.h"
#include <omp.h>

/*
 * Print the usage of this programme
 */
void help(){
	printf("Usage: \n");
	printf("<-i inputFileName>	:	input data file path and name\n");
	printf("[-k k-means]		:	the number of k, should be larger than 0, default 9\n");
	printf("[-r]			:	whether create centroids randomly\n");
	printf("[-c centroidFileName]	:	the starting centroids file\n");
	printf("[-p numOfThreads]       :       number of threads to spawn\n");
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
 * @param r				int*	whether create centroids randomly
 * @param centFileName		char**	the starting centroids file
 *
 * @return void
 */
void getCmdOptions(int argc, char **argv, char **inputFileName, int *k, int *r, char **centFileName, int *p){
	int c;
	opterr = 0;

	while((c = getopt(argc, argv, "i:k:c:p:hr")) != -1){
		switch(c){
			case 'i':
				*inputFileName = (char *)malloc(strlen(optarg) * sizeof(optarg));
				strcpy(*inputFileName, optarg);
				break;
			case 'k':
				*k = atoi(optarg);
				break;
			case 'r':
				*r = TRUE;
				break;
			case 'c':
				*centFileName = (char *)malloc(strlen(optarg) * sizeof(optarg));
				strcpy(*centFileName, optarg);
				break;
			case 'h':
				help();
				exit(0);
				break;
		        case 'p':
		                *p = atoi(optarg);
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
 * @param fileName	char*	the file path and name to be read
 * @param count		int*	number of file lines
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
 * Reads the centroids from input file
 *
 * This function will change the value of count
 *
 * @param fileName	char*	the file path and name to be read
 * @param count		int	number of file lines
 *
 * @return data		Point*	array storing the points
 *
 */
Point *readCentroids(char *fileName, int count){
	FILE *pRead;
	Point *data = (Point *) calloc(count, sizeof(Point));
	int i;

	if((pRead = fopen(fileName, "r")) == NULL){
		printf("Fail to open file: %s", fileName);
		exit(-1);
	}

	for(i = 0; i < count; i++){
		fscanf(pRead, "%f %f\n", &data[i].x, &data[i].y);
	}
	fclose(pRead);

	return data;
}

/*
 * k-means algorithm inplementation
 *
 * This function will change the value of centroids
 *
 * @param data		Point*		the input data array
 * @param size		int			the size of input data
 * @param k			int			k-means
 * @param centroids	Point*		array storing the k centroids
 *
 * @return labels	int*	an array storing the label of each point
 *
 */
int *kmeans(Point *data, int size, int k, Point *centroids, int p){
	int *labels = (int *) calloc(size, sizeof(int));
	int i, j, done, loops;
	float minDist, dist;
	float tempX, tempY;
	Point *tempC = (Point *) calloc(k, sizeof(Point)); /*temporary centroids*/
	int *counts = (int *) calloc(k, sizeof(int));	/*counts of each cluster*/

	printf("=====initial centroids=====\n");
	for(i = 0; i < k; i++){
		printf("%f %f\n", centroids[i].x, centroids[i].y);
	}
	printf("===========================\n");

	/* loop to determine the clusters */
	done = TRUE;
	loops = 0;

	do{

#pragma omp parallel default(shared) num_threads(p)
	        {
#pragma omp for schedule(static, 1)
		  /* initialize the helper arrays */
		  for(i = 0; i < k; i++){
		    counts[i] = 0;
		    tempC[i].x = 0;
		    tempC[i].y = 0;
		  }

#pragma omp for private(j, dist, minDist) schedule(static, 1) 
		  for(i = 0; i < size; i++){

		    minDist = FLT_MAX;

		    /* compute the distance between the point and each centroid*/
		    for(j = 0; j < k; j++){

		      /* no need to compute the sqrt, we just need the value for comparison */
		      dist = pow(data[i].x - centroids[j].x, 2) +
			pow(data[i].y - centroids[j].y, 2);

		      /* assign shortest distance to the j-th cluster*/
		      if(dist < minDist){
			minDist = dist;
			labels[i] = j;
		      }

		    }

		  }

		}
		
		/* unable to reduce arrays here */
		/* possible to create local-arrays in thread and merge but performance improvement unlikely */
		for (i = 0; i < size; i++) {
		  /* count the number of points in the j-th cluster */
		  ++counts[labels[i]];
		  /*printf("Total counts in cluster %d: %d\n", labels[i], counts[labels[i]]);*/

		  /*
		   * simply add on the x and y of each point,
		   * for further computation of new centroid
		   */
		  tempC[labels[i]].x += data[i].x;
		  tempC[labels[i]].y += data[i].y;
		
		}

		
		/* update the centroids */
		done = TRUE;
		for(i = 0; i < k; i++){
			tempX = counts[i] ? tempC[i].x / counts[i] : 0;
			tempY = counts[i] ? tempC[i].y / counts[i] : 0;
			if(centroids[i].x != tempX || centroids[i].y != tempY){
				done = FALSE; /* quit the loop until no change */
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
 * Initialize the centroids, randomly select initial points
 *
 * @param data	Point*	array of input points
 * @param size	int		number of points
 * @param k		int		number of clusters
 * @param r		int		whether create randomly
 *
 * @return Point* array of centroids
 *
 */
Point *initialCentroids(Point *data, int size, int k, int r){
	Point *c = (Point *) calloc(k, sizeof(Point));
	int i,j;
	char *fileName = "initial.txt";
	FILE *pWrite;

	if(k > size){
		k = size;
	}

	srand((unsigned) time(NULL));
	for(i = j = 0; i < k; i++){
		if(r){
			c[i].x = (float) (rand() % 1000) / 1000 * 8;
			c[i].y = (float) (rand() % 1000) / 1000 * 8;
 		}else{
 			/*
			 * pick the first point from k chunks,
			 * it's not real random, but acceptable
			 */
			j += size/k;
			c[i].x = data[j].x;
			c[i].y = data[j].y;
 		}
	}

	/* write to file */
	if((pWrite = fopen(fileName, "w")) == NULL){
		printf("Fail to open output file: %s\n", fileName);
		exit(-1);
	}

	for(i = 0; i < k; i++){
		fprintf(pWrite, "%f %f\n", c[i].x, c[i].y);
	}

	printf("Successfully wrote initial centroids into file: %s\n", fileName);

	fclose(pWrite);

	return c;
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
 * Main function
 */
int main(int argc, char **argv){

	char *inputFileName = NULL;
	char *centFileName = NULL;
	int size;	/* line count of input data*/
	Point *data;	/* input data points*/
	Point *centroids;
	int *labels;
	int k = 0;
	int r = FALSE;
	int p = 0;
	double start, end;
	start = omp_get_wtime();
	
	getCmdOptions(argc, argv, &inputFileName, &k, &r, &centFileName, &p);

	data = readData(inputFileName, &size);

	if(centFileName != NULL){
		centroids = readCentroids(centFileName, k);
	}else{
		centroids = initialCentroids(data, size, k, r);
	}

	labels = kmeans(data, size, k, centroids, p);

	writeToFile(labels, size, centroids, k);

	/*  Clean up */
	free(inputFileName);
	free(centFileName);

	if(data){
		free(data);
	}

	if(centroids){
		free(centroids);
	}

	end = omp_get_wtime();
	
	printf("%d points assigned to %d clusters in %.2f s.\n", size, k, (double)(end - start));

	return 0;
}
