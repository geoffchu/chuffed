#include <ctime>
#include <cstring>
#include <chuffed/core/options.h>
#include <chuffed/core/engine.h>
#include <chuffed/core/sat.h>
#include <chuffed/vars/int-var.h>
#include <chuffed/parallel/parallel.h>

Slave slave;

#ifdef PARALLEL

Slave::Slave() :
                thread_no(-1)
        , check_freq       (0.005)
        , report_freq      (1)
        , status           (RES_UNK)
        , next_check       (0)
        , report_message   (sizeof(Report)/sizeof(int),0)
        , unitFound(false)
        , numSkipped(0)
        , numAdded(0)
{}

void Slave::solve() {

        thread_no = so.thread_no;

        srand(thread_no+1);
        
        
        clauseSizeLimits.resize(so.num_threads, so.maxClSz);
        goodClausesFrom.resize(so.num_threads, 0);
        numClausesFrom.resize(so.num_threads, 0);
        
        
        checks = rand()%int(report_freq/check_freq);

        MPI_Buffer_attach(malloc(MPI_BUFFER_SIZE), MPI_BUFFER_SIZE);

        if (FULL_DEBUG) fprintf(stderr, "Solving\n");
        sendReport();
        while (receiveJob()) {
                real_time -= wallClockTime();
//              cpu_time -= cpuTime();
                status = engine.search();
                real_time += wallClockTime();
//              cpu_time += cpuTime();
                sendReport();
        }

        sendStats();
}

bool Slave::receiveJob() {
        double t;

        profile_start();
        //fprintf(stderr, "Waiting for job!\n");
        fflush(stderr);
        if (FULL_DEBUG) fprintf(stderr, "%d: Waiting for job\n", thread_no);
        assert(!engine.stop_init_phase);
        while (true) {
                MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &s);
                if (s.MPI_TAG == FINISH_TAG) { profile_end("finish", 0); return false; }
                MPI_Get_count(&s, MPI_INT, &message_length);
                if (s.MPI_TAG == JOB_TAG) break;
                if (s.MPI_TAG == LEARNTS_TAG) { receiveLearnts(); continue; }
                assert(s.MPI_TAG != LEARNTS_TAG);
                if(s.MPI_TAG == SOLUTION_TAG){
                  receiveSolution();
                  continue;
                }
                if(s.MPI_TAG == INT_SOLUTION_TAG){
                  assert(false && "This version does not support forwarding solution! ");
                  continue;
                }
                
                if(s.MPI_TAG == STEAL_TAG){
                  //fprintf(stderr, "Received steal-message while waiting for job! \n");
                }
                else if(s.MPI_TAG == INTERRUPT_TAG){
                 // nothing... 
                }
                else if(s.MPI_TAG == NEW_EXPORT_LIMIT){
                  // Only 1 Integer: New length
                  int newLength;
                  MPI_Recv(&newLength, 1, MPI_INT, 0, NEW_EXPORT_LIMIT, MPI_COMM_WORLD, &s);
                  so.maxClSz = newLength;
                  continue;
                }
                else if(s.MPI_TAG == NEW_STATE_TAG){
                  int newState;
                  MPI_Recv(&newState, 1, MPI_INT, 0, NEW_STATE_TAG, MPI_COMM_WORLD, &s);
                  _state = newState;
                  MPI_Bsend(&_state, 1, MPI_INT, 0, SLAVE_NEW_STATE_TAG , MPI_COMM_WORLD);
                  continue;
                }
                else{
                  assert(false && "Received strange message while waiting for job!\n");
                }
                
                if(message_length == 1)
                  MPI_Recv(&message_length, message_length, MPI_INT, 0, s.MPI_TAG, MPI_COMM_WORLD, &s); 
                else{
                  int * stuff = (int*)malloc(message_length * sizeof(int));
                  MPI_Recv(stuff, message_length, MPI_INT, 0, s.MPI_TAG, MPI_COMM_WORLD, &s);
                  free(stuff);
                }
        }
        assert(message_length <= TEMP_SC_LEN);

        MPI_Recv((int*) sat.temp_sc, message_length, MPI_INT, 0, JOB_TAG, MPI_COMM_WORLD, &s);

        profile_end("receive job", message_length);
        //fprintf(stderr, "Got job!\n");
        //fflush(stderr);
        if (FULL_DEBUG) fprintf(stderr, "%d: Received job\n", thread_no);

        
        sat.btToLevel(0);
        int before = storedClauses.size();
        sat.convertToClause(*sat.temp_sc);
        if(storedClauses.size() != before){
          fprintf(stderr, "%d: some strange stuff happened here!\n", thread_no);
        }
        
        for (int i = 0; i < engine.assumptions.size(); i++) {
                sat.decVarUse(engine.assumptions[i]/2);
        }
        engine.assumptions.clear();
        
        for (int i = 0; i < sat.out_learnt.size(); i++) {
                engine.assumptions.push(toInt(sat.out_learnt[i]));
                sat.incVarUse(engine.assumptions.last()/2);
        }
        sat.btToLevel(0);
        status = RES_SEA;

        if (FULL_DEBUG) fprintf(stderr, "%d: Parsed job\n", thread_no);
        return true;
}

