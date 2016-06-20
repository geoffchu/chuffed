#include <ctime>
#include <cstring>
#include <chuffed/core/options.h>
#include <chuffed/core/engine.h>
#include <chuffed/core/sat.h>
#include <chuffed/parallel/parallel.h>
#include <chuffed/vars/int-var.h>
Master master;

#ifdef PARALLEL

Master::Master() :
                num_threads(0)
        , thread_no(-1)
        , min_share_time   (1.0)
        , min_job_time     (0.025)
        , num_free_slaves  (0)
        , status           (RES_UNK)
        , nextSlaveToStealFrom(-1)
        , shared           (0)
        , shared_len       (0)
        , real_time        (0)
        , cpu_time         (0)
{}


void Master::initMPI() {
        MPI_Init(NULL, NULL);
  MPI_Comm_size(MPI_COMM_WORLD, &so.num_threads);
  MPI_Comm_rank(MPI_COMM_WORLD, &so.thread_no);

        so.num_threads--;
        so.thread_no--;

        if (so.num_threads > MAX_SLAVES) ERROR("Maximum number of slaves (%d) exceeded!\n", MAX_SLAVES);
}

void Master::finalizeMPI() {
        MPI_Finalize();
}
/*
 * Not used anymore
 * */
void Master::receiveSolution()
{
  MPI_Get_count(&s, MPI_INT, &message_length);
  message = (int*) malloc(message_length*sizeof(int));
  MPI_Recv(message, message_length, MPI_INT, s.MPI_SOURCE, REPORT_SOLUTION_TAG, MPI_COMM_WORLD, &s);
  //fprintf(stderr, "Solution from %d contained %d literals\n", s.MPI_SOURCE, message[0]);
  
  free(message);
  //fprintf(stderr, "Master::receiveSolution is done...\n");
}
/* * 
 * Receive objective from slave... Just to show what has been found
 * 
 * */
