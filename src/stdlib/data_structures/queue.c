#include "queue.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define DEFAULT_CAPACITY 16
#define GROWTH_FACTOR 2

/* Internal function to resize the queue */
static void resize_queue(Queue* queue, int new_capacity) {
    void** new_elements = (void**)malloc(new_capacity * sizeof(void*));
    if (new_elements == NULL) {
        /* Handle allocation failure */
        return;
    }

    /* Copy elements to new array */
    if (queue->front < queue->rear) {
        /* Elements are contiguous */
        memcpy(new_elements, &queue->elements[queue->front],
               queue->count * sizeof(void*));
    } else {
        /* Elements wrap around */
        int first_part = queue->capacity - queue->front;
        memcpy(new_elements, &queue->elements[queue->front],
               first_part * sizeof(void*));
        memcpy(&new_elements[first_part], queue->elements,
               queue->rear * sizeof(void*));
    }

    free(queue->elements);
    queue->elements = new_elements;
    queue->capacity = new_capacity;
    queue->front = 0;
    queue->rear = queue->count;
}

Queue* queue_create(int capacity, void (*free_element)(void*)) {
    if (capacity <= 0) {
        capacity = DEFAULT_CAPACITY;
    }

    Queue* queue = (Queue*)malloc(sizeof(Queue));
    if (queue == NULL) {
        return NULL;
    }

    queue->elements = (void**)malloc(capacity * sizeof(void*));
    if (queue->elements == NULL) {
        free(queue);
        return NULL;
    }

    queue->front = 0;
    queue->rear = 0;
    queue->count = 0;
    queue->capacity = capacity;
    queue->free_element = free_element;

    return queue;
}

void queue_destroy(Queue* queue) {
    if (queue == NULL) {
        return;
    }

    queue_clear(queue);
    free(queue->elements);
    free(queue);
}

bool queue_enqueue(Queue* queue, void* element) {
    if (queue == NULL || element == NULL) {
        return false;
    }

    if (queue->count >= queue->capacity) {
        resize_queue(queue, queue->capacity * GROWTH_FACTOR);
    }

    queue->elements[queue->rear] = element;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->count++;

    return true;
}

void* queue_dequeue(Queue* queue) {
    if (queue == NULL || queue->count == 0) {
        return NULL;
    }

    void* element = queue->elements[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->count--;

    return element;
}

void* queue_peek(Queue* queue) {
    if (queue == NULL || queue->count == 0) {
        return NULL;
    }

    return queue->elements[queue->front];
}

bool queue_is_empty(Queue* queue) {
    return queue == NULL || queue->count == 0;
}

bool queue_is_full(Queue* queue) {
    return queue != NULL && queue->count >= queue->capacity;
}

int queue_size(Queue* queue) {
    return (queue != NULL) ? queue->count : 0;
}

void queue_clear(Queue* queue) {
    if (queue == NULL) {
        return;
    }

    if (queue->free_element != NULL) {
        for (int i = 0; i < queue->count; i++) {
            int index = (queue->front + i) % queue->capacity;
            queue->free_element(queue->elements[index]);
        }
    }

    queue->front = 0;
    queue->rear = 0;
    queue->count = 0;
}

void* queue_get_at(Queue* queue, int index) {
    if (queue == NULL || index < 0 || index >= queue->count) {
        return NULL;
    }

    int actual_index = (queue->front + index) % queue->capacity;
    return queue->elements[actual_index];
}