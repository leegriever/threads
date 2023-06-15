#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <threads.h>

typedef struct Node {
    void* item;
    struct Node* next;
} Node;

typedef struct {
    Node* head;
    Node* tail;
    size_t size;
    size_t waitingThreads;
    mtx_t lock;
    cnd_t enqueueCond;
    cnd_t dequeueCond;
} Queue;

typedef struct 

Queue queue;

void initQueue(void) {
    queue.head = NULL;
    queue.tail = NULL;
    queue.size = 0;
    queue.waitingThreads = 0;
    mtx_init(&queue.lock, mtx_plain);
    cnd_init(&queue.enqueueCond);
    cnd_init(&queue.dequeueCond);
}

void destroyQueue(void) {
    mtx_destroy(&queue.lock);
    cnd_destroy(&queue.enqueueCond);
    cnd_destroy(&queue.dequeueCond);
}

void enqueue(void* item) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->item = item;
    newNode->next = NULL;

    mtx_lock(&queue.lock);

    if (queue.size == 0) {
        queue.head = newNode;
        queue.tail = newNode;
    } else {
        queue.tail->next = newNode;
        queue.tail = newNode;
    }

    queue.size++;

    if (queue.waitingThreads > 0) {
        cnd_signal(&queue.dequeueCond);
    }

    mtx_unlock(&queue.lock);
}

void* dequeue(void) {
    mtx_lock(&queue.lock);

    while (queue.size == 0) {
        queue.waitingThreads++;
        cnd_wait(&queue.dequeueCond, &queue.lock);
        queue.waitingThreads--;
    }

    Node* node = queue.head;
    void* item = node->item;
    queue.head = node->next;

    if (queue.head == NULL) {
        queue.tail = NULL;
    }

    free(node);
    queue.size--;

    if (queue.waitingThreads > 0) {
        cnd_signal(&queue.enqueueCond);
    }

    mtx_unlock(&queue.lock);

    return item;
}

bool tryDequeue(void** item) {
    mtx_lock(&queue.lock);

    if (queue.size == 0) {
        mtx_unlock(&queue.lock);
        return false;
    }

    Node* node = queue.head;
    *item = node->item;
    queue.head = node->next;

    if (queue.head == NULL) {
        queue.tail = NULL;
    }

    free(node);
    queue.size--;

    if (queue.waitingThreads > 0) {
        cnd_signal(&queue.enqueueCond);
    }

    mtx_unlock(&queue.lock);

    return true;
}

size_t size(void) {
    mtx_lock(&queue.lock);
    size_t currentSize = queue.size;
    mtx_unlock(&queue.lock);
    return currentSize;
}

size_t waiting(void) {
    mtx_lock(&queue.lock);
    size_t currentWaitingThreads = queue.waitingThreads;
    mtx_unlock(&queue.lock);
    return currentWaitingThreads;
}

size_t visited(void) {
    mtx_lock(&queue.lock);
    size_t itemsVisited = queue.size;
    mtx_unlock(&queue.lock);
    return itemsVisited;
}

int main() {
    // Example usage of the queue functions

    initQueue();

   
