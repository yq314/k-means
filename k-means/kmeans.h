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
#include <malloc/malloc.h>
#include <string.h>
#include <unistd.h>
#include <float.h>
#include <math.h>
#include <time.h>

typedef struct{
	float x;
	float y;
} Point;

void help();

void getCmdOptions(int argc, char **argv, char **input_file_name, int *k);

Point *readData(char *fileName, int *count);

int *kmeans(Point *data, int n, int k, Point *centroids);

void writeToFile(int *labels, int n, Point *centroids, int k);

#endif /* KMEANS_H_ */
