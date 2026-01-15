#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "queue.h"

// 初始化队列
void queue_init(ImageQueue *queue) 
{
    memset(queue, 0, sizeof(ImageQueue));
    for (int i = 0; i < QUEUE_SIZE; i++) {
        queue->elements[i].image_path = (char *)malloc(PATH_LEN);  // 分配内存
        queue->elements[i].result_path = (char *)malloc(PATH_LEN);  // 分配内存
        if (queue->elements[i].result_path == NULL || queue->elements[i].image_path == NULL) {
            perror("Failed to allocate memory");
            exit(EXIT_FAILURE);
        }
        queue->elements[i].in_use = 0;  // 标记为空闲
        printf("image path %p  result path %p\r\n", queue->elements[i].image_path, queue->elements[i].result_path);
    }
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
}

// 从队列中获取空闲的地址
void queue_allocate(ImageQueue *queue, char * image_path, char * result_path) 
{
    pthread_mutex_lock(&queue->mutex);

    // 等待队列中有空闲的内存块
    while (queue->elements[queue->head].in_use == 1) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    // 获取空闲内存块
    //result_path = queue->elements[queue->head].result_path;
	//image_path = queue->elements[queue->head].image_path;
	memcpy(queue->elements[queue->head].image_path, image_path, 128);
	memcpy(queue->elements[queue->head].result_path, result_path, 128);
    queue->elements[queue->head].in_use = 1;  // 标记为已占用
    queue->head = (queue->head + 1) % QUEUE_SIZE;

    pthread_mutex_unlock(&queue->mutex);
}

// 将占用的内存块释放并标记为空闲
void queue_release(ImageQueue *queue, char * image_path, char * result_path) 
{
    pthread_mutex_lock(&queue->mutex);
    // 查找释放的内存块并标记为空闲
    for (int i = 0; i < QUEUE_SIZE; i++) {
        if (queue->elements[i].image_path == image_path && 
			queue->elements[i].result_path == result_path) {
            queue->elements[i].in_use = 0;  // 标记为未占用
            queue->tail = (queue->tail + 1) % QUEUE_SIZE;
            break;
        }
    }
    // 唤醒等待的线程
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

// 销毁队列并释放内存
void queue_destroy(ImageQueue *queue) 
{
    for (int i = 0; i < QUEUE_SIZE; i++) {
        free(queue->elements[i].result_path);
		free(queue->elements[i].image_path);
    }
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->cond);
}


