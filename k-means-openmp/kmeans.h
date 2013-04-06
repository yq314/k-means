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
#include <time.h>

typedef struct{
	float x;
	float y;
} Point;

#define TRUE 1
#define FALSE 0

void help();

void getCmdOptions(int argc, char **argv, char **inputFileName, int *k, int *r, char ** centFileName, int *p);

Point *readData(char *fileName, int *count);

Point *readCentroids(char *fileName, int count);

int *kmeans(Point *data, int n, int k, Point *centroids, int p);

Point *initialCentroids(Point *data, int size, int k, int r);

void writeToFile(int *labels, int n, Point *centroids, int k);

#endif /* KMEANS_H_ */