void Master::receiveOptObj(){
  int obj;
  MPI_Recv(&obj, 1, MPI_INT, MPI_ANY_SOURCE, REPORT_OPTIMUM_TAG, MPI_COMM_WORLD, &s);
  // Sender ist s.MPI_SOURCE - 1 (!!!) 
  if(so.verbosity >= 2)
    fprintf(stderr, "Received result %d from %d\n", obj, s.MPI_SOURCE-1);
  if(engine.opt_type){
    // Maximisation: 
    if(obj > bestObjReceived){
      bestObjReceived = obj;
      
      if(slaveStates[s.MPI_SOURCE-1] == NORMAL_SEARCH)
        nextSlaveToStealFrom = s.MPI_SOURCE-1;
    }
  }
  else
    if(obj < bestObjReceived){
      bestObjReceived = obj;
      
      if(slaveStates[s.MPI_SOURCE-1] == NORMAL_SEARCH)
        nextSlaveToStealFrom = s.MPI_SOURCE-1;
    }
}
void Master::solve() {  

        lastCubeFinished = wallClockTime();
        num_threads = so.num_threads;
        job_start_time.growTo(num_threads, DONT_DISTURB);
        job_start_time_backup.growTo(num_threads, DONT_DISTURB);
        cur_job.growTo(num_threads, NULL);
        lhead.growTo(num_threads, 0);
        last_send_learnts.growTo(num_threads, 0);
        global_learnts.reserve(10000000);
        long maxCycleTime = 0;
        stoppedInit = false;
        if(engine.opt_var){
          bestObjReceived = engine.opt_type ? engine.opt_var->getMin() : engine.opt_var->getMax();
        }
        
        if(so.purePortfolio){
          for(int i = 0 ; i < num_threads ; i++)
            job_queue.push(new SClause());
        }
        else
          job_queue.push(new SClause());
        
        for(int i = 0 ; i < num_threads ; i++){
          if(so.greedyInit && (i  % 3 < 2)){
            slaveStates.push_back(RUNNING_GREEDY);
            slavesRunningGreedy++;
            setState(i, RUNNING_GREEDY);
          }
          else{
            slaveStates.push_back(NORMAL_SEARCH);
            setState(i, NORMAL_SEARCH);
          }
        } 

        MPI_Buffer_attach(malloc(MPI_BUFFER_SIZE), MPI_BUFFER_SIZE);

        // Search:
        int lastPrinted = time(NULL);
        int tStart = lastPrinted;
        long lastSleep = clock();
        bool stealJobsNow = true;
        while (status == RES_UNK && time(NULL) < so.time_out) {
                //fprintf(stderr, "Trying to send jobs...\n");
                while (num_free_slaves > 0 && job_queue.size() > 0) {
                  if(!sendJob())
                    break;
                }
                /**************************************************************************
                 * Ask all jobs to finish the first phase, if this has not happened yet.
                 * */
                if(!stoppedInit &&  time(NULL) > engine.half_time){
                  if(PAR_DEBUG) fprintf(stderr, "Asking remaining init-workers to continue with normal search...\n");
                  for(int i = 0 ; i < num_threads ; i++){
                    if(slaveStates[i] == RUNNING_GREEDY){
                      setState(i, NORMAL_SEARCH);
                    }
                  }
                  stoppedInit = true;
                }
                int received;
                MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &received, &s);
                if (received) {
                        double t;
                        profile_start();
                      //  printf("Profiling started...\n");
                        switch (s.MPI_TAG) {
                                case REPORT_TAG:
                                    //    printf("It was a report!\n");
                                        receiveReport();
                                    //    printf("ReceiveReport done!\n");
                                        profile_end("receive report", 0);
                                        continue;
                                case SPLIT_TAG:
                                     //   printf("Split tag received!\n");
                                        receiveJobs();
                                        profile_end("receive jobs", 0);
                                        continue;
                                case REPORT_SOLUTION_TAG:
                                    if(PAR_DEBUG) fprintf(stderr, "Received solution! \n");
                                    receiveSolution();
                                    continue;
                                case REPORT_OPTIMUM_TAG:
                                  receiveOptObj();
                                  continue;
                                case SOLUTION_PHASE_TAG:
                                  receivePhase();
                                  continue;
                                case SLAVE_NEW_STATE_TAG:
                                  int dummy;
                                  MPI_Recv(&dummy, 1, MPI_INT, s.MPI_SOURCE, SLAVE_NEW_STATE_TAG, MPI_COMM_WORLD, &s);
                                  if(PAR_DEBUG) fprintf(stderr, "Setting state of slave %d to %d\n", s.MPI_SOURCE-1, dummy);
                                  if(slaveStates[s.MPI_SOURCE-1] == RUNNING_GREEDY && dummy == NORMAL_SEARCH)
                                    slavesRunningGreedy--;
                                  slaveStates[s.MPI_SOURCE-1] = dummy;
                                  continue;
                                  continue;
                                default:
                                        assert(false);
                        }
                }
                if (job_queue.size() < 2*num_threads-2 && !so.purePortfolio ) { 
                  // Steal jobs if 
                  // - normal mode
                  // - greedy-init, and at least one job finished now...
                  if(!so.greedyInit || slavesRunningGreedy < num_threads){
                    stealJobs(); 
                    //continue; 
                  }
                  
                }
                
                long now = clock();
                maxCycleTime = std::max(maxCycleTime, now - lastSleep);
                usleep(500);
                lastSleep = clock();
                
                
        }
        if (PAR_DEBUG){
          fprintf(stderr, "Waiting for slaves to terminate...\n");
          fprintf(stderr, "Max cycle time: %d (%lf)\n", maxCycleTime, (double)maxCycleTime/CLOCKS_PER_SEC);
          fprintf(stderr, "End of problem called\n");
        }
        MPI_Request r;
        for (int i = 0; i < num_threads; i++) {
                MPI_Isend(NULL, 0, MPI_INT, i+1, INTERRUPT_TAG, MPI_COMM_WORLD, &r);
                MPI_Isend(NULL, 0, MPI_INT, i+1, FINISH_TAG, MPI_COMM_WORLD, &r);
        }

        while (num_free_slaves != num_threads) {
                MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &s);
                int thread_no = s.MPI_SOURCE-1;

                MPI_Get_count(&s, MPI_INT, &message_length);
                message = (int*) malloc(message_length*sizeof(int));
                MPI_Recv(message, message_length, MPI_INT, s.MPI_SOURCE, s.MPI_TAG, MPI_COMM_WORLD, &s);
                if (s.MPI_TAG == REPORT_TAG) {
                        if (message[0] != RES_SEA) {
                                assert(job_start_time[thread_no] != NOT_WORKING);
                                num_free_slaves++;
                                job_start_time[thread_no] = NOT_WORKING;
                                if (PAR_DEBUG) fprintf(stderr, "%d is free, %f\n", thread_no, wallClockTime());
                        }
                }

                free(message);
        }

        collectStats();
        if(PAR_DEBUG) fprintf(stderr, "Master terminating with obj in %d and %d, bestResult= %d\n", engine.opt_var->getMin(), engine.opt_var->getMax(), bestObjReceived);
        if(so.verbosity > 0)
          printStats();

}
/* 
 * Receive phase (integer values for integer variables, boolean for boolean), and forward it to other solvers 
 * 
 * Not used anymore
 * */
