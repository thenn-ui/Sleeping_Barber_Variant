#include <stdio.h>
#include <stdlib.h>


pthread_mutex_t id_init_lock;

void dotutoring()
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
	free(arg);
	printf("student id = %d\n", studentid);

	// arrives at the csmc center 
	//
	// finds an empty chair to sit in waiting area
	//
	// if no chair available then go back to programming
	//
	// once help received from tutor go back to programming
}

void *tutor_thread(void *arg)
{
	int tutorid = *(int*)arg;
	free(arg);
	printf("tutor id = %d\n", tutorid);

	// find student with the highest priority
	//
	// start tutoring
}

void *coordinator_thread(void *arg)
{
	printf("coordinator thread started...\n");
	sleep(5);

	// once student arrives, queue the student based on student priority
	//
	// notify an idle tutor
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
		int *studentid = malloc(sizeof(int));
		*studentid = i;
		pthread_create(&studenttids[i], NULL, student_thread, studentid);
	}

	pthread_t tutorstids[tutorscount];
	for(i = 0; i < tutorscount; i++)
	{
		int *tutorid = malloc(sizeof(int));
		*tutorid = i;
		pthread_create(&tutorstids[i], NULL, tutor_thread, tutorid);
	}

	pthread_join(coordinatortid, NULL);	

}
