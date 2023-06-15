#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>
// delete assert
#include <assert.h>
#include "queue.h"

// my implementation uses linked list to represent the Queue
// and linked list to represent the cv corrisponding to each thread
// structs

typedef struct queueNode
{
    void *element;
    struct queueNode *next;
} queueNode;

typedef struct cvNode
{
    cnd_t cv;
    struct cvNode *next;
} cvNode;

typedef struct Queue
{
    queueNode *head;
    queueNode *tail;
    atomic_int size;
    atomic_int visited;

} Queue;

typedef struct conditionVariablesQueue
{
    cvNode *head;
    cvNode *tail;
    atomic_int waiting;
} conditionVariablesQueue;

// variables

mtx_t lock;
// cnd_t notEmpty;
static Queue *queue;
static conditionVariablesQueue *cvQueue;

// TODO
void initQueue(void)
{
    queue = (Queue *)malloc(sizeof(Queue));
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    queue->visited = 0;

    cvQueue = (conditionVariablesQueue *)malloc(sizeof(conditionVariablesQueue));
    cvQueue->head = NULL;
    cvQueue->tail = NULL;
    cvQueue->waiting = 0;

    mtx_init(&lock, mtx_plain);
    return;
}

// TODO
void destroyQueue(void)
{
    mtx_destroy(&lock);
    // cnd_destroy- check but its not suppose to happen
    if (cvQueue)
    {
        cvNode *curr = cvQueue->head;
        while (curr)
        {
            cvNode *tmp = curr->next;
            cnd_destroy(&curr->cv);
            curr->next = NULL;
            free(curr);
            curr = tmp;
        }
        free(cvQueue);
    }
    cvQueue = NULL;

    if (queue)
    {
        queueNode *curr = queue->head;
        while (curr)
        {
            queueNode *tmp = curr->next;
            curr->element = NULL;
            curr->next = NULL;
            free(curr);
            curr = tmp;
        }
    }
    queue = NULL;
    return;
}

void enqueue(void *element)
{
    mtx_lock(&lock);
    queueNode *newElem = (queueNode *)malloc(sizeof(queueNode));
    newElem->element = element;
    newElem->next = NULL;
    // queue is empty
    if (!queue->head)
    {
        queue->head = newElem;
        queue->tail = newElem;
    }
    // queue is not empty
    else
    {
        queue->tail->next = newElem;
        queue->tail = newElem;
    }
    queue->size++;
    if (cvQueue->waiting != 0)
    {
        cnd_signal(&cvQueue->head->cv);
    }

    mtx_unlock(&lock);
    return;
}

void *dequeue(void)
{
    mtx_lock(&lock);
    cnd_t newcv;
    cnd_init(&newcv);
    cvNode *newCvNode = (cvNode *)malloc(sizeof(cvNode));
    newCvNode->cv = newcv;
    newCvNode->next = NULL;
    cvQueue->tail->next = newCvNode;
    cvQueue->tail = cvQueue->tail->next;
    cvQueue->waiting++;
    while (queue->size == 0 || cvQueue->head != newCvNode)
    {
        cnd_wait(&cvQueue->head->cv, &lock);
    }
    // queue is not empty, latest thread will arise
    // delete head from queue
    queueNode *tmp = queue->head;
    void *res = tmp->element;
    queue->head = queue->head->next;
    tmp->element = NULL;
    tmp->next = NULL;
    free(tmp);

    queue->size--;
    queue->visited++;

    // delete cv from cvQueue
    cvNode *tmpCv = cvQueue->head;
    cvQueue->head = cvQueue->head->next;
    cnd_destroy(&tmpCv->cv);
    tmpCv->next = NULL;
    free(tmpCv);

    cvQueue->waiting--;

    mtx_unlock(&lock);
    return res;
}

size_t size(void)
{
    return queue->size;
}

size_t waiting(void)
{
    return cvQueue->waiting;
}

size_t visited(void)
{
    return queue->visited;
}

int main(int argc, char *argv[])
{
    initQueue();
    int items[] = {1, 2, 3, 4, 5};
    size_t num_items = sizeof(items) / sizeof(items[0]);

    // Enqueue items
    for (size_t i = 0; i < num_items; i++)
    {
        enqueue(&items[i]);
    }

    // Dequeue items and check the order
    for (size_t i = 0; i < num_items; i++)
    {
        int *item = (int *)dequeue();
        printf("Dequeued: %d\n", *item);
        if (*item != items[i])
        {
            printf("ERROR - %ls", item);
        }
    }

    if (size() != 0)
    {
        printf("ERROR in size");
    }

    destroyQueue();

    destroyQueue();

    return 1;
}