void Master::receivePhase(){
  MPI_Get_count(&s, MPI_INT, &message_length);
  message = (int*) malloc(message_length*sizeof(int));
  MPI_Recv(message, message_length, MPI_INT, s.MPI_SOURCE, SOLUTION_PHASE_TAG, MPI_COMM_WORLD, &s);
  
  free(message);
}
void Master::receiveReport() {
        int thread_no = s.MPI_SOURCE-1;

        MPI_Get_count(&s, MPI_INT, &message_length);
        message = (int*) malloc(message_length*sizeof(int));
        MPI_Recv(message, message_length, MPI_INT, s.MPI_SOURCE, REPORT_TAG, MPI_COMM_WORLD, &s);
        Report& r = *((Report*) message);

        //for (int i = 0; i < message_length; i++) fprintf(stderr, "%d ", message[i]); fprintf(stderr, "\n");

        if (PAR_DEBUG) fprintf(stderr, "Received report %d from %d\n", r.status, thread_no);

        if (r.status == RES_SEA) {
                if (wallClockTime() - last_send_learnts[thread_no] > min_share_time) sendLearnts(thread_no);
        }

        if (r.status != RES_SEA) {
                assert(job_start_time[thread_no] != NOT_WORKING);
                num_free_slaves++;
                job_start_time[thread_no] = NOT_WORKING;
                if (PAR_DEBUG) fprintf(stderr, "%d is free, %f\n", thread_no, wallClockTime());
        }
        
        if(slaveStates[thread_no] == NORMAL_SEARCH && r.status == RES_LUN){
          if(PAR_DEBUG) fprintf(stderr, "LUN: Slave %d solved cube of size %d\n", thread_no, cur_job[thread_no] ? cur_job[thread_no]->size : -1);
        }

        
        if (r.status == RES_GUN) { 
            if(PAR_DEBUG) fprintf(stderr, "Master: Received RES_GUN from %d\n", thread_no); 
            status = RES_GUN;
        }
        if (r.status == RES_SAT){
          if(PAR_DEBUG) fprintf(stderr, "Master: Received RES_SAT from %d\n", thread_no);
          status = RES_SAT;
        }
        

        if (r.status == RES_LUN && slaveStates[thread_no]==NORMAL_SEARCH && updateProgress(cur_job[thread_no]->size)) {
          if(so.greedyInit){
            
            assert(job_queue.size() == 0 && "size of job queue is strange? ");
            status = RES_GUN;
          }
          else{
                for (int i = 0; i < num_threads; i++) 
                  assert(job_start_time[i] == NOT_WORKING);
                assert(job_queue.size() == 0 && "Finished, but there are still jobs left?");
                status = RES_GUN;
          }
        }
        if(r.status == RES_GREEDY_DONE ){
          if(thread_no % 3 == 0){
            
          }
          else if(thread_no % 3 == 1){
            // Slave is waiting right now. So, set state here immediately!
            if(so.mixWS_Portfolio){
              slaveStates[thread_no] = PORTFOLIO_SEARCH;
              setState(thread_no, PORTFOLIO_SEARCH);
            }
            else{
              slaveStates[thread_no] = NORMAL_SEARCH;
              setState(thread_no, NORMAL_SEARCH);
            }
            slavesRunningGreedy--;
          }
          assert(so.greedyInit && "Slave returns GREEDY_DONE, but not in greedy-mode?");
          
        }

        if(r.status == RES_LUN && slaveStates[thread_no] == RUNNING_GREEDY){
          if(thread_no % 3 == 0){
            
          }
          else if(thread_no % 3 == 1){
            setState(thread_no, NORMAL_SEARCH);
            slaveStates[thread_no] = NORMAL_SEARCH;
          }
          else{
            setState(thread_no, NORMAL_SEARCH);
          }
        }
        if (num_free_slaves == num_threads && job_queue.size() == 0 && status != RES_GUN && !so.purePortfolio) {
                for (int i = 0; i < search_progress.size(); i++) printf("%d ", search_progress[i]);
                assert(false && "No free slave, and no RES_GUN! ");
        }

        if (status != RES_GUN || true) receiveLearnts(r, thread_no, message_length);
        free(message);

}

