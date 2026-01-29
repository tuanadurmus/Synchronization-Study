#ifndef STUDY_H
#define STUDY_H

#include <pthread.h>
#include <semaphore.h>
#include <stdexcept>
#include <iostream>
#include <string>

using namespace std;

static pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;

void print_status_safe(pthread_t tid, const string& status) {
    pthread_mutex_lock(&print_lock);
    cout << "Thread ID: " << (unsigned long)tid << " | Status: " << status << endl;
    pthread_mutex_unlock(&print_lock);
}

class Study{
    private:
        const int session_size; 
        const bool requires_tutor;
        const int all_count;
        int current_occupancy ;
        bool session_active;
        pthread_t tutor_tid ;
        int leaving_count ;
        pthread_mutex_t general_lock;
        sem_t waiting_students_sem;
        pthread_barrier_t leave_barrier;

    public:
        Study(int required_students, int has_tutor)
        : session_size(required_students), requires_tutor(has_tutor==1), all_count(required_students + has_tutor)
        {
            current_occupancy = 0;
            session_active = false;
            tutor_tid = 0;
            leaving_count = 0;
            if (required_students <= 0){
                throw invalid_argument("An error occured.");
            }
            if(has_tutor!= 0 && has_tutor != 1){
                throw invalid_argument("An error occured.");
            }
            if (pthread_mutex_init(&general_lock, NULL) != 0) {
                throw runtime_error("An error occured.");
            }
            if (sem_init(&waiting_students_sem, 0, all_count) != 0) {
                throw runtime_error("An error occured.");
            }
            if (pthread_barrier_init(&leave_barrier, NULL, all_count) != 0) {
                throw runtime_error("An error occured.");
            }
        }        
        ~Study(){
            pthread_mutex_destroy(&general_lock);
            sem_destroy(&waiting_students_sem);
        }
        void arrive() {
            pthread_t tid = pthread_self();
            print_status_safe(tid, "Arrived at the IC.");
            sem_wait(&waiting_students_sem);
            pthread_mutex_lock(&general_lock);
            current_occupancy++;
            bool session_just_started = false;
            if(!session_active){
                if (!requires_tutor){
                    if(current_occupancy == session_size){
                        session_active = true;
                        session_just_started = true;
                    }
                }
                else{
                    if(current_occupancy==all_count){
                            session_active=true;
                            session_just_started=true;
                            tutor_tid = tid;
                    }
                }
            }
            if (session_just_started){
                print_status_safe(tid, "There are enough students, the study session is starting.");

            }
            else{
                    
                print_status_safe(tid, "Only " + to_string(current_occupancy) + " students inside, studying individually.");
            }
            pthread_mutex_unlock(&general_lock);
            
        }
        void start();

        void leave() {
        pthread_t tid = pthread_self();
        pthread_mutex_lock(&general_lock);
        if (!session_active) {
            print_status_safe(tid, "No group study formed while I was waiting, I am leaving."); 
            current_occupancy--;
            sem_post(&waiting_students_sem); 
            pthread_mutex_unlock(&general_lock);
            return;
        }
        
        bool is_tutor = requires_tutor && pthread_equal(tid, tutor_tid);
        
        if (requires_tutor) {
            if (is_tutor) {
                print_status_safe(tid, "Session tutor speaking, the session is over.");
            }
            
            pthread_mutex_unlock(&general_lock);
        
            pthread_barrier_wait(&leave_barrier); 
        
            pthread_mutex_lock(&general_lock);
        } 
        
     
        if ( !is_tutor) {
            print_status_safe(tid, "I am a student and I am leaving."); 
        }
        
        current_occupancy--; 
        
        if (current_occupancy == 0) { 
            session_active = false; 
            tutor_tid = 0; 

            print_status_safe(tid, "All students have left, the new students can come."); 
            
            for (int i = 0; i < all_count; ++i) {
                sem_post(&waiting_students_sem);
            }
        }
        pthread_mutex_unlock(&general_lock);
    }
};

#endif
