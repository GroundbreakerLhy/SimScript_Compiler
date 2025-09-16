#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/* Set data structure for SIMSCRIPT */
typedef struct Set {
    void** elements;
    int count;
    int capacity;
    int (*compare)(const void*, const void*);  /* Comparison function */
    void (*free_element)(void*);               /* Element cleanup function */
} Set;

/* Create a new set */
Set* set_create(int (*compare)(const void*, const void*), void (*free_element)(void*));

/* Destroy a set and free all elements */
void set_destroy(Set* set);

/* Add an element to the set (returns true if added, false if already exists) */
bool set_add(Set* set, void* element);

/* Remove an element from the set (returns true if removed) */
bool set_remove(Set* set, void* element);

/* Check if element exists in set */
bool set_contains(Set* set, void* element);

/* Get the number of elements in the set */
int set_size(Set* set);

/* Check if set is empty */
bool set_is_empty(Set* set);

/* Clear all elements from the set */
void set_clear(Set* set);

/* Get element at index (for iteration) */
void* set_get_at(Set* set, int index);

/* Union of two sets (creates new set) */
Set* set_union(Set* set1, Set* set2);

/* Intersection of two sets (creates new set) */
Set* set_intersection(Set* set1, Set* set2);

/* Difference of two sets (creates new set) */
Set* set_difference(Set* set1, Set* set2);

/* Check if set1 is subset of set2 */
bool set_is_subset(Set* set1, Set* set2);

/* Check if two sets are equal */
bool set_equals(Set* set1, Set* set2);

#ifdef __cplusplus
}
#endif