void Master::setState(int thread_no, int state){
   MPI_Bsend(&state, 1, MPI_INT, thread_no+1, NEW_STATE_TAG, MPI_COMM_WORLD);
}

bool Master::updateProgress(int i) {
        search_progress.growTo(i+1,0);
        search_progress[i]++;
        for ( ; i > 0; i--) {
                if (search_progress[i]%2) return false;
                search_progress[i-1] += search_progress[i]/2;
                search_progress[i] = 0;
        }
        return (search_progress[0] == 1);
}

void Master::sendLearnts(int thread_no) {
        if (PAR_DEBUG) fprintf(stderr, "Sending learnts to %d\n", thread_no);

        vec<int> message(sizeof(Report)/sizeof(int),0);
        int num_learnts = 0;

        SClause *sc = (SClause*) &global_learnts[lhead[thread_no]];
        for ( ; (int*) sc != &global_learnts[global_learnts.size()]; sc = sc->getNext()) {
                if (sc->source == thread_no) continue;
                sc->pushInVec(message);
                num_learnts++;
        }
        lhead[thread_no] = global_learnts.size();

        Report& r = *((Report*) (int*) message);
        r.num_learnts = num_learnts;

        profile_start();

        MPI_Bsend((int*) message, message.size(), MPI_INT, thread_no+1, LEARNTS_TAG, MPI_COMM_WORLD);

        last_send_learnts[thread_no] = wallClockTime();

        profile_end("send learnts", message.size())
}

void Master::receiveLearnts(Report& r, int thread_no, int message_length) {
        if (FULL_DEBUG) fprintf(stderr, "Parsing learnts from %d\n", thread_no);
        SClause *sc = (SClause*) r.data;
        for (int i = 0; i < r.num_learnts; i++) {
                assert(sc->size > 0);
                if (sc->size == 1) {
                        sat.convertToClause(*sc);
                        Lit x = sat.out_learnt[0];
                        if (sat.value(x) == l_False) { status = RES_GUN; return; }
                        if (sat.value(x) == l_Undef) { 
                          sat.enqueue(x); 
                        }
                }
                
                shared++;
                shared_len += sc->size;
                sc->pushInVec(global_learnts);
                sc = sc->getNext();
        }

        if (global_learnts.size() > global_learnts.capacity()/2) {
                int first = global_learnts.size();
                for (int i = 0; i < num_threads; i++) if (lhead[i] < first) first = lhead[i];
                for (int i = 0; i < num_threads; i++) lhead[i] -= first;
                for (int i = first; i < global_learnts.size(); i++) global_learnts[i-first] = global_learnts[i];
                global_learnts.shrink(first);
                fprintf(stderr, "Shrink global learnts by %d\n", first);
        }

        if (FULL_DEBUG) fprintf(stderr, "Received learnts from %d\n", thread_no);
}

