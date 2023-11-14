#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <semaphore.h>

// structures

typedef struct StudentDetails{
	int studentid;
	int priority;
	struct StudentDetails *next;

}StudentDetails;


typedef struct StudentQueue{
	StudentDetails *Head;
	StudentDetails *Tail;
	int count;
}StudentQueue;

// global variables
int helps;
int maxhelps;
int helpcount;
sem_t *tutor_to_students;
int *student_tutor_map;
bool coordinatoractive;

// shared variables
int num_chairs;
pthread_mutex_t chair_lock;

sem_t student_to_coordinator;
sem_t coordinator_to_tutor;

StudentQueue studentqueue; // FIFO queue handled by student and coordinator
StudentQueue *tutoringqueue; // Multi level FIFO queue handled by coordinator and Tutors
pthread_mutex_t tutoringqueue_lock;

int tutorsactive;
int totalstudentshelped;

pthread_mutex_t tutorstat_lock;

// Helper functions
void addToStudentQueue(int studentid, int priority)
{
	StudentDetails *details = malloc(sizeof(StudentDetails));
	details -> studentid = studentid;
	details -> priority = priority;
	details -> next = NULL;

	if(studentqueue.Head == NULL && studentqueue.Tail == NULL) //then queue is empty
	{
		studentqueue.Head = details;
		studentqueue.Tail = details;

	}
	else if(studentqueue.Tail != NULL) //add to end of the queue
	{
		studentqueue.Tail->next = details;
		studentqueue.Tail = details;
	}

	studentqueue.count++;
}

StudentDetails* removeFromStudentQueue()
{
	if(studentqueue.Head == NULL) // No student in the queue
		return NULL;

	StudentDetails *studentrecord = studentqueue.Head;
	studentqueue.Head = studentqueue.Head->next;
	studentrecord -> next = NULL;

	if(studentqueue.Head == NULL) // set tail to NULL when queue is empty
		studentqueue.Tail = NULL;

	studentqueue.count--;

	return studentrecord;
}

void addToTutoringQueue(StudentDetails *details)
{
	int priority = details -> priority;

	StudentQueue *priorityqueue = &tutoringqueue[priority];

	if(priorityqueue->Head == NULL && priorityqueue->Tail == NULL) //then queue is empty
	{
		priorityqueue->Head = details;
		priorityqueue->Tail = details;

	}
	else if(priorityqueue->Tail != NULL) //add to end of the queue
	{
		priorityqueue->Tail->next = details;
		priorityqueue->Tail = details;
	}

	priorityqueue->count++;
}

StudentDetails* removeFromTutoringQueue() //will always remove student with max priority
{

	StudentQueue *priorityqueue = NULL;
	int priority; 
	for(priority = helpcount; priority > 0; priority--)
	{
		priorityqueue = &tutoringqueue[priority];
		if(priorityqueue->count > 0)
			break;
	}

	if(priorityqueue == NULL || priorityqueue->Head == NULL) // No student in the queue
	{
		printf("Error in remove from tutor queue. Checked queue count = %d\n", priorityqueue->count);
		abort();
		return NULL;
	}

	StudentDetails *studentrecord = priorityqueue->Head;
	priorityqueue->Head =  priorityqueue->Head->next;
	studentrecord -> next = NULL;

	if(priorityqueue->Head == NULL) // set tail to NULL when queue is empty
		priorityqueue->Tail = NULL;

	priorityqueue->count--;

	return studentrecord;
}
// implemetation
void tutoring()
{
	usleep(200); // will sleep for 0.2 ms
}

void doprogramming()
{
	usleep(2000); // will sleep for 2 ms
}

void *student_thread(void *arg)
{
	int studentid = *(int*)arg;
	int remaininghelps = helpcount;
	free(arg);
	//printf("student id = %d\n", studentid);

	// arrives at the csmc center 
	while(remaininghelps > 0)
	{
		// tries to find an empty chair to sit in waiting area
		bool chairfound = false;
		while(!chairfound)
		{
			pthread_mutex_lock(&chair_lock);
			if(num_chairs > 0)
			{
				num_chairs--;
				printf("S: Student %d takes a seat. Empty chairs = %d\n", studentid, num_chairs);
				chairfound = true;
				addToStudentQueue(studentid, remaininghelps);
				pthread_mutex_unlock(&chair_lock);
			}	
			else  // if no chair available then go back to programming
			{
				printf("S: Student %d found no empty chair. Will try again later.\n", studentid);
				pthread_mutex_unlock(&chair_lock);
				doprogramming();
			}
		}
		//
		// chair found, informs the co ordinator
		sem_post(&student_to_coordinator);

		// now waits in the waiting area for tutor to call
		sem_wait(&tutor_to_students[studentid-1]);

		// Now moves to tutoring area. Hence gives up the chair
		pthread_mutex_lock(&chair_lock);
		num_chairs++;
		pthread_mutex_unlock(&chair_lock);

		// gets tutored in tutoring area;
		tutoring();
		printf("S: Student %d received help from Tutor %d\n", studentid, student_tutor_map[studentid-1]); 
		student_tutor_map[studentid-1] = 0;
		remaininghelps--;
		// once help received from tutor go back to programming
		doprogramming();
	}
}

