/*
 * kmeans.h
 *
 *  Created on: Mar 20, 2013
 *      Author: qingye
 */

#ifndef KMEANS_H_
#define KMEANS_H_

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <float.h>
#include <math.h>
#include <mpi.h>

typedef struct{
	float x;
	float y;
} Point;

#define TRUE 1
#define FALSE 0
#define ROOT 0
#define BLOCK_LOW(id, p, n) ((id)*(n)/(p))
#define BLOCK_HIGH(id, p, n) (BLOCK_LOW((id)+1, p, n) - 1)
#define BLOCK_SIZE(id, p, n) (BLOCK_LOW((id)+1, p, n) - BLOCK_LOW(id, p, n))

void help();

void getCmdOptions(int argc, char **argv, char **inputFileName, int *k, int *r, char ** centFileName);

Point *readData(char *fileName, int *count);

Point *readCentroids(char *fileName, int count);

Point *initialCentroids(Point *data, int size, int k, int r);

void writeToFile(int *labels, int n, Point *centroids, int k);

void sumPoint(void *in, void *inout, int *len, MPI_Datatype *dptr);

#endif /* KMEANS_H_ */
