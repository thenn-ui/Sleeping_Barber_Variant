#include <stdio.h>
#include <stdlib.h>


pthread_mutex_t id_init_lock;

void tutor_sleep()
{
	usleep(200); // will sleep for 0.2 ms
}

void programming()
{
	usleep(2000); // will sleep for 2 ms
}

void *student_thread(void *arg)
{
	int studentid = *(int*)arg;
	printf("student id = %d\n", studentid);
	pthread_mutex_unlock(&id_init_lock);
}

void *tutor_thread(void *arg)
{
	int tutorid= *(int*)arg;
	printf("tutor id = %d\n", tutorid);
	pthread_mutex_unlock(&id_init_lock);
}

void *coordinator_thread(void *arg)
{
	printf("coordinator thread started...\n");
	sleep(5);
}


int main(int argc, char **argv)
{
	if(argc < 2 || argc > 5)
	{
		exit(0);
	}
	
	int studentscount = strtol(argv[1], NULL, 10);
	int tutorscount = strtol(argv[2], NULL, 10);
	int num_chairs = strtol(argv[3], NULL, 10);
	int helpcount = strtol(argv[4], NULL, 10);

	pthread_mutex_init(&id_init_lock, NULL);

	pthread_t coordinatortid;
	pthread_create(&coordinatortid, NULL, coordinator_thread, NULL);

	int i;
	pthread_t studenttids[studentscount];
	for(i = 0; i < studentscount; i++)
	{
		pthread_mutex_lock(&id_init_lock);
		pthread_create(&studenttids[i], NULL, student_thread, &i);
	}

	pthread_t tutorstids[tutorscount];
	for(i = 0; i < tutorscount; i++)
	{
		pthread_mutex_lock(&id_init_lock);
		pthread_create(&tutorstids[i], NULL, tutor_thread, &i);
	}

	pthread_join(coordinatortid, NULL);	

}
