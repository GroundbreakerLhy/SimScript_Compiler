#include "set.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define INITIAL_CAPACITY 16
#define GROWTH_FACTOR 2

/* Internal function to find element index */
static int find_element(Set* set, void* element) {
    for (int i = 0; i < set->count; i++) {
        if (set->compare(set->elements[i], element) == 0) {
            return i;
        }
    }
    return -1;
}

/* Internal function to resize the set */
static void resize_set(Set* set, int new_capacity) {
    void** new_elements = (void**)realloc(set->elements, new_capacity * sizeof(void*));
    if (new_elements == NULL) {
        /* Handle allocation failure */
        return;
    }
    set->elements = new_elements;
    set->capacity = new_capacity;
}

Set* set_create(int (*compare)(const void*, const void*), void (*free_element)(void*)) {
    if (compare == NULL) {
        return NULL;
    }

    Set* set = (Set*)malloc(sizeof(Set));
    if (set == NULL) {
        return NULL;
    }

    set->elements = (void**)malloc(INITIAL_CAPACITY * sizeof(void*));
    if (set->elements == NULL) {
        free(set);
        return NULL;
    }

    set->count = 0;
    set->capacity = INITIAL_CAPACITY;
    set->compare = compare;
    set->free_element = free_element;

    return set;
}

void set_destroy(Set* set) {
    if (set == NULL) {
        return;
    }

    if (set->free_element != NULL) {
        for (int i = 0; i < set->count; i++) {
            set->free_element(set->elements[i]);
        }
    }

    free(set->elements);
    free(set);
}

bool set_add(Set* set, void* element) {
    if (set == NULL || element == NULL) {
        return false;
    }

    if (find_element(set, element) >= 0) {
        return false;  /* Element already exists */
    }

    if (set->count >= set->capacity) {
        resize_set(set, set->capacity * GROWTH_FACTOR);
    }

    set->elements[set->count++] = element;
    return true;
}

bool set_remove(Set* set, void* element) {
    if (set == NULL || element == NULL) {
        return false;
    }

    int index = find_element(set, element);
    if (index < 0) {
        return false;  /* Element not found */
    }

    if (set->free_element != NULL) {
        set->free_element(set->elements[index]);
    }

    /* Shift elements to fill the gap */
    for (int i = index; i < set->count - 1; i++) {
        set->elements[i] = set->elements[i + 1];
    }

    set->count--;
    return true;
}

bool set_contains(Set* set, void* element) {
    if (set == NULL || element == NULL) {
        return false;
    }
    return find_element(set, element) >= 0;
}

int set_size(Set* set) {
    return (set != NULL) ? set->count : 0;
}

bool set_is_empty(Set* set) {
    return set == NULL || set->count == 0;
}

void set_clear(Set* set) {
    if (set == NULL) {
        return;
    }

    if (set->free_element != NULL) {
        for (int i = 0; i < set->count; i++) {
            set->free_element(set->elements[i]);
        }
    }

    set->count = 0;
}

void* set_get_at(Set* set, int index) {
    if (set == NULL || index < 0 || index >= set->count) {
        return NULL;
    }
    return set->elements[index];
}

Set* set_union(Set* set1, Set* set2) {
    if (set1 == NULL || set2 == NULL) {
        return NULL;
    }

    Set* result = set_create(set1->compare, set1->free_element);
    if (result == NULL) {
        return NULL;
    }

    /* Add all elements from set1 */
    for (int i = 0; i < set1->count; i++) {
        set_add(result, set1->elements[i]);
    }

    /* Add all elements from set2 */
    for (int i = 0; i < set2->count; i++) {
        set_add(result, set2->elements[i]);
    }

    return result;
}

Set* set_intersection(Set* set1, Set* set2) {
    if (set1 == NULL || set2 == NULL) {
        return NULL;
    }

    Set* result = set_create(set1->compare, set1->free_element);
    if (result == NULL) {
        return NULL;
    }

    /* Add elements that exist in both sets */
    for (int i = 0; i < set1->count; i++) {
        if (set_contains(set2, set1->elements[i])) {
            set_add(result, set1->elements[i]);
        }
    }

    return result;
}

Set* set_difference(Set* set1, Set* set2) {
    if (set1 == NULL || set2 == NULL) {
        return NULL;
    }

    Set* result = set_create(set1->compare, set1->free_element);
    if (result == NULL) {
        return NULL;
    }

    /* Add elements from set1 that don't exist in set2 */
    for (int i = 0; i < set1->count; i++) {
        if (!set_contains(set2, set1->elements[i])) {
            set_add(result, set1->elements[i]);
        }
    }

    return result;
}

bool set_is_subset(Set* set1, Set* set2) {
    if (set1 == NULL || set2 == NULL) {
        return false;
    }

    for (int i = 0; i < set1->count; i++) {
        if (!set_contains(set2, set1->elements[i])) {
            return false;
        }
    }
    return true;
}

bool set_equals(Set* set1, Set* set2) {
    if (set1 == NULL || set2 == NULL) {
        return false;
    }

    if (set1->count != set2->count) {
        return false;
    }

    return set_is_subset(set1, set2) && set_is_subset(set2, set1);
}