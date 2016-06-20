#include <vector>
// #include "Vec.h"
#include <chuffed/core/propagator.h>
#include <chuffed/globals/mddglobals.h>

#include <chuffed/mdd/MDD.h>
#include <chuffed/mdd/mdd_prop.h>

typedef struct {
    int state;
    int value;
    int dest;
} dfa_trans;


static void addMDDProp(vec<IntVar*>& x, MDDTable& tab, MDD m, bool simple=false);

static MDD fd_regular(MDDTable& tab, int dom, int n, int nstates, vec< vec<int> >& transition, vec<int>& accepts);
static MDD mdd_table(MDDTable& mddtab, int arity, vec<int>& doms, vec< vec<int> >& entries, bool is_pos);

static void addMDDProp(vec<IntVar*>& x, MDDTable& tab, MDD m, bool simple)
{
   vec<int> doms;
   vec< IntView<> > w;

   for (int i = 0; i < x.size(); i++)
   {
      doms.push(x[i]->getMax()+1);
      // assert( x[i]->getMin() == 0 );
   }
	
   for (int i = 0; i < x.size(); i++) x[i]->specialiseToEL();
   for (int i = 0; i < x.size(); i++) w.push(IntView<>(x[i],1,0));
   
   MDDTemplate* templ = new MDDTemplate(tab, m, doms);

   new MDDProp<0>(templ, w, simple); 
}

// x: Vars | q: # states | s: alphabet size | d[state,symbol] -> state | q0: start state | f: accepts
// States range from 1..q (0 is reserved as dead)
// 
void mdd_regular(vec<IntVar*>& x, int q, int s, vec<vec<int> >& d, int q0, vec<int>& f) {
//   NOT_SUPPORTED;
   MDDTable tab(x.size(),s); 
   MDD m( fd_regular(tab, s+1, x.size(),q+1, d, f) );  
   addMDDProp(x, tab, m);
}

void mdd_table(vec<IntVar*>& x, vec<vec<int> >& t) {
   vec<int> doms;
   
   int maxdom = 0; 
   for( int i = 0; i < x.size(); i++ )
   {
      // assert(x[i]->getMin() == 0);
      doms.push(x[i]->getMax()+1); 

      // Could also generate maxdom from tuples.
      if( (x[i]->getMax() + 1) > maxdom )
         maxdom = x[i]->getMax() + 1;
   }
   
   MDDTable tab(x.size(),maxdom); 
   
   // Assumes a positive table. 
   MDD m( mdd_table(tab, x.size(), doms, t, true) );  
   
//   tab.print_mdd_tikz(m);

   addMDDProp(x, tab, m);
}

//MDD mdd_table(MDDTable& mddtab, int arity, vec<int>& doms, vec< std::vector<unsigned int> >& entries, bool is_pos)
MDD mdd_table(MDDTable& mddtab, int arity, vec<int>& doms, vec< vec<int> >& entries, bool is_pos)
{
   assert( doms.size() == arity );
      
   MDD table = MDDFALSE;

   for( int i = 0; i < entries.size(); i++ )
   {
      table = mddtab.mdd_or(table, mddtab.tuple(entries[i]));
   }

   if( !is_pos )
   {

      std::vector<unsigned int> vdoms;
      for( int i = 0; i < doms.size(); i++ )
         vdoms.push_back(doms[i]);

//      mddtab.print_mdd_tikz(table);

      table = mddtab.mdd_not(vdoms,table);
      
   }

   return table;
}

MDD fd_regular(MDDTable& tab, int dom, int n, int nstates, vec< vec<int> >& transition, vec<int>& accepts)
{
   std::vector< std::vector<MDD> > states;
   for( int i = 0; i < nstates; i++ )
   {
      states.push_back(std::vector<MDD>());
      states[i].push_back(MDDFALSE);
   }

   for( int i = 0; i < accepts.size(); i++ )
   {
      states[accepts[i]-1][0] = MDDTRUE;
   }
   
   // Inefficient implementation. Should fix.
   int prevlevel = 0;
   for( int j = n-1; j >= 0; j-- )
   {
      for( int i = 0; i < nstates-1; i++ )
      {
#if 0
         MDD cstate( MDDFALSE );
         for( int k = 0; k < transition.size(); k++ )
         {
            if( transition[k].state == i )
               cstate = tab.mdd_or( cstate,
                   tab.mdd_and( tab.mdd_vareq(j,transition[k].value), states[transition[k].dest][prevlevel] ) );
         }
         states[i].push_back(cstate);
#else
         std::vector<edgepair> cases;
         for( int k = 0; k < transition[i].size(); k++ )
         {
           if( transition[i][k] > 0 )
               cases.push_back(edgepair(k+1,states[transition[i][k]-1][prevlevel]));
         }
         states[i].push_back(tab.mdd_case(j,cases));
#endif
      }
      prevlevel++;
   }
   
   MDD out( states[0][states[0].size()-1] );
   
//   std::cout << out << std::endl;
//   tab.print_mdd_tikz(out);
   return out;
}
