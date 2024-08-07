#include "vector.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

struct vector {
  void **elems;
  int capacity;
  int size;
};

static void **get_element(const vector v, int i);
static void extend_if_necessary(vector v);

vector vector_create() 
{

  vector v = malloc(sizeof (vector));
  assert(v != NULL);
  v->elems = malloc(sizeof (void *));
  assert(v->elems != NULL);

  v->capacity = 1;
  v->size = 0;

  return v;
}

void vector_destroy(vector v) 
{
  free(v->elems);
  free(v);
}

int vector_size(const vector v) 
{
  return v->size;
}

bool vector_in_bounds(const vector v, int i) 
{
  return i < (size_t) v->size;
}

void vector_set(vector v, int i, void *value) 
{
  *get_element(v, i) = value;
}

void *vector_get(const vector v, int i)
{
  return *get_element(v, i);
}

void vector_insert(vector v,int i, void *value)
{
  v->size += 1;
  extend_if_necessary(v);

  void **target = get_element(v, i);

  int remaining = v->size - i - 1;
  memmove(target + 1, target, remaining * sizeof (void *));

  *target = value;
}

void *vector_remove(vector v, int i)
{
  void **target = get_element(v, i);
  void *result = *target;

  int remaining = v->size - i - 1;
  memmove(target, target + 1, remaining * sizeof (void *));
  v->size -= 1;

  return result;
}

void vector_push(vector v, void *value)
{
  vector_insert(v, v->size, value);
}

void *vector_pop(vector v)
{
  return vector_remove(v, v->size - 1);
}

static void **get_element(const vector v, int i)
{
  assert(i < (size_t) v->size);
  return &v->elems[i];
}

static void extend_if_necessary(vector v)
{
  if (v->size > v->capacity) {

    v->capacity *= 2;
    v->elems = realloc(v->elems, v->capacity * sizeof (void *));
  }
}