void *tutor_thread(void *arg)
{
	int tutorid = *(int*)arg;
	free(arg);
	//printf("tutor id = %d\n", tutorid);
	while(true)
	{
		//if(coordinatoractive)
		//{
			// wake up by co ordinator
			sem_wait(&coordinator_to_tutor);
		//}
		//else
		//{
		//	break;
		//}

		// find student with the highest priority
		pthread_mutex_lock(&tutoringqueue_lock);
		StudentDetails *student = removeFromTutoringQueue();
		pthread_mutex_unlock(&tutoringqueue_lock);

		if(student == NULL)
		{
			printf("Error in tutor thread %d\n", tutorid);
			abort();
			continue;
		}

		// notify student to move to tutoring area
		sem_post(&tutor_to_students[student->studentid-1]);

		// start tutoring
		pthread_mutex_lock(&tutorstat_lock);
		tutorsactive++;
		pthread_mutex_unlock(&tutorstat_lock);

		tutoring();
		student_tutor_map[student->studentid-1] = tutorid;

		pthread_mutex_lock(&tutorstat_lock);
		tutorsactive--;
		totalstudentshelped++;
		pthread_mutex_unlock(&tutorstat_lock);

		printf("T: Student %d tutored by Tutor %d. Students tutored now = %d. Total sessions tutored = %d\n", student->studentid, tutorid, tutorsactive, totalstudentshelped); 
		//free(student);
	}
}

void *coordinator_thread(void *arg)
{
	printf("coordinator thread started...\n");
	coordinatoractive = true;
	helps = 0;
	while(helps < maxhelps)
	{
		// once a student notifies, queue the student to tutors based on student priority
		printf("helps = %d, max helps = %d\n", helps, maxhelps);
		sem_wait(&student_to_coordinator);
		StudentDetails *student = removeFromStudentQueue(); //will the student who notified the coordinator be here?
		if(student == NULL) //error
		{
			printf("ERROR in remove from student queue. queue size = %d\n", studentqueue.count);
			abort();
			continue;
		}

		helps++;
		addToTutoringQueue(student);
		printf("C: Student %d with priority %d added to the queue. Waiting students now = %d. Total requests = %d.\n", student->studentid, student->priority, studentqueue.count, helps); 
		// notify an idle tutor
		sem_post(&coordinator_to_tutor);

	}
	
	printf("Coordinator gone home!!\n");
}


int main(int argc, char **argv)
{
	if(argc < 2 || argc > 5)
	{
		exit(0);
	}

	int studentscount = strtol(argv[1], NULL, 10);
	int tutorscount = strtol(argv[2], NULL, 10);
	num_chairs = strtol(argv[3], NULL, 10);
	helpcount = strtol(argv[4], NULL, 10); //this will set the priority for each student
	maxhelps = helpcount * studentscount;

	pthread_mutex_init(&chair_lock, NULL);
	pthread_mutex_init(&tutorstat_lock, NULL);
	pthread_mutex_init(&tutoringqueue_lock, NULL);
	sem_init(&student_to_coordinator, 0, 0);
	sem_init(&coordinator_to_tutor, 0, 0);

	int i;

	tutor_to_students = malloc(sizeof(sem_t) * studentscount); //a semaphore for each student
	tutoringqueue = calloc(helpcount+1, sizeof(StudentQueue));
	student_tutor_map = malloc(sizeof(int) * studentscount);

	for(i = 0; i < studentscount; i++)
	{
		sem_init(&tutor_to_students[i], 0, 0);
	}

	pthread_t coordinatortid;
	pthread_create(&coordinatortid, NULL, coordinator_thread, NULL);

	pthread_t tutorstids[tutorscount];
	for(i = 0; i < tutorscount; i++)
	{
		int *tutorid = malloc(sizeof(int));
		*tutorid = i+1;
		pthread_create(&tutorstids[i], NULL, tutor_thread, tutorid);
	}


	pthread_t studenttids[studentscount];
	for(i = 0; i < studentscount; i++)
	{
		int *studentid = malloc(sizeof(int)); //dynamic allocation avoids the use of locks
		*studentid = i+1;
		pthread_create(&studenttids[i], NULL, student_thread, studentid);
	}

	pthread_join(coordinatortid, NULL);
	for(i = 0; i < studentscount; i++)
	{
		pthread_join(studenttids[i], NULL);
		//printf("Student thread %d finished.\n", i);
	}

}
