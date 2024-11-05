/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Fall 2022
 *
 * Kernel Semaphores
 */

#include <spede/string.h>

#include "kernel.h"
#include "ksem.h"
#include "queue.h"
#include "scheduler.h"

// Table of all semephores
sem_t semaphores[SEM_MAX];

// semaphore ids to be allocated
queue_t sem_queue;

/**
 * Initializes kernel semaphore data structures
 * @return -1 on error, 0 on success
 */
int ksemaphores_init() {
    kernel_log_info("Initializing kernel semaphores");

    // Initialize the semaphore table
    memset(&semaphores, 0, sizeof(semaphores));
    // Initialize the semaphore queue
    if(!queue_init(&sem_queue)){
        return -1;
    }
    // Fill the semaphore queue
    for(int i = 0; i < SEM_MAX; i++){
        if(!queue_in(&sem_queue, i)){
            return -1;
        }
    }
    
    return 0;
}

/**
 * Allocates a semaphore
 * @param value - initial semaphore value
 * @return -1 on error, otherwise the semaphore id that was allocated
 */
int ksem_init(int value) {
    int id = 0;
    // Obtain a semaphore id from the semaphore queue
    queue_out(&sem_queue, &id);
    // Ensure that the id is within the valid range
    if(id < 0 || id > SEM_MAX -1 ){
        return -1;
    }

    sem_t * table_entry = &semaphores[id];

    // Initialize the semaphore data structure
    // sempohare table + all members (wait queue, allocated, count)
        // set count to initial value

    table_entry->allocated = 1;
    table_entry->count = 0;
    queue_init(&table_entry->wait_queue);

    kernel_log_debug("initialized semaphore %d", id);
    return id;
}

/**
 * Frees the specified semaphore
 * @param id - the semaphore id
 * @return 0 on success, -1 on error
 */
int ksem_destroy(int id) {
    sem_t * sem = NULL;
    
    // look up the sempaphore in the semaphore table
    if(id < 0 || id > (SEM_MAX - 1)){
        return -1;
    }
    else{
        sem = &semaphores[id];
    }
    // If the semaphore is locked, prevent it from being destroyed
    if(sem->count > 0){
        return -1;
    }
    // Add the id back into the semaphore queue to be re-used later
    if(!queue_in(&sem_queue,id)){
        return -1;
    }
    // Clear the memory for the data structure
    memset(sem, 0, sizeof(semaphores[id]));
    kernel_log_debug("removed semaphore id: %d", id);
    return 0;
}

/**
 * Waits on the specified semaphore if it is held
 * @param id - the semaphore id
 * @return -1 on error, otherwise the current semaphore count
 */
int ksem_wait(int id) {
    if(active_proc == NULL || active_proc->pid == 0){
        return -1;
    }
    
    // look up the sempaphore in the semaphore table
    sem_t * sem = NULL;
    
    if(id < 0 || id > (SEM_MAX - 1)){
        return -1;
    }
    else{
        sem = &semaphores[id];
    }

    // If the semaphore count is 0, then the process must wait
    // Set the state to WAITING
    if(sem->count == 0){
        active_proc->state = WAITING;
    // add to the semaphore's wait queue
        queue_in(&sem->wait_queue, active_proc->pid);
    // remove from the scheduler
        scheduler_remove(active_proc);
    }

    // If the semaphore count is > 0 decrement the count
    if(sem->count > 0){ sem->count--; }

    // Return the current semaphore count
    return sem->count;
}

/**
 * Posts the specified semaphore
 * @param id - the semaphore id
 * @return -1 on error, otherwise the current semaphore count
 */
int ksem_post(int id) {
    if(active_proc == NULL || active_proc->pid == 0){
        return -1;
    }
    
    // look up the sempaphore in the semaphore table
    sem_t * sem = NULL;
    
    if(id < 0 || id > (SEM_MAX - 1)){
        return -1;
    }
    else{
        sem = &semaphores[id];
    }

    // incrememnt the semaphore count
    sem->count++;

    // check if any processes are waiting on the semaphore (semaphore wait queue)
    // if so, queue out and add to the scheduler
    if(&sem->wait_queue.size > 0){
        int pid;
        queue_out(&sem->wait_queue, &pid);
        scheduler_add(pid_to_proc(pid));
    }

    // decrement the semaphore count
    sem->count--;

    // return current semaphore count
    return sem->count;
}