/*
 * Precondition: job_queue.size() > 0 && num_free_slaves > 0
 * */
bool Master::sendJob() {
        if (PAR_DEBUG) fprintf(stderr, "Finding free slave\n");

        profile_start();
        // Find empty worker
        int thread_no = -1;
        
        for(int i = 0 ; i < num_threads ; i++){
          if(job_start_time[i] == NOT_WORKING){
            thread_no = i;
            break;
          }
        }
        assert(thread_no >= 0 && "Did not find an idle worker?");   
        // Decide which job to send. 
        int job_num = -1;
        if(so.purePortfolio){
          if(slaveStates[thread_no] == NORMAL_SEARCH){
            job_num = job_queue.size() - 1;
          }
          else{
            // Create a job with random bound...
            if (createNewJob(thread_no, so.random_split))
              job_num = job_queue.size() - 1;
            else{
              setState(thread_no, NORMAL_SEARCH);
              slaveStates[thread_no] = NORMAL_SEARCH;
              slavesRunningGreedy--;
              job_num = selectJob(thread_no);
            }
          }
        }
        else{
          // Work stealing
          if(slaveStates[thread_no] == NORMAL_SEARCH){
            job_num = selectJob(thread_no);
          }
          else if(slaveStates[thread_no] == PORTFOLIO_SEARCH){
            job_queue.push(new SClause());
            job_num = job_queue.size() - 1;
          }
          else{
            // Create a new job with random bound
            if(createNewJob(thread_no, so.random_split))
              job_num = job_queue.size() - 1;
            else{
              // Okay, no more initialization... 
              setState(thread_no, NORMAL_SEARCH);
              slaveStates[thread_no] = NORMAL_SEARCH;
              slavesRunningGreedy--;
              job_num = selectJob(thread_no);
            }
          }
        }
                
        num_free_slaves--;
        job_start_time[thread_no] = wallClockTime();
        //if(!foundJob)
        //  job_num = selectJob(thread_no);
        assert(thread_no < num_threads);
        assert(thread_no >= 0);
        assert(job_num < job_queue.size());
        assert(job_num >= 0);
        if (cur_job[thread_no]) free(cur_job[thread_no]);
        cur_job[thread_no] = job_queue[job_num];
        job_queue[job_num] = job_queue.last();
        job_queue.pop();

        if (PAR_DEBUG) fprintf(stderr, "Sending job of size %d to %d , job_queue.size()=%d\n", 
              cur_job[thread_no]->size, 
              thread_no, 
              job_queue.size()
                                      );
       
        MPI_Request r;
        MPI_Isend((int*) cur_job[thread_no], cur_job[thread_no]->memSize(), MPI_INT, thread_no+1, JOB_TAG, MPI_COMM_WORLD, &r);
        MPI_Request_free(&r);

        if (PROFILING && job_queue.size() <= 4) fprintf(stderr, "Number of jobs left in queue: %d\n", job_queue.size());
        return true;
  
}

bool Master::createNewJob(int thread_no, bool random){
  if(NULL == engine.opt_var)
    return false;
  int lb = engine.opt_var->getMin() + (engine.opt_type ? 1 : 0);
  int ub = engine.opt_var->getMax() - (engine.opt_type ? 1 : 0);
  // val must be in (lb, ub). 
  int val; 
  if(lb == ub)
    val = lb;
  else 
    val = random? (lb + (rand() % (ub - lb))) 
        : (lb + (ub - lb) * ((double) (thread_no+1) / (num_threads+1)));
  
  if(PAR_DEBUG) fprintf(stderr, "Creating new job for %d, bound=%d from (%d, %d)\n", thread_no, val, lb, ub);
  if(lb + 1 >= ub)
    return false;
  Lit l = engine.opt_type ?  engine.opt_var->operator>=(val)
          : engine.opt_var->operator<=(val);
  vec<Lit> ls;
  ls.push(l);
  assert(sat.value(l) == l_Undef);
  Clause * c = Clause_new(ls, false);
  sat.convertToSClause(*c);
  job_queue.push(sat.temp_sc->copy());
  return true;
}
int Master::selectJob(int thread_no) {
        assert(job_queue.size());
        assert(!so.greedyInit ||  slaveStates[thread_no] == NORMAL_SEARCH);
        int best_job = -1;
        
        static int jobsSend = 0;
        jobsSend++;
        
        
        int longest_match = -1;
        for (int i = 0; i < job_queue.size(); i++) {
                int j, k = (job_queue.size()*thread_no/num_threads + i) % job_queue.size();
                if (cur_job[thread_no] == NULL) { best_job = k; break; }
                for (j = 0; j < job_queue[k]->memSize()-1 && j < cur_job[thread_no]->memSize()-1; j++) {
                        if (job_queue[k]->data[j] != cur_job[thread_no]->data[j]) break;
                }
                assert(j < cur_job[thread_no]->memSize() - 1);
                if (j > longest_match) {
                        longest_match = j;
                        best_job = k;
                }
        }
        if(PAR_DEBUG) fprintf(stderr, "Sending new job, longest_match=%d\n", longest_match);
        

        assert(best_job != -1);
        return best_job;
}



