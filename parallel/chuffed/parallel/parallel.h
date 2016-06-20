#ifndef parallel_h
#define parallel_h

// Slave states

#define RUNNING_GREEDY          0
#define NORMAL_SEARCH           1
#define COMPLEMENT              2
#define PORTFOLIO_SEARCH        3

#ifdef PARALLEL
  #define PAR_EXEC(x) x 
#else
  #define PAR_EXEC(x) 
#endif

#ifdef PARALLEL

#include "mpi.h"

#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

#define MAX_SLAVES 128
#define MPI_BUFFER_SIZE 500000000

// Reporting options

#define PAR_DEBUG 0
#define FULL_DEBUG 0
#define PROFILING 0
#define MIN_PROF_TIME 0.05

// Special values for job_start_time

#define NOT_WORKING 1E100
#define DONT_DISTURB 1E90
#define ABORTING 1E80

// Master to Slave message tags
enum { JOB_TAG = 1, LEARNTS_TAG, STEAL_TAG, INTERRUPT_TAG, FINISH_TAG, SOLUTION_TAG, INT_SOLUTION_TAG, NEW_EXPORT_LIMIT , NEW_STATE_TAG};

// Slave to Master message tags
enum { REPORT_TAG = 1, SPLIT_TAG, REPORT_SOLUTION_TAG, REPORT_OPTIMUM_TAG, SOLUTION_PHASE_TAG, IMPORT_LIMITS, SLAVE_NEW_STATE_TAG };



//=================================================================================================
// Parallel Data:


class SClause {
public:
        unsigned short size;
        unsigned char extra;
        unsigned char source;
        int data[0];
        SClause() : size(0), extra(0), source(-1) {}
        int memSize() { return sizeof(SClause)/4+size+extra; }
        void pushInVec(vec<int>& a) {
                for (int i = 0; i < memSize(); i++) a.push(((int*) this)[i]);
        }
        SClause* getNext() { return (SClause*) &data[size+extra]; }
        SClause* copy() {
                SClause* sc = (SClause*) malloc(memSize() * sizeof(int));
                sc->size = size; sc->extra = extra; sc->source = source;
                for (int i = 0; i < size+extra; i++) sc->data[i] = data[i];
                return sc;
        }
        void pop() {
                assert(size);
                if (extra && (0x8000000 & data[size+extra-2])) extra--;
                size--;
        }
        void negateLast() {
                assert(size);
                // TODO: Is das hier richtig?
                if(extra && (0x8000000 & data[size + extra - 2])){
                  // Okay, last value refers to int-lit! 
                  data[size+extra-2] ^= 1;
                }
                else
                  data[size+extra-1] ^= 1;
        }
        void negate(int index){
          assert(size);
          assert(index < size);
          int pos = 0;
          while(index > 0){
            if(data[pos] & 0x8000000){
              pos += 2;
            }
            else
              pos+=1;
            index--;
          }
          assert(pos < size + extra);
          data[pos] ^= 1;
        }
};

class Report {
public:
        int status;
        int num_learnts;
        int data[0];
        Report() {}
};

//-----

class Master {
public:

        // Parameters
        int num_threads;
        int thread_no;

        // Clause sharing parameters
        double min_share_time;
        double min_job_time;

        // Master search tree data
        vec<double> job_start_time;
        vec<double> job_start_time_backup; // when stealing jobs, store when job was actually started!
        vec<SClause*> job_queue;
        vec<SClause*> cur_job;

        // Shared learnt clauses
        vec<int> global_learnts;
        vec<int> lhead;

        // Verification data
        vec<int> search_progress;

        // Solver state data
        int num_free_slaves;
        RESULT status;
        
        int slavesRunningGreedy;
        std::vector<int> slaveStates;
        int nextSlaveToStealFrom;
        void setState(int thread_no, int state);

        // Timers
        vec<double> last_send_learnts;
        double t;
        bool stoppedInit;
        // MPI
        MPI_Status s;
        int message_length;
        int *message;

        // Stats
        long long int shared;
        long long int shared_len;
        double real_time;
        double cpu_time;
        double lastCubeFinished;
        int bestObjReceived;
        // importLimits(i*num_threads+j) = i's limit to import clauses from j
        vec<int> importLimits;
        vec<int> exportLimits;
        Master();

        
        void initMPI();
        void finalizeMPI();

        void solve();
        void receiveReport();
        void updateShareCriteria();
        void receiveSolution();
        void receivePhase();
        void sendLearnts(int thread_no);
        bool updateProgress(int i);
        void receiveLearnts(Report& r, int thread_no, int message_length);
        bool sendJob();
        int  selectJob(int thread_no);
        void stealJobs();
        bool createNewJob(int thread_no, bool random);
        void receiveJobs();
        void receiveOptObj();
        void updateExchangeLimits();
        
        void collectStats();
        void printStats();

};

extern Master master;

class Slave {
public:

        // Parameters
        int thread_no;
        double check_freq;
        double report_freq;

        // Solver state data
        RESULT status;

        // Timers
        double next_check;
        double t;
        int _state;

        // MPI
        MPI_Status s;
        int message_length;
        int *message;
        vec<int> report_message;

        // Stats
        int checks;
        double real_time;
        double cpu_time;
        bool unitFound;         // If unit clause was found, send report ASAP

        Slave();
        /* 
         * statistics on received clauses: 
         *      - how many clauses where received from solver "i"?
         *      - how many of them were good?
         *      - what is the clause size limit for clauses received form "i"?
         *  c.f. "Control-based Clause Sharing in Parallel SAT Solving" (ICJAI '09)
         * */
        std::vector<double> clauseSizeLimits;
        std::vector<int> goodClausesFrom;
        std::vector<int> numClausesFrom;
        int numSkipped;
        int numAdded;
        void solve();
        bool receiveJob();
        bool checkMessages();
        void sendReport();
        void splitJob();
        void receiveLearnts();

        void sendStats();
        void receiveSolution();
        void exportBounds();
        void sendObjective(int objVal);
        // Minor methods

        void shareClause(Clause& c);
        // Intermediate storages for clauses and solutions: 
        std::vector<std::vector<int> > storedClauses;
        vec<int> storedSolutions;
        std::vector<int> tmp_header;
        // TODO: remember how many clauses were actually received since last import!
        bool newClausesReceived() { return true; }
        int state() { return _state; }
};

extern Slave slave;

// Debug

inline void printArray(int *a, int sz) {
        for (int i = 0; i < sz; i++) fprintf(stderr, "%d ", a[i]);
        fprintf(stderr, "\n");
}

// Profiling

#define profile_start() if (PROFILING) t = wallClockTime()
#define profile_end(action, length)    \
        if (PROFILING) {                     \
                t = wallClockTime() - t;           \
                if (t > MIN_PROF_TIME) fprintf(stderr, "%d: Time wasted by " action " = %f, message length = %d\n", thread_no, t, length); \
        }                            

#else

// Dummy Master class for when compiled without PARALLEL support

class Master {
public:

        RESULT status;

        Master() {}

        void initMPI() { NEVER; }
        void finalizeMPI() { NEVER; }
        void solve() { NEVER; }
        void printStats() { NEVER; }

};

extern Master master;

// Dummy Slave class for when compiled without PARALLEL support

class Slave {
public:

        Slave() {}

        void solve() { NEVER; }
        bool checkMessages() { NEVER; }
        void shareClause(Clause& c) { NEVER; }
        bool newClausesReceived() { NEVER; }
        int state() { NEVER; }
};

extern Slave slave;

#endif

#endif