bool Slave::checkMessages() {
        //if (engine.decisionLevel() <= engine.assumptions.size()) return false;

        double t = wallClockTime();

        if (t <= next_check && engine.decisionLevel() > 0) return false;

        real_time += t;
//      cpu_time += cpuTime();

        int received;
        //fprintf(stderr, "%d: CheckMessages! \n", thread_no);
        while (true) {
                MPI_Iprobe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &received, &s);
                if (!received) break;
                switch (s.MPI_TAG) {
                        case INTERRUPT_TAG:
                                MPI_Recv(NULL, 0, MPI_INT, 0, INTERRUPT_TAG, MPI_COMM_WORLD, &s);
                                if (PAR_DEBUG) fprintf(stderr, "%d: Interrupted! %f\n", thread_no, wallClockTime());
                                real_time -= wallClockTime();
//                              cpu_time -= cpuTime();
                                return true;
                        case STEAL_TAG:
                                //fprintf(stderr, "%d asked to split job\n", thread_no);
                                splitJob();
                                break;
                        case LEARNTS_TAG:
                                //fprintf(stderr, "%d receives learnts\n", thread_no);
                                receiveLearnts();
                                
                                break;
                        case SOLUTION_TAG:
                                receiveSolution();
                          break;
                        case INT_SOLUTION_TAG:
                          assert(false && "This version does not support forwarding solution! ");
                          break;
                        case NEW_EXPORT_LIMIT:
                                // Only 1 Integer: New length
                                int newLength;
                                //fprintf(stderr, "Receiving new export limit! \n");
                                MPI_Recv(&newLength, 1, MPI_INT, 0, NEW_EXPORT_LIMIT, MPI_COMM_WORLD, &s);
                                so.maxClSz = newLength;
                                //fprintf(stderr, "Done, set to %d \n", newLength);
                          break;
                        case NEW_STATE_TAG:
                          int newState;
                          MPI_Recv(&newState, 1, MPI_INT, 0, NEW_STATE_TAG, MPI_COMM_WORLD, &s);
                          if(_state != newState){
                            if(_state == RUNNING_GREEDY)
                              engine.stop_init_phase = true;
                            _state = newState;
                            
                          }
                          MPI_Bsend(&_state, 1, MPI_INT, 0, SLAVE_NEW_STATE_TAG , MPI_COMM_WORLD);
                          break;
                        default:
                                fprintf(stderr, "assert false!\n");
                                assert(false);
                }
        }
        
        if ((++checks%int(report_freq/check_freq) == 0) 
              || unitFound){
                unitFound = false;
                sendReport();
        }

        t = wallClockTime();
        if(engine.decisionLevel() > 0)
          next_check = t + check_freq;

        real_time -= t;
        if(engine.decisionLevel() == 0){
          for(int i = 0 ; i < storedClauses.size() ; i++){
            assert(storedClauses[i].size() > 0);
            assert(storedClauses[i][0] == tmp_header[i]);
            
            // TODO: 
            vec<int> tmp;
            for(int j = 0 ; j < storedClauses[i].size() ; j++)
              tmp.push(storedClauses[i][j]);
            sat.convertToClause(tmp);
            sat.addLearnt();
          }
          storedClauses.clear();
          tmp_header.clear();
        }
        return false;
}
void Slave::exportBounds(){
   int lb = engine.opt_var->getMin();
   int ub = engine.opt_var->getMax();
   Lit p =  engine.opt_var->getLit(lb, 2);
   Lit p2 = engine.opt_var->getLit(ub, 3);
                vec<Lit> ps;
                ps.push(p);
                Clause *c = Clause_new(ps, true);
                shareClause(*c);
                ps.clear();
                ps.push(p2);
                Clause *c2 = Clause_new(ps, true);
                shareClause(*c2);
                unitFound=true;
                free(c);
                free(c2);
}
void Slave::sendObjective(int objVal){
  // REPORT_OPTIMUM_TAG
  MPI_Bsend(&objVal, 1, MPI_INT, 0, REPORT_OPTIMUM_TAG, MPI_COMM_WORLD);
}

