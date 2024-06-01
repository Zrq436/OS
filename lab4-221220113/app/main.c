#include "lib.h"
#include "types.h"

sem_t mutex;
sem_t full_buffers;
sem_t empty_buffers;

void producer(int pid, int index) {
	// producer logic
	while(1){
		sem_wait(&empty_buffers);	//wait empty
		sem_wait(&mutex);	//wait mutex
		printf("Producer %d: produce(process:%d)\n", index, pid);
		sem_post(&mutex);
		sem_post(&full_buffers);
		sleep(128);
	}
}

void consumer(int pid) {
	while(1){
		sem_wait(&full_buffers);	//wait full
		sem_wait(&mutex);	//wait mutex
		printf("Consumer : consume(process:%d)\n", pid); 
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
	sem_init(&mutex, 1);
    sem_init(&full_buffers, 0);
	sem_init(&empty_buffers, 8);
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
		sleep(128);
		producer(pid, i + 1);
		exit();
	}

	return 0;
}
