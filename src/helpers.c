#include "icssh.h"
#include "linkedlist.h"
// Your helper functions need to be here.
void freeHistory(list_t * list){
    for (node_t*temp = list->head; temp != NULL; temp = temp->next){
        list->deleter(temp->data);
    }
    DeleteList(list);
    free(list);
}
void printHistory(list_t * list){
    int i = 1;
    for (node_t * temp = list->head; temp != NULL; temp=temp->next){
        char * line = (char *) temp->data;
        printf("%d: %s\n",i, line);
        i++;
    }
}

void terminatePrint(job_info * job, pid_t pid){
    fprintf(stdout, "Background process %d: %s, has terminated.\n",pid, job->line);
}

void terminateProcess(list_t* list) {
    if (list->head == NULL){
        return;
    }
    int status;
    int wpid;
    bgentry_t * process = list->head->data;

    if (list->length == 1){
        if (wpid = waitpid(process->pid, &status, WNOHANG) >0){
            terminatePrint(process->job, process->pid);
            free_job(process->job);
            
            free(process);
            free(list->head);
            list->length--;
            list->head = NULL;
            return;
    }}
    node_t* prev = NULL;

    node_t * temp;

    node_t * current = list->head;

    while(current != NULL){
        bgentry_t * currentProcess = current->data;
        if (wpid = waitpid(currentProcess->pid, &status, WNOHANG) > 0){
            //head removal
            if (list->head == current){
                temp = current;
                list->head = current->next;
                current = current->next;
                terminatePrint(currentProcess->job, currentProcess->pid);
                free_job(currentProcess->job);
                free(currentProcess);
                free(temp);
                list->length--;
            }
            //tail removal
            else if (current->next == NULL){
                terminatePrint(currentProcess->job, currentProcess->pid);
                free_job(currentProcess->job);
                free(currentProcess);
                free(current);
                current = NULL;
                prev->next = NULL;
                list->length--;
            }

            //middle removal
            else{
                temp = current;
                prev->next = current->next;
                current = current->next;
                terminatePrint(currentProcess->job, currentProcess->pid);
                free_job(currentProcess->job);
                free(currentProcess);
                free(temp);

            }
        }

        else{
            prev = current;
            current = current->next;
        }
    }
    }
    


void linePrinter(void * line, void * stream){
    char * line1 = (char *) line;
    printf("%s\n", line1);
}
void lineDeleter(void * line1){
    char * line = (char *) line1;
    free(line);
}

void bgentryPrinter(void * bg1, void * stream){
    bgentry_t * bg = (bgentry_t *) bg1;
    print_bgentry(bg);
}

void bgentryDeleter(void * bg){
    bgentry_t * bg1 = (bgentry_t *) bg;
    free_job(bg1->job);
    free(bg1);
}
int bgentryCompatator(const void * bg1, const void * bg2){
    bgentry_t * bg = (bgentry_t *) bg1;
    bgentry_t * bg3 = (bgentry_t *) bg2;

    if (bg->seconds < bg3->seconds)
        return -1;
    if (bg->seconds == bg3->seconds)
        return 0;
    else
        return 1;
}