void Slave::sendReport() {
        if (FULL_DEBUG) fprintf(stderr, "%d: Forming report\n", thread_no);
        static bool firstCall = true;
        if(firstCall){
            if(engine.decisionLevel() == 0 && so.shareBounds)
              exportBounds();
        }
        
        Report& r = *((Report*) (int*) report_message);
        r.status = status;

        if (FULL_DEBUG) fprintf(stderr, "%d: Sending report to master\n", thread_no);

        profile_start();

        MPI_Bsend((int*) report_message, report_message.size(), MPI_INT, 0, REPORT_TAG, MPI_COMM_WORLD);

        profile_end("send result", report_message.size());

        if (FULL_DEBUG) fprintf(stderr, "%d: Sent report to master\n", thread_no);

        report_message.clear();
        report_message.growTo(sizeof(Report)/sizeof(int),0);
        
        unitFound = false;
}

void Slave::receiveSolution(){
  MPI_Probe(0, SOLUTION_TAG, MPI_COMM_WORLD, &s);
  MPI_Get_count(&s, MPI_INT, &message_length);
  message = (int*) malloc(message_length*sizeof(int));
  MPI_Recv(message, message_length, MPI_INT, 0, SOLUTION_TAG, MPI_COMM_WORLD, &s);
  int size = message[0];
  int extra = message[1];
  int * pt = &message[2];
  int tmp_size = size;
  int tmp_extra = extra;
  bool lazyFound = false;
  storedSolutions.clear();
  storedSolutions.push(0);
  for(int i = 2 ; i < message_length ; i++)
    storedSolutions.push(message[i]);
  SClause * testSC = (SClause*) (int*)storedSolutions;
  testSC->size = size;
  testSC->extra = extra;
  free(message);
}

void Slave::splitJob() {
        vec<int> message;
        int num_splits;
        //fprintf(stderr, "%d: Split job called, assumptions.size()=%d, DL=%d!\n", thread_no, engine.assumptions.size(), engine.decisionLevel());
        profile_start();

        MPI_Recv(&num_splits, 1, MPI_INT, 0, STEAL_TAG, MPI_COMM_WORLD, &s);

        int max_splits = engine.decisionLevel() - engine.assumptions.size() - 1;
        if (num_splits > max_splits) num_splits = max_splits;
        if (num_splits < 0) num_splits = 0;
        
        if (FULL_DEBUG) fprintf(stderr, "%d: Splitting %d jobs\n", thread_no, num_splits);

        for (int i = 0; i < num_splits; i++) {
                engine.assumptions.push(toInt(sat.decLit(engine.assumptions.size()+1)));
                sat.incVarUse(engine.assumptions.last()/2);
        }
        assert(num_splits == 0 || engine.decisionLevel() > engine.assumptions.size());

        vec<Lit> ps;
        for (int i = 0; i < engine.assumptions.size(); i++) ps.push(toLit(engine.assumptions[i]));
        Clause *c = Clause_new(ps);
        
        sat.convertToSClause(*c);
        free(c);
        message.push(num_splits);
        sat.temp_sc->pushInVec(message);

        MPI_Bsend((int*) message, message.size(), MPI_INT, 0, SPLIT_TAG, MPI_COMM_WORLD);

        profile_end("send split job", message.size());

        if (FULL_DEBUG) fprintf(stderr, "%d: Sent %d split job to master\n", thread_no, message[0]);
}

void Slave::receiveLearnts() {
        if (PAR_DEBUG) fprintf(stderr, "%d: Adding foreign clauses, current level = %d\n", thread_no, sat.decisionLevel());

        profile_start();

        MPI_Probe(0, LEARNTS_TAG, MPI_COMM_WORLD, &s);
        MPI_Get_count(&s, MPI_INT, &message_length);
        message = (int*) malloc(message_length*sizeof(int));
        MPI_Recv(message, message_length, MPI_INT, 0, LEARNTS_TAG, MPI_COMM_WORLD, &s);
        Report& r = *((Report*) message);

        profile_end("receive learnts", message_length);

        profile_start();

        SClause *sc = (SClause*) r.data;
        
        for (int i = 0; i < r.num_learnts; i++) {
          std::vector<int> tmp;
          // Just copy first 32 bits of SClause...
          tmp.push_back(*(int*)sc);
          for(int j = 0 ; j < (sc->size+sc->extra) ; j++){
            tmp.push_back(sc->data[j]);
          }
          storedClauses.push_back(tmp);
          tmp_header.push_back(*(int*)sc); //(sc->size << 16) | (sc->extra << 8) | sc->source);
          sc = sc->getNext();
        }
        
        profile_end("processing learnts", message_length);

        if (PAR_DEBUG) fprintf(stderr, "%d: Added foreign clauses, new level = %d\n", thread_no, sat.decisionLevel());

        free(message);

}

