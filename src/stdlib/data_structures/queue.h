#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/* Queue data structure for SIMSCRIPT */
typedef struct Queue {
    void** elements;
    int front;
    int rear;
    int count;
    int capacity;
    void (*free_element)(void*);  /* Element cleanup function */
} Queue;

/* Create a new queue */
Queue* queue_create(int capacity, void (*free_element)(void*));

/* Destroy a queue and free all elements */
void queue_destroy(Queue* queue);

/* Add an element to the back of the queue */
bool queue_enqueue(Queue* queue, void* element);

/* Remove and return the front element from the queue */
void* queue_dequeue(Queue* queue);

/* Return the front element without removing it */
void* queue_peek(Queue* queue);

/* Check if queue is empty */
bool queue_is_empty(Queue* queue);

/* Check if queue is full */
bool queue_is_full(Queue* queue);

/* Get the number of elements in the queue */
int queue_size(Queue* queue);

/* Clear all elements from the queue */
void queue_clear(Queue* queue);

/* Get element at specific index (for iteration) */
void* queue_get_at(Queue* queue, int index);

#ifdef __cplusplus
}
#endif