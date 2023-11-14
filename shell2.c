#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/wait.h>

struct Process {
    int pid;
    int priority;
    int status;
    char program[32];
};

struct Process processes[5];

int stoppedPIDS[] = {0, 0, 0, 0, 0};

void removeProcess(int * slot) {
  struct Process temp;
  temp.pid = -1;
  temp.status = -1;
  temp.priority = 0;
  strcpy(temp.program, "N/A");
  processes[*slot] = temp;

}

void * waitOnProcess(void * test) {
  int temp = 0;
  int pid = *((int *)test);
  waitpid(pid, &temp, 0);
  int i;
  for (i = 0; i < 5; i++) {
    if (processes[i].pid == pid) {
      processes[i].priority = -2;
      break;
    }
  }
}

void startProcess(struct Process * process) {
    pthread_t pthread;

    int pid = fork();
    if (pid == 0) {

        char * arguments[] = {NULL};
        execve(process->program, arguments, NULL);
    } else {
        process->pid = pid;
        process->status = 1;
	pthread_create(&pthread, NULL, waitOnProcess, &pid);
	pthread_detach(pthread);
    }
}

void stopProcess(struct Process* process) {
    if (process->status == 1) {
        kill(process->pid, SIGSTOP);
        process->status = 0;
	for (int i = 0; i < 5; i++) {
          if (stoppedPIDS[i] == 0) {
            stoppedPIDS[i] = process->pid;
	    break;
	  }
	}
    }
}

int isRunning() {
  for (int i = 0; i < 5; i++) {
    if (processes[i].status == 1) {
      return i;
    }
  }
  return -1;
}

void displayProcesses() {
    printf("PID Priority Status Program\n");
    for (int i = 0; i < 5; i++) {
        if (processes[i].status != -1) {
          printf("%d    %d       %s    %s\n", processes[i].pid, processes[i].priority, processes[i].status == 1 ? "running" : "ready", processes[i].program);   
        }
    }
}

void * checkForDelete() {
  while (1) {
    for (int i = 0; i < 5; i++) {
      if (processes[i].priority == -2) {
        removeProcess(&i);
        break;
      }  
    }  
  }
}

int isWaitEmpty() {
  if (stoppedPIDS[0] == 0 && stoppedPIDS[1] == 0 && stoppedPIDS[2] == 0 && stoppedPIDS[3] == 0 && stoppedPIDS[4] == 0) {
    return 1;
  }
  else {
    return 0;
  }
}

int main() {
    pthread_t check;
    pthread_create(&check, NULL, checkForDelete, NULL);
    pthread_detach(check);

    for (int i = 0; i < 5; i++) {
        processes[i].pid = -1;
        processes[i].status = -1;
    }

    char input[32];
    int skip = 0;
    
    while (1) {
        sleep(0.25);
        printf("pshell: ");
        fgets(input, 32, stdin);
	skip = 0;

        if (strcmp(input, "status\n") == 0) {
            displayProcesses();
        } 
	else {
          char program[32];
          int priority;

          sscanf(input, "%s %d", program, &priority);

          int emptySlot = -1;
          for (int i = 0; i < 5; i++) {
            if (processes[i].status == -1) {
              emptySlot = i;
              break;
            }
          }

          if (emptySlot == -1) {
            printf("Not enough room for new job\n");
          } 
	  else {
            struct Process* newProcess = &processes[emptySlot];
            newProcess->priority = priority;
            newProcess->status = 0;
            strcpy(newProcess->program, program);


            if (isRunning() == -1 && isWaitEmpty() == 1) {
              startProcess(&processes[emptySlot]);
	    }
	    else if (isRunning() == -1 && isWaitEmpty() == 0) {
              int highest = 0;
	      int waiting = 0;
	      for (int i = 0; i < 5; i++) {
                if (processes[i].priority > processes[highest].priority) {
		  highest = i;
		}
	      }
	      for (int i = 0; i < 5; i++) {
                if (stoppedPIDS[i] == processes[highest].pid) {
                  waiting = 1;
		  stoppedPIDS[i] = 0;
		  break;
		}
	      }
	      if (waiting == 1) {
	        processes[highest].status = 1;
	        kill(processes[highest].pid, SIGCONT);
	      }
	      else {
	        startProcess(&processes[highest]);
	      }
	    }
	    else {
              for (int i = 0; i < 5; i++) {
                if (processes[i].status == 1 && processes[emptySlot].priority > processes[i].priority) {
                  stopProcess(&processes[i]);
		  startProcess(&processes[emptySlot]);
		  break;
		}
	      }
	    }
          }
       }
    }

    return 0;
}