bool SAT::convertToSClause(Clause& c) {
        assert(c.size() <= TEMP_SC_LEN/2);
        temp_sc->size = c.size();
        temp_sc->extra = 0;
        temp_sc->source = so.thread_no;
        int j = 0;
        // TODO: DO I need this? (=> do I want to send/receive clauses?)
        /*
         * Different kind of literals: 
         *      - Boolean literal (purely boolean)
         *      - Lazy Literal for integers
         *      - Eager Literal for Integers
         * */
        //fprintf(stderr, "ConvertToSC called! \n");
        //fflush(stderr);
        for (int i = 0; i < c.size(); i++) {
                int type = c_info[var(c[i])].cons_type;
                if(type == 2)
                  printf("hmm, prop-lit? \n");
                assert(type != 2);
                int vid = c_info[var(c[i])].cons_id;
                // TODO: 1. Feld: Name der (Int-LL)-Variablen. 
                // Feld 2: Vorzeichen (1 Bit), Art (1 Bit), Wert (30 Bit)?
                // ChannelInfo: - Val_type <-> Gleichung/Ungleichung
                //              - Val: 
                if (type == 1 && vid != -1 && engine.vars[vid]->getType() == INT_VAR_LL) {
                        int var_field = (0x8000000 + (vid<<2) + (3*c_info[var(c[i])].val_type))^sign(c[i]);
                        temp_sc->data[j++] = var_field;
                        temp_sc->data[j++] = c_info[var(c[i])].val;
                        temp_sc->extra++;
                } else {
                        // Only a boolean literal. Okay as 
                        //      either this is a boolean variable, 
                        //      or it was encoded the same way for every process
                        //)
                        temp_sc->data[j++] = toInt(c[i]);
                }
        }
        return true;
}

void SAT::convertToClause(vec<int> & in){
  int * ptr = (int*)in;
  SClause * sc = (SClause*)ptr;
  int header = in[0];
  int size = header & 0xFFFF;
  int extra = (header>>16)&0xFF;
  int src = (header>>24) & 0xFF;
  convertToClause(*sc);
}

void SAT::convertToClause(SClause& sc) {
        out_learnt.clear();
        int *pt = sc.data;
        for (int i = 0; i < sc.size; i++) {
          
                if (0x8000000 & *pt) {
                        int index = 0x7ffffff & *pt;
                        int var_index = (0x7ffffff & *pt) >> 2;
                        assert(index >= 0);
                        assert(var_index < engine.vars.size() && "Index is too high!");
                        int constraint_type = *pt & 0x3;
                        IntVar *v = engine.vars[var_index];
                        assert(v->getType() == INT_VAR_LL);
                        
                        int value = pt[1];
                        Lit p = ((IntVarLL*) v)->getLit((constraint_type == 2 ? (value + 1) : value), constraint_type);
                        out_learnt.push(p);
                        pt += 2;
                } else {
                        out_learnt.push(toLit(*pt));
                        pt += 1;
                }
        }
}


/*
 * Problems: 
 *      Running time for "level": O(stacksize)?
 *      Value: Is this defined, if I add a literal lazily just before adding the clause?
 *      Add clauses which do NOT contain lazy literals? Where, and when?
 */

void SAT::addLearnt(){
  addLearnt(out_learnt);
}

void SAT::addLearnt(vec<Lit> & toAdd) {
  assert(decisionLevel() == 0);
  vec<Lit> newC;
  for(int j = 0 ; j < toAdd.size() ; j++){
    if(value(toAdd[j]) == l_True){ // Satisfied at DL0, skip this clause!
      return;
    }
    else if(value(toAdd[j]) == l_Undef){ 
      newC.push(toAdd[j]);
    }
    // Otherwise, this lit is false at DL0, remove it
  }
  
  slave.numAdded++;
  
  if(newC.size() == 0){
    printf("Got UNSAT! \n");
    engine.status = RES_GUN;
    return;
  }
  
  Clause * r = Clause_new(newC, true);
  r->age = 1;
  r->received = true;
  r->activity() = cla_inc;
  addClause(*r, so.one_watch);
  if(r->size() == 1)
    enqueue(newC[0], r);
}


void Slave::sendStats() {
        MPI_Reduce(&engine.conflicts, NULL, 1, MPI_LONG_LONG_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&engine.propagations, NULL, 1, MPI_LONG_LONG_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&engine.opt_time, NULL, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

        if (PROFILING) fprintf(stderr, "%d: Real time spent searching = %f\n", thread_no, real_time);
        if (PROFILING) fprintf(stderr, "%d: CPU time spent searching = %f\n", thread_no, cpu_time);
}

//--------
// Minor methods

void Slave::shareClause(Clause& c) {
        // TODO: REMOVE THIS...
        //return;
        if(c.size() == 1){
          unitFound=true;
        }
        sat.convertToSClause(c);
        sat.temp_sc->pushInVec(report_message);
        ((Report*) (int*) report_message)->num_learnts++;
}

#endif
