#ifndef __VECTOR_H
#define __VECTOR_H

#include <stdbool.h>

typedef struct vector *vector;

vector vector_create();
void vector_destroy();
int vector_size();
int vector_size(const vector v);
bool vector_in_bounds(const vector v, int i);
void vector_set(vector v, int i, void *value);
void *vector_get(const vector v, int i);
void vector_insert(vector v, int i, void *value);
void *vector_remove(vector v, int i);
void vector_push(vector v, void *value);
void *vector_pop(vector v);

#endif