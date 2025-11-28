#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <Windows.h>

#define NUM_THREADS 5
#define NUM_REWARD 10
#define NUM_USER 20
#define QUEUE_SIZE 20

int running = 1;

//struct type

typedef struct _Request {
	int user_id;
	int reward_id;
}Request;

typedef struct _User {
	int user_id;
	int reward_id;
}User;

typedef struct _Queue {
	Request contents[QUEUE_SIZE];
	int front;
	int rear;
	int size;
}Queue;

//Shared Variables

int rewards[NUM_REWARD];
User user_list[NUM_USER];
static Queue* request_queue;

pthread_cond_t cv_enqueue = PTHREAD_COND_INITIALIZER;
pthread_cond_t cv_dequeue = PTHREAD_COND_INITIALIZER;
sem_t sem_reward;
pthread_mutex_t mutex_queue = PTHREAD_MUTEX_INITIALIZER;

Queue* create_queue() {
	Queue* q = (Queue*)malloc(sizeof(Queue));
	q->front = 0;
	q->rear = 0;
	q->size = 0;
	return q;
}

void enqueue(Queue* q, Request r) {
	if ((q->front + 1) % QUEUE_SIZE == q->rear) abort();
	q->contents[q->front] = r;
	q->front = (q->front + 1) % QUEUE_SIZE;
	q->size++;
	printf("r:%d f:%d s:%d", q->rear, q->front, q->size);
}

Request dequeue(Queue* q) {
	if (q->front == q->rear) abort();
	Request ret = q->contents[q->rear];
	q->rear = (q->rear + 1) % QUEUE_SIZE;
	q->size--;
	printf("r:%d f:%d s:%d", q->rear, q->front, q->size);
	return ret;
}

int is_empty(Queue* q){
	return q->size == 0;
}

int is_full(Queue* q){
	return q->size == QUEUE_SIZE-1;
}

void* requester_thread(void* thread_id) {
	int tid = (int)thread_id;
	while (running) {
		Request r = { rand() % NUM_USER, rand() % NUM_REWARD };

		pthread_mutex_lock(&mutex_queue);
		printf("[Requester : %d] Lock acquired\n", tid);
		while (is_full(request_queue)) {
			printf("[Requester : %d] Queue is full. Wait for dequeue\n", tid);
			pthread_cond_wait(&cv_dequeue, &mutex_queue);
		}
		enqueue(request_queue, r);
		printf("[Requester : %d] Request queued\n", tid);
		pthread_cond_signal(&cv_enqueue);
		printf("[Requester : %d] Lock released\n", tid);
		pthread_mutex_unlock(&mutex_queue);

	}
	pthread_exit(NULL);
	return 0;
}

void* worker_thread(void* thread_id) {
	int tid = (int)thread_id;
	while (running) {
		pthread_mutex_lock(&mutex_queue);
		printf("[Worker : %d] Lock acquired\n", tid);
		while (is_empty(request_queue)) {
			printf("[Worker : %d] Queue is empty. Wait for enqueue\n", tid);
			pthread_cond_wait(&cv_enqueue, &mutex_queue);
		}
		Request r = dequeue(request_queue);
		pthread_cond_signal(&cv_dequeue);

		printf("[Worker : %d] Lock released\n", tid);
		pthread_mutex_unlock(&mutex_queue);

		sem_wait(&sem_reward);
		rewards[r.reward_id]--;
		printf("[Worker : %d] Request resolved\n", tid);
		sem_post(&sem_reward);
	}
	pthread_exit(NULL);
	return 0;
}

int main() {
	srand(time(NULL));
	sem_init(&sem_reward, 0, 1);

	request_queue = create_queue();

	for (int i = 0; i < NUM_REWARD; i++) {
		rewards[i] = 1 + i;
	}
	for (int i = 0; i < NUM_USER; i++) {
		user_list[i] = { i + 1, 0 };
	}

	pthread_t requester1;
	pthread_create(&requester1, NULL, requester_thread, (void*)1);
	pthread_t requester2;
	pthread_create(&requester2, NULL, requester_thread, (void*)2);

	pthread_t worker_threads[NUM_THREADS];
	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_create(&worker_threads[i], NULL, worker_thread, (void*)(i + 1));
	}
	
	Sleep(1000);
	running = false;
	for (int i = 0; i < NUM_REWARD; i++) {
		rewards[i] = 1 + i;
	}
	for (int i = 0; i < NUM_USER; i++) {
		user_list[i] = { i + 1, 0 };
	}
	pthread_exit(NULL);
}