void Master::stealJobs() {
       
        int num_splits = 4-2*job_queue.size()/(num_threads-1);
        assert(num_splits > 0);
        //fprintf(stderr, "Check for busy slave, free slaves = %d\n", num_free_slaves);

        profile_start();

        int slave = -1;
        double cur_time = wallClockTime();
        double oldest_job = cur_time;
       
        if(nextSlaveToStealFrom >= 0 
          && job_start_time[nextSlaveToStealFrom] < cur_time
          && job_start_time[nextSlaveToStealFrom] != NOT_WORKING
           && (!so.greedyInit || slaveStates[nextSlaveToStealFrom] == NORMAL_SEARCH)){
          
          slave = nextSlaveToStealFrom;
          oldest_job = job_start_time[nextSlaveToStealFrom];
          nextSlaveToStealFrom = -1;
          }
        else{
          for (int i = 0; i < num_threads; i++) {
                  if (job_start_time[i] < oldest_job 
                    && (!so.greedyInit || slaveStates[i] == NORMAL_SEARCH)
                  ) {
                          slave = i;
                          oldest_job = job_start_time[i];
                  }
          }
        }
        if (oldest_job < cur_time-min_job_time) {
          assert(slaveStates[slave] == NORMAL_SEARCH);
                MPI_Bsend(&num_splits, 1, MPI_INT, slave+1, STEAL_TAG, MPI_COMM_WORLD);
                if (PAR_DEBUG) fprintf(stderr, "Steal request sent to %d to steal %d jobs\n", slave, num_splits);
                assert(job_start_time[slave] != NOT_WORKING);
                job_start_time_backup[slave] = job_start_time[slave];
                job_start_time[slave] = DONT_DISTURB;
        } 
        else{
          //fprintf(stderr, "Stole no job, oldest=%lf, cur=%lf, min_time=%lf\n", oldest_job, cur_time, min_job_time);
        }

        profile_end("steal job", 0);

//      fprintf(stderr, "Sent steal message to %d\n", slave);

}

void Master::receiveJobs() {
        int thread_no = s.MPI_SOURCE-1;
        static bool firstCall = true;
        MPI_Get_count(&s, MPI_INT, &message_length);
        message = (int*) malloc(message_length*sizeof(int));
        MPI_Recv(message, message_length, MPI_INT, s.MPI_SOURCE, SPLIT_TAG, MPI_COMM_WORLD, &s);

        if (PAR_DEBUG) fprintf(stderr, "Received %d jobs from %d\n", message[0], thread_no);
        
        if(thread_no == nextSlaveToStealFrom)
          nextSlaveToStealFrom = -1;
        assert(job_start_time[thread_no] != NOT_WORKING);
        if(message[0] == 0)
          job_start_time[thread_no] = job_start_time_backup[thread_no];
        else
          job_start_time[thread_no] = wallClockTime();

        assert(message_length <= TEMP_SC_LEN);

        memcpy(sat.temp_sc, message + 1, (message_length-1) * sizeof(int));
       
        assert(cur_job[thread_no]->size + message[0] == sat.temp_sc->size);
        free(cur_job[thread_no]);
        cur_job[thread_no] = sat.temp_sc->copy();
        
        for (int i = 0; i < message[0]; i++) {
                //sat.convertToClause(*sat.temp_sc);
                sat.temp_sc->negateLast();
                //sat.convertToClause(*sat.temp_sc);
                job_queue.push(sat.temp_sc->copy());
                sat.temp_sc->pop();
        }
        

        free(message);

//      fprintf(stderr, "New curr job for %d is: ", thread_no);
//      for (int j = 0; j < cur_job[slave].size(); j++) fprintf(stderr, "%d ", cur_job[slave][j]); fprintf(stderr, "\n");

}


