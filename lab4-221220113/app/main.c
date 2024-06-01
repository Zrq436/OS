#include "lib.h"
#include "types.h"

int buffer[8];
int buffer_tail;
int buffer_head;
int buffer_size;
sem_t mutex;
sem_t full_buffers;
sem_t empty_buffers;

void producer(int pid, int index) {
	// produce special product
	int i = 1;
	int offset = 1;
	for (int i = 1; i < index; i++){
		offset = offset * 10;
	}
	// producer logic
    while(1){
		//printf("pid %d: producer %d want to produce product\n", pid, index);
		//printf("pid %d: producer %d wait on mutex\n", pid, index);
		sem_wait(&mutex);
		//printf("pid %d: producer %d lock mutex successfully\n", pid, index);
		while (buffer_size == 8){
			//printf("pid %d: producer %d find buffer is full\n", pid, index);
			//printf("pid %d: producer %d unlock mutex\n", pid, index);
			sem_post(&mutex);
			//printf("pid %d: producer %d wait on empty_buffers\n", pid, index);
			sem_wait(&empty_buffers);
			//printf("pid %d: producer %d lock empty_buffers successfully\n", pid, index);
			//printf("pid %d: producer %d wait on mutex\n", pid, index);
			sem_wait(&mutex);
			//printf("pid %d: producer %d relock mutex successfully\n", pid, index);
		}
		//printf("pid %d: producer %d find there is an empty buffer\n", pid, index);
		buffer[buffer_tail] = i * offset;
		printf("pid %d: producer %d produce product successfully, product is %d in buffer[%d]\n", pid, index, buffer[buffer_tail], buffer_tail);
		buffer_tail++;
		buffer_tail = buffer_tail % 8;
		i++;
		buffer_size++;
		//printf("pid %d: producer %d unlock mutex, post full_buffers and sleep 128\n", pid, index);
		sem_post(&mutex);
		sem_post(&full_buffers);
		sleep(128);
	}
}

void consumer(int pid) {
	while(1){
		//printf("pid %d: consumer want to comsume product\n", pid);
		//printf("pid %d: consumer wait on mutex\n", pid);
		sem_wait(&mutex);
		//printf("pid %d: consumer lock mutex successfully\n", pid);
		while(buffer_size == 0){
			//printf("pid %d: consumer find buffer is empty\n", pid);
			//printf("pid %d: consumer unlock mutex\n", pid);
			sem_post(&mutex);
			//printf("pid %d: consumer wait on full_buffers\n", pid);
			sem_wait(&full_buffers);
			//printf("pid %d: consumer lock empty_buffers successfully\n", pid);
			//printf("pid %d: consumer wait on mutex\n", pid);
			sem_wait(&mutex);
			//printf("pid %d: consumer relock mutex successfully\n", pid);
		}
		printf("pid %d: cosumer find there is a product in buffer\n", pid);
		printf("pid %d: cosumer consume product successfully, product is %d in buffer[%d]\n", pid, buffer[buffer_head], buffer_head);
		buffer_head++;
		buffer_head = buffer_head % 8;
		buffer_size--;
		//printf("pid %d: consumer unlock mutex, post empty_buffers and sleep 128\n", pid);
		sem_post(&mutex);
		sem_post(&empty_buffers);
		sleep(128);
	}
}

int uEntry(void)
{
	// For lab4.1
	// Test 'scanf'
	int dec = 0;
	int hex = 0;
	char str[6];
	char cha = 0;
	int ret = 0;
	while (1)
	{
		printf("Input:\" Test %%c Test %%6s %%d %%x\"\n");
		ret = scanf(" Test %c Test %6s %d %x", &cha, str, &dec, &hex);
		printf("Ret: %d; %c, %s, %d, %x.\n", ret, cha, str, dec, hex);
		if (ret == 4)
			break;
	}

	// For lab4.2
	// Test 'Semaphore'
	int i = 4;

	sem_t sem;
	printf("Father Process: Semaphore Initializing.\n");
	ret = sem_init(&sem, 2);
	if (ret == -1)
	{
		printf("Father Process: Semaphore Initializing Failed.\n");
		exit();
	}

	ret = fork();
	if (ret == 0)
	{
		while (i != 0)
		{
			i--;
			printf("Child Process: Semaphore Waiting.\n");
			sem_wait(&sem);
			printf("Child Process: In Critical Area.\n");
		}
		printf("Child Process: Semaphore Destroying.\n");
		sem_destroy(&sem);
		exit();
	}
	else if (ret != -1)
	{
		while (i != 0)
		{
			i--;
			printf("Father Process: Sleeping.\n");
			sleep(128);
			printf("Father Process: Semaphore Posting.\n");
			sem_post(&sem);
		}
		printf("Father Process: Semaphore Destroying.\n");
		sem_destroy(&sem);
		//exit();
	}

	// For lab4.3
	// TODO: You need to design and test the problem.
	// Note that you can create your own functions.
	// Requirements are demonstrated in the guide.
	buffer_tail = 0;
	buffer_head = 0;
	buffer_size = 0;
	sem_init(&mutex, 1);
    sem_init(&full_buffers, 0);
	sem_init(&empty_buffers, 8);
	for (int i = 0; i < 8; i++){
		buffer[i] = -1;
	}
	for (i = 0; i < 4; i++) {
        if (fork() == 0)
            break;
    }

	int pid = get_pid();
    if (i == 4) {
        printf("consumer process init over\n");
		consumer(pid);
		exit();
    }
	else{
		printf("producer process %d init over\n", i + 1);
		producer(pid, i + 1);
		exit();
	}

	return 0;
}
