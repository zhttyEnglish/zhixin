#include <pthread.h>

// 队列的最大大小
#define QUEUE_SIZE 8
#define PATH_LEN 64

// 队列元素结构体
typedef struct {
    char image_path[PATH_LEN]; 
	char result_path[PATH_LEN];
    int in_use;              // 标志，表示该内存块是否已被占用
} QueueElement;

// 队列结构体
typedef struct {
    QueueElement elements[QUEUE_SIZE];  // 队列元素
    int head;                           // 队列头
    int tail;                           // 队列尾
    pthread_mutex_t mutex;              // 用于线程同步
    pthread_cond_t cond;                // 用于条件变量
} ImageQueue;

void queue_init(ImageQueue *queue);
void queue_allocate(ImageQueue *queue, char * image_path, char * result_path);
void queue_release(ImageQueue *queue, char * image_path, char * result_path);
void queue_destroy(ImageQueue *queue);


