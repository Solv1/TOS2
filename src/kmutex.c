/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Fall 2022
 *
 * Kernel Mutexes
 */

#include <spede/string.h>

#include "kernel.h"
#include "kmutex.h"
#include "queue.h"
#include "scheduler.h"

// Table of all mutexes
mutex_t mutexes[MUTEX_MAX];

// Mutex ids to be allocated
queue_t mutex_queue;

/**
 * Initializes kernel mutex data structures
 * @return -1 on error, 0 on success
 */
int kmutexes_init() {
    kernel_log_info("Initializing kernel mutexes");

    // Initialize the mutex table
    memset(&mutexes, 0, sizeof(mutexes));

    // Initialize the mutex queue
    if(!queue_init(&mutex_queue)){
        return -1;
    }

    // Fill the mutex queue
    for(int i = 0; i < MUTEX_MAX; i++){
       if(!queue_in(&mutex_queue,i)){
            return -1;
       }
    }
    //What Condition does this error out?
    return 0;
}

/**
 * Allocates a mutex
 * @return -1 on error, otherwise the mutex id that was allocated
 */
int kmutex_init(void) {
    int id = 0;
    // Obtain a mutex id from the mutex queue
    queue_out(&mutex_queue, &id);
    // Ensure that the id is within the valid range
    if(id < 0 || id > MUTEX_MAX - 1){
        return -1;
    }
    // Pointer to the mutex table entry
    mutex_t * table_entry = &mutexes[id];
    // Initialize the mutex data structure (mutex_t + all members)
    table_entry->allocated = 1;
    table_entry->locks = 0;
    table_entry->owner = NULL;
    if(!queue_init(&table_entry->wait_queue)){
        return -1;
    }
    // return the mutex id
    return id;
}

/**
 * Frees the specified mutex
 * @param id - the mutex id
 * @return 0 on success, -1 on error
 */
int kmutex_destroy(int id) {
    mutex_t* mutex = NULL;
    // look up the mutex in the mutex table
    if((id > (MUTEX_MAX-1)) || (id < 0)){
        return -1;
    }
    else{
        mutex = &mutexes[id];
    }
    // If the mutex is locked, prevent it from being destroyed (return error)
    if(mutex->owner != NULL ){
        return -1;
    }
    // Add the id back into the mutex queue to be re-used later
    if(!queue_in(&mutex_queue,id)){
        return -1;
    }
    // Clear the memory for the data structure
    memset(mutex,0,sizeof(mutexes[id]));
    return 0;
}

/**
 * Locks the specified mutex
 * @param id - the mutex id
 * @return -1 on error, otherwise the current lock count
 */
int kmutex_lock(int id) {
    if(active_proc == NULL){
        return -1;
    }
    mutex_t* mutex = NULL;
    // look up the mutex in the mutex table
    if((id > (MUTEX_MAX-1)) || (id < 0)){
        return -1;
    }
    else{
        mutex = &mutexes[id];
    }
    // If the mutex is already locked
    //   1. Set the active process state to WAITING
    //   2. Add the process to the mutex wait queue (so it can take
    //      the mutex when it is unlocked)
    //   3. Remove the process from the scheduler, allow another
    //      process to be scheduled
    // If the mutex is not locked
    //   1. set the mutex owner to the active process
    //   2. Increment the lock count
    //   3. Return the mutex lock count

    if(mutex->owner != active_proc){
        active_proc->state = WAITING;
        queue_in(&mutex->wait_queue,active_proc->pid);
        return mutex->locks;
    }
    else{
        mutex->owner = active_proc;
        mutex->locks++;
        return mutex->locks;
    }
    return -1;
}

/**
 * Unlocks the specified mutex
 * @param id - the mutex id
 * @return -1 on error, otherwise the current lock count
 */
int kmutex_unlock(int id) {
    mutex_t * mutex = NULL;
    int next_id = 0;
    // look up the mutex in the mutex table
    if((id > (MUTEX_MAX-1)) || (id < 0)){
        return -1;
    }
    else{
        mutex = &mutexes[id];
    }
    // If the mutex is not locked, there is nothing to do
    if(mutex->owner != NULL){
        return mutex->locks;
    }
    // Decrement the lock count
    mutex->locks--;
    // If there are no more locks held:
    //    1. clear the owner of the mutex

    // If there are still locks held:
    //    1. Obtain a process from the mutex wait queue
    //    2. Add the process back to the scheduler
    //    3. set the owner of the of the mutex to the process
    // return mutex locks value
    if(mutex->locks == 0){
        mutex->owner = NULL;
        return mutex->locks;
    }
    else{
        if(!queue_out(&mutex->wait_queue,&next_id)){
            return -1;
        }
        scheduler_add(pid_to_proc(next_id));
        mutex->owner = pid_to_proc(next_id);
        return mutex->locks;
    }

}
