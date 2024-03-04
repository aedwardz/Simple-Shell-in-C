#include "icssh.h"
#include "helpers.h"
#include "linkedlist.h"
#include <readline/readline.h>

volatile int sigchldFlag = 0;

volatile int numBgs = 0; //set in line 267

void sigchld_handler(int sig){
	sigchldFlag = 1;
}

void sigusr2_handler(int sig){
	const char * msg = "Num of Background processes: ";
	write(2, msg, strlen(msg));
	char buf[20];
	int num_char = snprintf(buf, sizeof(buf), "%d", numBgs);
	write(2, buf, num_char);
	char * newline = "\n";
	write(2, newline, strlen(newline));
}

int main(int argc, char* argv[]) {
    int max_bgprocs = -1;
	int exec_result;
	int exit_status;
	pid_t pid;
	pid_t wait_result;
	char* line;
	//int sigchldFlag;
#ifdef GS
    rl_outstream = fopen("/dev/null", "w");
#endif

    // check command line arg
    if(argc > 1) {
        int check = atoi(argv[1]);
        if(check != 0)
            max_bgprocs = check;
        else {
            printf("Invalid command line argument value\n");
            exit(EXIT_FAILURE);
        }
    }

	// Setup segmentation fault handler
	if (signal(SIGSEGV, sigsegv_handler) == SIG_ERR) {
		perror("Failed to set signal handler");
		exit(EXIT_FAILURE);
	}
	//implement stuff for the lists


	list_t * list = CreateList(&bgentryCompatator, &bgentryPrinter, &bgentryDeleter);
	list_t * history = CreateList(&bgentryCompatator, &linePrinter, &lineDeleter);
    	// print the prompt & wait for the user to enter commands string

	
	
	if (signal(SIGCHLD, sigchld_handler) == SIG_ERR){
		perror("Signal Error\n");
		}
	
	if (signal(SIGUSR2, sigusr2_handler) == SIG_ERR){
		perror("SIGUSR2 Error\n");
	}
	

	while ((line = readline(SHELL_PROMPT)) != NULL) {

		// printf("sigchldFlag at the beginning of the loop: %d\n ", sigchldFlag);
		if (sigchldFlag == 1){
			//traverse the list and use waitpid to check if a process has been terminated
			//if it has been terminated, then it will return something >0
			// printf("before termination\n");
			terminateProcess(list);
			sigchldFlag = 0;
			// printf("background process terminated\n");
			}

		

		if(*(line) == '!'){
			//check if second char is null terminator
			char * command = line;
			command++;
			if (*command == '\0'){
				free(line);
				char * newline = (char *)malloc(100);
				if (history->head == NULL){
					continue;
				}

				strcpy(newline, (char *)history->head->data);
				line = newline;
				printf("%s\n", line);
			}
			else{
						
				int historyNum = atoi(command);
				free(line);
				// if (*command)
				if (historyNum < 1 || historyNum > 5 ){
					continue;}
				else if (historyNum > history->length){
					continue;
				}
				else{
					int count = 1;
					char * newline = (char *)malloc(100);
					for (node_t * temp = history->head; temp != NULL; temp=temp->next){
						if (count == historyNum){
							strcpy(newline, (char *)temp->data);
							break;
						}
						count++;
					}
					line = newline;
					printf("%s\n", line);
				}
				}
		}

		if (*line != '!' && strcmp(line, "")!= 0 && strcmp(line, "history") != 0){
			char * dup = (char *)malloc(100);
			strcpy(dup, line);
			InsertAtHead(history, dup);
			if (history->length > 5){
				node_t * ptr = history->head;
				for (ptr; ptr->next != NULL; ptr = ptr->next){}
				history->deleter(ptr->data);
				RemoveFromTail(history);
			}}
		
		
		
		


        // MAGIC HAPPENS! Command string is parsed into a job struct
        // Will print out error message if command string is invalid
		job_info* job = validate_input(line);
        if (job == NULL) { // Command was empty string or invalid
			free(line);
			continue;
		}

        //Prints out the job linked list struture for debugging
        #ifdef DEBUG   // If DEBUG flag removed in makefile, this will not longer print
     		debug_print_job(job);
        #endif

		
		

		// example built-in: exit
		if (strcmp(job->procs->cmd, "exit") == 0) {
			// Terminating the shell
			free(line);
			free_job(job);
            // validate_input(NULL);  // calling validate_input with NULL will free the memory it has allocated

			freeHistory(history);
			//free bglist
			if (list->head==NULL)
				free(list);
			else{
				for (node_t * temp = list->head; temp != NULL; temp=temp->next){
					if(temp->data){
						bgentry_t *bg = temp->data;
						kill(bg->pid, SIGTERM);
						waitpid(bg->pid, &exit_status, 0);
						
						terminatePrint(bg->job, bg->pid);
						free_job(bg->job);
						free(bg);
					}
				}
				DeleteList(list);
				free(list);
			}
            break;
		}

		//handle cd
		if (strcmp(job->procs->cmd, "cd") == 0){
			int code;
			char ** argvv = job->procs->argv;
			// printf("Arguments:\n 0: %s\n 1: %s\n", argvv[0], argvv[1]);

			if (job->procs->argc == 1){
				code = chdir(getenv("HOME"));
			}
			else{
			
			code = chdir(argvv[1]);
			}

			if (code == -1){
				fprintf(stderr, "%s", DIR_ERR);
			}
			else{
				char cwd[100];
				getcwd(cwd, 100);
				printf("%s\n", cwd);
			}
			free(line);
			free_job(job);
			continue;
		}

		if (strcmp(job->procs->cmd, "estatus") == 0){
			if (WIFEXITED(exit_status)){
				printf("%d\n", WEXITSTATUS(exit_status));
			}
			else{
				printf("-100\n");
			}
			free(line);
			free_job(job);
			continue;
		}
		if (strcmp(job->procs->cmd, "bglist") == 0){
			node_t* ptr = list->head;
			
			for (int i = 0; i < list->length; i++){
				list->printer(ptr->data, ptr);
				ptr = ptr->next;

			}
			free(line);
			free_job(job);
			continue;

		}
		if (strcmp(job->procs->cmd, "history") == 0){
			printHistory(history);
			free(line);
			free_job(job);
			continue;
		}
		
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
		if (job->nproc > 1){
			if (job->nproc == 2){
				//create a pipe
				int p[2] = {0,0};
				pipe(p);
				
				pid_t p1;
				pid_t p2;

			if ((p1 = fork()) < 0) {
			perror("fork error");
			exit(EXIT_FAILURE);
			}

			if (p1 == 0){
				//connect to write side of pipe
				int newfd = dup2(p[1], STDOUT_FILENO);

				//close both sides
				close(p[0]);
				close(p[1]);
				//exec

				proc_info* proc = job->procs;
				exec_result = execvp(proc->cmd, proc->argv);
				if (exec_result < 0) {  //Error checking
					printf(EXEC_ERR, proc->cmd);
			

					free_job(job);
					free(line);
    				validate_input(NULL);  // calling validate_input with NULL will free the memory it has allocated

				exit(EXIT_FAILURE);}

			}
			
			if ((p2 = fork()) < 0) {
			perror("fork error");
			exit(EXIT_FAILURE);
		}
			if (p2 == 0){
				int newfd1 = dup2(p[0],STDIN_FILENO);
				close(p[0]);
				close(p[1]);

				proc_info* proc = job->procs->next_proc;
				exec_result = execvp(proc->cmd, proc->argv);
				if (exec_result < 0) {  //Error checking
					printf(EXEC_ERR, proc->cmd);
					free(line);
					free_job(job);
					validate_input(NULL);
				exit(EXIT_FAILURE);}
			}else{
				close(p[0]);
				close(p[1]);
				if (job->bg){
				time_t timing;
				bgentry_t * bgentry = (bgentry_t*)malloc(sizeof(bgentry_t));
				bgentry->job = job;
				bgentry->pid = pid;
				bgentry->seconds = time(&timing);
				InsertInOrder(list, bgentry);
				numBgs = list->length;
				free(line);
				
				continue;
			}
			wait_result = waitpid(p1, &exit_status, 0);
			// printf("Exit Status: %d\n", exit_status);
			if (wait_result < 0) {
				printf(WAIT_ERR);
				exit(EXIT_FAILURE);
			}
			wait_result = waitpid(p2, &exit_status, 0);
			// printf("Exit Status: %d\n", exit_status);
			if (wait_result < 0) {
				printf(WAIT_ERR);
				exit(EXIT_FAILURE);
			}
			
			}

			free(line);
			free_job(job);
			continue;
			}

			else if (job->nproc == 3){
				//create 2 pipes
				int p[2] = {0,0};
				pipe(p);
				int d[2] = {0,0};
				pipe(d);
				proc_info * proc1 = job->procs;
				proc_info * proc2 = proc1->next_proc;
				proc_info * proc3 = proc2->next_proc;

				
				pid_t p1;
				pid_t p2;
				pid_t p3;

			if ((p1 = fork()) < 0) {
			perror("fork error");
			exit(EXIT_FAILURE);
			}

			if (p1 == 0){
				//connect to write side of pipe
				int newfd = dup2(p[1], STDOUT_FILENO);

				//close both sides
				close(p[0]);
				close(p[1]);
				close(d[0]);
				close(d[1]);
				//exec

				proc_info* proc = job->procs;
				exec_result = execvp(proc->cmd, proc->argv);
				if (exec_result < 0) {  //Error checking
					printf(EXEC_ERR, proc->cmd);
			

					free_job(job);
					free(line);
    				validate_input(NULL);  // calling validate_input with NULL will free the memory it has allocated

				exit(EXIT_FAILURE);}

			}
			
			if ((p2 = fork()) < 0) {
			perror("fork error");
			exit(EXIT_FAILURE);
		}
			if (p2 == 0){
				int newfd1 = dup2(p[0],STDIN_FILENO);
				int newfd2 = dup2(d[1], STDOUT_FILENO);
				
				close(p[0]);
				close(p[1]);
				close(d[0]);
				close(d[1]);

				proc_info* proc = job->procs->next_proc;
				exec_result = execvp(proc->cmd, proc->argv);
				if (exec_result < 0) {  //Error checking
					printf(EXEC_ERR, proc->cmd);
					free(line);
					free_job(job);
					validate_input(NULL);
				exit(EXIT_FAILURE);}
			}

			if ((p3 = fork()) < 0) {
			perror("fork error");
			exit(EXIT_FAILURE);
		}
			if (p3 == 0){
				int newfd5 = dup2(d[0],STDIN_FILENO);
				
				close(p[0]);
				close(p[1]);
				close(d[0]);
				close(d[1]);

				proc_info* proc = proc3;
				exec_result = execvp(proc->cmd, proc->argv);
				if (exec_result < 0) {  //Error checking
					printf(EXEC_ERR, proc->cmd);
					free(line);
					free_job(job);
					validate_input(NULL);
				exit(EXIT_FAILURE);}
			}
			else{
				close(p[0]);
				close(p[1]);
				close(d[0]);
				close(d[1]);
				if (job->bg){
				time_t timing;
				bgentry_t * bgentry = (bgentry_t*)malloc(sizeof(bgentry_t));
				bgentry->job = job;
				bgentry->pid = pid;
				bgentry->seconds = time(&timing);
				InsertInOrder(list, bgentry);
				numBgs = list->length;
				free(line);
				continue;
				}
			// close(p[0]);
			// close(p[1]);
			// close(d[0]);
			// close(d[1]);
			wait_result = waitpid(p3, &exit_status, 0);
			// printf("Exit Status: %d\n", exit_status);
			if (wait_result < 0) {
				printf(WAIT_ERR);
				exit(EXIT_FAILURE);
			}
			wait_result = waitpid(p2, &exit_status, 0);
			// printf("Exit Status: %d\n", exit_status);
			if (wait_result < 0) {
				printf(WAIT_ERR);
				exit(EXIT_FAILURE);
			}
			wait_result = waitpid(p1, &exit_status, 0);
			// printf("Exit Status: %d\n", exit_status);
			if (wait_result < 0) {
				printf(WAIT_ERR);
				exit(EXIT_FAILURE);
			}
			}

			free(line);
			free_job(job);
			continue;
			}

			
		}


		
		//example of good error handling!
        //create the child proccess
		if ((pid = fork()) < 0) {
			perror("fork error");
			exit(EXIT_FAILURE);
		}

		

		if (pid == 0) {  //If zero, then it's the child process
            //get the first command in the job list to execute
			if (job->in_file){
			//open the file
				int fd = 0;
				fd = open(job->in_file, O_RDONLY);
				if(fd < 0){
					fprintf(stderr, "%s", RD_ERR);
					free_job(job);
					free(line);
					continue;
				}
				// printf("file descriptor (original): %d\n", fd);
				// printf("input file: %s\n", job->in_file);
				
				int newFd;
				if (newFd = dup2(fd,0) < 0){
					perror("New file descriptor");
				}
				
			}
			if (job->out_file){
			//open the file
				int fd;
				fd = open(job->out_file,O_WRONLY | O_CREAT | O_TRUNC);
				if(fd < 0){
					fprintf(stderr, "%s", RD_ERR);
					free_job(job);
					free(line);
					continue;
				}
				// printf("file descriptor (original): %d\n", fd);
				// printf("input file: %s\n", job->in_file);
				
				int newFd;
				if (newFd = dup2(fd,1) < 0){
					perror("New file descriptor");
				}
				
			}
			if (job->procs->err_file){
			//open the file
				char * error_file = job->procs->err_file;
				// printf("Error_file: %s\n", error_file);
				int fd;
				fd = open(error_file,O_WRONLY | O_CREAT | O_TRUNC);
				if(fd < 0){
					fprintf(stderr, "%s", RD_ERR);
					free_job(job);
					free(line);
					continue;
				}
				// printf("file descriptor (original): %d\n", fd);
				// printf("input file: %s\n", job->in_file);
				
				int newFd;
				if (newFd = dup2(fd,2) < 0){
					perror("New file descriptor");
				}
				
			}

		    proc_info* proc = job->procs;
			exec_result = execvp(proc->cmd, proc->argv);
			if (exec_result < 0) {  //Error checking
				printf(EXEC_ERR, proc->cmd);
			

				
				// Cleaning up to make Valgrind happy 
				// (not necessary because child will exit. Resources will be reaped by parent)
				free_job(job);  
				free(line);
    				validate_input(NULL);  // calling validate_input with NULL will free the memory it has allocated

				exit(EXIT_FAILURE);
			}
		} else {
			if (job->bg){
			time_t timing;
			bgentry_t * bgentry = (bgentry_t*)malloc(sizeof(bgentry_t));
			bgentry->job = job;
			bgentry->pid = pid;
			bgentry->seconds = time(&timing);
			InsertInOrder(list, bgentry);
			numBgs = list->length;
			free(line);
			continue;
		}
        	// As the parent, wait for the foreground job to finish
			wait_result = waitpid(pid, &exit_status, 0);
			// printf("Exit Status: %d\n", exit_status);
			if (wait_result < 0) {
				printf(WAIT_ERR);
				exit(EXIT_FAILURE);
			}
		}

		free_job(job);  // if a foreground job, we no longer need the data
		free(line);
	}
	

    	// calling validate_input with NULL will free the memory it has allocated
    	validate_input(NULL);

#ifndef GS
	fclose(rl_outstream);
#endif
	
	return 0;
}