void Master::collectStats() {
        int64_t dummy = 0;
        MPI_Reduce(&dummy, &engine.conflicts, 1, MPI_LONG_LONG_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&dummy, &engine.propagations, 1, MPI_LONG_LONG_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&dummy, &engine.opt_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
}

void Master::printStats() {
        printf("%lld shared clauses\n", shared);
        printf("%.1f avg shared len\n", (double) shared_len / shared);
        printf("Num conflicts: %d\n", engine.conflicts);
        printf("Master: Time %d\n", engine.opt_time);
}

/*
void Master::printStats() {

        double total_time = wallClockTime()-start_time;
        double comm_time = total_time * num_threads - real_time;
        char result_string[20] = "RES_UNK";
        
        if (status == SAT) sprintf(result_string, "SATISFIABLE");
        if (status == RES_GUN) sprintf(result_string, "UNSATISFIABLE"); 

        fprintf(stderr, "===============================================================================\n");
        fprintf(stderr, "conflicts             : %-12lld   (%.0f /sec)\n", conflicts, conflicts/total_time);
        fprintf(stderr, "decisions             : %-12lld   (%.0f /sec)\n", decisions, decisions/total_time);
        fprintf(stderr, "propagations          : %-12lld   (%.0f /sec)\n", propagations, propagations/total_time);
        fprintf(stderr, "shared clauses        : %-12lld   (%.0f /sec)\n", shared, shared/total_time);
        fprintf(stderr, "Slave CPU time        : %g s\n", cpu_time);
        fprintf(stderr, "Master CPU time       : %g s\n", cpuTime());
        fprintf(stderr, "Slave comm time       : %g s\n", comm_time);
        fprintf(stderr, "Total time            : %g s\n", total_time);
        printf("%lld,%lld,%lld,%lld,%g,%g,%g,%g,%s\n", conflicts, decisions, propagations shared, cpu_time, cpuTime(), comm_time, total_time, result_string);

}
*/

void Master::updateExchangeLimits(){
  int mLength = so.num_threads + 1;
  int message[mLength];
  MPI_Recv(message, mLength, MPI_INT, s.MPI_SOURCE, IMPORT_LIMITS, MPI_COMM_WORLD, &s);
  // Update importLimits
  int maxImport = 0;
  int sender = message[0];
  for(int i = 0 ; i < so.num_threads ; i++){
    if(i != sender){
      maxImport = std::max(maxImport, message[i+1]);
    }
    importLimits[sender*so.num_threads + i] = message[i+1];
  }
  
  // Do exportLimits change?
  for(int from = 0 ; from < so.num_threads ; from++){
    int maxImport = 0;
    int minImport = 1000;
    for(int to = 0 ; to < so.num_threads ; to++){
      if(from != to){
        maxImport = std::max(importLimits[to*so.num_threads+from], maxImport);
        minImport = std::min(importLimits[to*so.num_threads+from], minImport);
      }
      
    }
    // ExportRule: maxImport + 2?
    if(exportLimits[from] != maxImport+2){
      //fprintf(stderr, "Changing export rule for %d: %d -> %d\n", from, exportLimits[from], maxImport+2);
      int newSize = maxImport+2;
      exportLimits[from] = newSize;
      MPI_Bsend(&newSize, 1, MPI_INT, from+1, NEW_EXPORT_LIMIT, MPI_COMM_WORLD);
    }
  }
}

#endif