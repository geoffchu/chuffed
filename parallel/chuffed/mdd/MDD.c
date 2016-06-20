#include <iostream>

#ifdef __APPLE__
#include <memory>
#define MEMCPY std::memcpy
#else
#include <cstring>
#define MEMCPY memcpy
#endif

#include <cassert>
#include <climits>
#include <iostream>
#include <chuffed/mdd/MDD.h>

// static int collisions;

//inline MDDNode MDDTable::allocNode(int sz)
MDDNode MDDTable::allocNode(int sz)
{
#ifdef USE_POOL
   return alloc.allocate(sz);
#else
   return new MDDNodeEl[sz];
#endif
}

inline void MDDTable::deallocNode(MDDNode node, int sz)
//void MDDTable::deallocNode(MDDNode node, int sz)
{
//   return;
#ifdef USE_POOL
   alloc.deallocate(node,sz);
#else
   delete [] node;
#endif
}

MDDTable::MDDTable(int _nvars, int _domain)
   : nvars( _nvars ), domain( _domain ),
     opcache( OpCache(100000) ),
#ifdef USE_POOL
      alloc(),
#endif
#ifdef SPLIT_CACHE
      cache(new NodeCache[nvars]),
#else
//      cache(180000),
      cache(),
#endif
      nodes()
{
   // Initialize \ttt and \fff.
   nodes.push_back(NULL); // false node
   nodes.push_back(NULL); // true node
   status.push_back(0);
   status.push_back(0);
}

MDDTable::~MDDTable(void)
{
   for( unsigned int i = 2; i < nodes.size(); i++ )
      deallocNode(nodes[i],2*(domain+1));

#ifdef SPLIT_CACHE
   delete [] cache;
#endif
}

MDD MDDTable::insert(MDDNode* node)
{
#ifdef SPLIT_CACHE
//   NodeCache& varcache( cache[node[0]] );
   NodeCache& varcache( cache[(*node)[0]] );
#else
   NodeCache& varcache( cache );
#endif
   
//   assert( node[0] < 2*(domain+1) );
//   assert( (*node)[0] < 2*(domain+1) );

   if( (*node)[0] < 2 )
      return MDDFALSE;
   
   NodeCache::iterator res = varcache.find(*node);

   if( res != varcache.end() )
      return (*res).second;

#if 0   
   MDDNode act = allocNode(2*(domain+1));

   MEMCPY(act,node,sizeof(MDDNodeEl)*(node[0]+1));
//   MEMCPY(act,node,sizeof(MDDNodeEl)*(2*(domain+1)));

   varcache[act] = nodes.size();
   nodes.push_back(act);
#else
   varcache[*node] = nodes.size();
   nodes.push_back(*node);
   status.push_back(0);
   *node = allocNode(2*(domain+1));
#endif

   return nodes.size() - 1;
}

template <class T>
MDD MDDTable::tuple(vec<T>& tpl)
{
   MDD res = MDDTRUE;
   
   for( int i = tpl.size() -1; i >= 0; i-- )
   {
      if( ((unsigned int)tpl[i]) >= ((unsigned int) domain) )
         return MDDFALSE;
   }

   MDDNode tempNode = allocNode(2*(domain+1));
//   tempNode[0] = 3; // var, one edge.

   for( int i = tpl.size() - 1; i >= 0; i-- )
   {
      assert( ((unsigned int)tpl[i]) < ((unsigned int) domain) );

      tempNode[0] = 3; // var, one edge.
      tempNode[1] = i; // var
      tempNode[2] = tpl[i];
      tempNode[3] = res;

      res = insert(&tempNode);
   }
   
   deallocNode(tempNode,2*(domain+1));

   return res;
}

template MDD MDDTable::tuple(vec<int>& tpl);

MDD MDDTable::mdd_vareq(int var, int val)
{
   assert( var < nvars );
   assert( val < domain );
   
   MDDNode tempNode = allocNode(2*(domain+1));
   tempNode[0] = 3;
   tempNode[1] = var;
   tempNode[2] = val;
   tempNode[3] = MDDTRUE;
   
   MDD res = insert(&tempNode);
   deallocNode(tempNode,2*(domain+1));
   
   return res;      
}

MDD MDDTable::mdd_varlt(int var, int val)
{
   
   MDDNode tempNode = allocNode(2*(domain+1));
   tempNode[0] = 1 + 2*val;
   tempNode[1] = var;
   
   for( int i = 0; i < val; i++ )
   {
      tempNode[2 + 2*i] = i;
      tempNode[3 + 2*i] = MDDTRUE;
   }
   MDD res = insert(&tempNode);
   deallocNode(tempNode,2*(domain+1));
   
   return res;      
}

MDD MDDTable::mdd_vargt(int var, int val)
{
   
   MDDNode tempNode = allocNode(2*(domain+1));
   tempNode[1] = var;
   
   int j = 2; 
   for( int i = val+1; i < domain; i++ )
   {
      tempNode[j] = i;
      tempNode[j+1] = MDDTRUE;
      j += 2;
   }

   tempNode[0] = j-1;
   MDD res = insert(&tempNode);
   deallocNode(tempNode,2*(domain+1));
   
   return res;      
}


MDD MDDTable::mdd_case(int var, std::vector<edgepair>& cases)
{
   
   MDDNode tempNode = allocNode(2*(domain+1));
   
   tempNode[1] = var;
   
   int j = 2;
    
   for( unsigned int i = 0; i < cases.size(); i++ )
   {
      if( cases[i].second != MDDFALSE )
      {
         tempNode[j] = cases[i].first;
         tempNode[j+1] = cases[i].second;
         j += 2;
      }
   }
   tempNode[0] = j-1;
   
   MDD res = insert(&tempNode);
   deallocNode(tempNode,2*(domain+1));
   return res;   
}

MDD MDDTable::mdd_and(MDD a, MDD b)
{
   if( a == MDDFALSE || b == MDDFALSE )
      return MDDFALSE;
   if( a == MDDTRUE )
      return b;
   if( b == MDDTRUE )
      return a;

   MDD res;
   if( (res = opcache.check(OpCache::OP_AND,a,b)) != UINT_MAX )
       return res;
   
//   assert( nodes[a][1] == nodes[b][1] ); // Same var.


   MDDNode tempNode = allocNode(2*(domain+1));
   
   unsigned int j = 2;

   if( nodes[a][1] < nodes[b][1] )
   {
//      tempNode[0] = nodes[a][0];
      tempNode[1] = nodes[a][1];
      for( unsigned int i = 2; i < nodes[a][0]; i+=2 )
      {
         res = mdd_and(nodes[a][i+1],b);
         if( res )
         {
            tempNode[j] = nodes[a][i];
            tempNode[j+1] = res;
            j += 2;
         }
      }
   } else if( nodes[a][1] > nodes[b][1] ) {
//      tempNode[0] = nodes[b][0];
      tempNode[1] = nodes[b][1];
      for( unsigned int i = 2; i < nodes[b][0]; i+=2 )
      {
         res = mdd_and(a,nodes[b][i+1]);
         if( res )
         {
            tempNode[j] = nodes[b][i];
            tempNode[j+1] = res;
            j += 2;
         }
      }
   } else {
      tempNode[1] = nodes[a][1]; // Assign var.

      unsigned int ia = 2;
      MDDNodeEl na = nodes[a][ia];
      unsigned int ib = 2;
      MDDNodeEl nb = nodes[b][ib];
      while( ia < nodes[a][0] && ib < nodes[b][0] )
      {
         if( na < nb )
         {
            tempNode[j] = na;
            tempNode[j+1] = nodes[a][ia+1];

            j += 2;
            ia += 2;
            na = nodes[a][ia];

         } else if( nb < na ) {
            tempNode[j] = nb;
            tempNode[j+1] = nodes[b][ib+1];

            j += 2;
            ib += 2;
            nb = nodes[b][ib];
         } else {
            res = mdd_and(nodes[a][ia+1],nodes[b][ib+1]);
            if( res )
            {
               tempNode[j] = na; // na == nb
               tempNode[j+1] = res;
               j += 2;
               ia += 2;
               na = nodes[a][ia];
               ib += 2;
               nb = nodes[b][ib];
            }
         }
      }

      while( ia < nodes[a][0] )
      {
         tempNode[j] = na;
         tempNode[j+1] = nodes[a][ia+1];
         j += 2;
         ia += 2;
         na = nodes[a][ia];
      }
      while( ib < nodes[b][0] )
      {
         tempNode[j] = nb;
         tempNode[j+1] = nodes[b][ib+1];
         j += 2;
         ib += 2;
         nb = nodes[b][ib];
      }
   }

   if( j > 2 )
   {
       tempNode[0] = j-1;
          
       res = insert(&tempNode);
   } else {
       std::cout << "!" << std::endl;
       res = MDDFALSE;
   }

   opcache.insert(OpCache::OP_AND,a,b,res);

   deallocNode(tempNode,2*(domain+1));
   return res;
}

MDD MDDTable::mdd_or(MDD a, MDD b)
{
   if( a == MDDTRUE || b == MDDTRUE )
      return MDDTRUE;
   if( a == MDDFALSE )
      return b;
   if( b == MDDFALSE )
      return a;
   
   MDD res;
   if( (res = opcache.check(OpCache::OP_OR,a,b)) != UINT_MAX )
       return res;

   assert( nodes[a][1] == nodes[b][1] );
   assert( ((int) nodes[a][0]) < 2*(domain+1) && ((int) nodes[b][0]) < 2*(domain+1) );
   MDDNode tempNode = allocNode(2*(domain+1));
   if( nodes[a][1] < nodes[b][1] )
   {
      tempNode[0] = nodes[a][0];
      for( unsigned int i = 2; i < tempNode[0]; i+=2 )
      {
         tempNode[i] = nodes[a][i];
         tempNode[i+1] = mdd_or(nodes[a][i+1],b);
      }
   } else if( nodes[a][1] > nodes[b][1] ) {
      tempNode[0] = nodes[b][0];
      for( unsigned int i = 2; i < tempNode[0]; i+=2 )
      {
         tempNode[i] = nodes[b][i];
         tempNode[i+1] = mdd_or(a,nodes[b][i+1]);
      }
   } else {
      tempNode[1] = nodes[a][1]; // Assign var.

      unsigned int ia = 2;
      MDDNodeEl na = nodes[a][ia];
      unsigned int ib = 2;
      MDDNodeEl nb = nodes[b][ib];
      unsigned int j = 2;
      while( ia < nodes[a][0] && ib < nodes[b][0] )
      {
         if( na < nb )
         {
            tempNode[j] = na;
            tempNode[j+1] = nodes[a][ia+1];

            j += 2;
            ia += 2;
            na = nodes[a][ia];

         } else if( nb < na ) {
            tempNode[j] = nb;
            tempNode[j+1] = nodes[b][ib+1];

            j += 2;
            ib += 2;
            nb = nodes[b][ib];
         } else {
            tempNode[j] = na; // na == nb
            tempNode[j+1] = mdd_or(nodes[a][ia+1],nodes[b][ib+1]);
            j += 2;
            ia += 2;
            na = nodes[a][ia];
            ib += 2;
            nb = nodes[b][ib];
         }
      }
      while( ia < nodes[a][0] )
      {
         tempNode[j] = na;
         tempNode[j+1] = nodes[a][ia+1];
         j += 2;
         ia += 2;
         na = nodes[a][ia];
      }
      while( ib < nodes[b][0] )
      {
         tempNode[j] = nb;
         tempNode[j+1] = nodes[b][ib+1];
         j += 2;
         ib += 2;
         nb = nodes[b][ib];
      }
     
      tempNode[0] = j-1;
   }

   res = insert(&tempNode);
   opcache.insert(OpCache::OP_OR,a,b,res);

   deallocNode(tempNode,2*(domain+1));
   return res;
}

MDD MDDTable::mdd_true(std::vector<unsigned int>& doms, int var)
{
   MDD m = MDDTRUE;
   MDDNode tempNode = allocNode(2*(domain+1));

   for( int i = nvars-1; i >= var; i-- )
   {
      tempNode[0] = 2*doms[i] + 1;
      tempNode[1] = i;
      for( unsigned int j = 0; j < doms[i]; j++ )
      {
         tempNode[2*j + 2] = j;
         tempNode[2*j + 3] = m;
      }
      m = insert(&tempNode);

   }
   deallocNode(tempNode,2*(domain+1));
   return m;
}

MDD MDDTable::mdd_not(std::vector<unsigned int>& doms, MDD root)
{
   if( root == MDDTRUE )
      return MDDFALSE; 
   if( root == MDDFALSE )
      return MDDTRUE; // Will need to handle long edges.
   MDDNode tempNode = allocNode(2*(domain+1));

   MDDNode old(nodes[root]);

   unsigned int dom = doms[old[1]];
   assert( dom <= (unsigned int) domain );
   tempNode[1] = old[1]; // Var

   int j = 2;

   int klim = old[0];
   int k = 2;

   for( unsigned int i = 0; i < dom; i++ )
   {
      if( k < klim && old[k] == i )
      {
         if( old[k+1] != MDDTRUE )
         {
            MDD res = mdd_not(doms, old[k+1]);
            if( res )
            {
               tempNode[j] = i;
               tempNode[j+1] = res;
               j += 2;
            }
         }
         k += 2;
      } else {
         // Gap in the domain 
         tempNode[j] = i;
         tempNode[j+1] = mdd_true(doms,old[1]+1);
//         tempNode[j+1] = MDDTRUE;
         j += 2;
      }
   }
   tempNode[0] = j-1;

   MDD res( insert(&tempNode) );
   deallocNode(tempNode,2*(domain+1));
   return res;
}

void MDDTable::print_nodes(void)
{
#if 1
   for( unsigned int i = 2; i < nodes.size(); i++ )
   {
      print_node(i);
   }
#else
   std::cout << nodes.size() << std::endl;
#endif
}

void MDDTable::print_node(MDD r)
{
   std::cout << r << "(" << nodes[r][1] << "):";
   for( unsigned int j = 2; j < nodes[r][0]; j += 2 )
       std::cout << " (" << nodes[r][j] << "," << nodes[r][j+1] << ")";
   std::cout << std::endl;
}

void MDDTable::print_mdd(MDD r)
{
   std::vector<MDD> queued;
   queued.push_back(r);
   status[0] = 1;
   status[1] = 1;
   status[r] = 1;
   unsigned int head = 0;

   while( head < queued.size() )
   {
      MDD n = queued[head];

      print_node(n);
      for( unsigned int j = 2; j < nodes[n][0]; j += 2 )
      {
         if( status[nodes[n][j+1]] == 0 )
         {
            status[nodes[n][j+1]] = 1;
            queued.push_back(nodes[n][j+1]);
         }
      }
      head++;
   }
   for( unsigned int i = 0; i < queued.size(); i++ )
   {
       status[queued[i]] = 0;
   }
   status[0] = 0;
   status[1] = 0;
}

void MDDTable::print_mdd_tikz(MDD r)
{
   std::cout << "\\documentclass{article}\n";

   std::cout << "\\usepackage{tikz}\n";
   std::cout << "\\usetikzlibrary{arrows,shapes}\n";
   std::cout << "\\begin{document}\n";
   std::cout << "\\begin{tikzpicture}\n";
   std::cout << "\\tikzstyle{vertex}=[draw,circle,fill=black!25,minimum size=20pt,inner sep=0pt]\n";
   std::cout << "\\tikzstyle{smallvert}=[circle,fill=black!25,minimum size=5pt,inner sep=0pt]\n";
   std::cout << "\\tikzstyle{edge} = [draw,thick,->]\n";
   std::cout << "\\tikzstyle{kdedge} = [draw,thick,=>,color=red]\n";
   std::cout << "\\tikzstyle{kaedge} = [draw,thick,=>,color=blue]\n";
   std::cout << "\\tikzstyle{kbedge} = [draw,thick,=>,color=pinegreen!25]\n";
   
   std::vector<MDD> queued;
   queued.push_back(r);
   status[0] = 1;
   status[1] = 1;
   status[r] = 1;
   unsigned int head = 0;
   std::cout << "\\foreach \\pos/\\name/\\stat in {";
    
   bool first = true;
   
   int off = 0;
   unsigned int var = 0; 
   while( head < queued.size() )
   {
      MDD n = queued[head];
      
      if(first)
      {
         first = false;
         std::cout << "{(0,0)/1/T}";
      }
      std::cout << ",";

      if( var != nodes[n][1] )
      {
         var = nodes[n][1];
         off = 0;
      }

      std::cout << "{(" << off << "," << 1.5*(nvars - nodes[n][1]) << ")/" << n << "/" << nodes[n][1] << "}";
      off += 2;

      for( unsigned int j = 2; j < nodes[n][0]; j += 2 )
      {
         if( status[nodes[n][j+1]] == 0 )
         {
            status[nodes[n][j+1]] = 1;
            queued.push_back(nodes[n][j+1]);
         }
      }
      head++;
   }
   std::cout << "}\n\t\t\\node[vertex] (\\name) at \\pos {$x_{\\stat}$};\n";

   std::cout << "\\foreach \\source/\\dest/\\label in {";
   
   first = true;
   for( unsigned int i = 0; i < queued.size(); i++ )
   {
      MDD n = queued[i];


      for( unsigned int j = 2; j < nodes[n][0]; j += 2 )
      {
         if(first)
         {
            first = false;
         } else {
            std::cout << ",";
         }

         std::cout << "{" << n << "/" << nodes[n][j+1] << "/" << nodes[n][j] << "}" ;
      }
   }
   std::cout << "}\n\t\t\\path[edge] (\\source) -- node {$\\label$} (\\dest);\n";

   std::cout << "\\end{tikzpicture}\n";
   std::cout << "\\end{document}\n";
}


OpCache::OpCache(unsigned int  sz)
   : tablesz( sz ), members( 0 ),
     indices( (unsigned int*) malloc(sizeof(unsigned int)*sz) ),
     entries( (cache_entry*) malloc(sizeof(cache_entry)*sz) )
{
//    collisions = 0;
}

OpCache::~OpCache(void)
{
   free(indices);
   free(entries);
//    std::cout << members << ", " << collisions << std::endl;
}


inline unsigned int OpCache::hash(char op, unsigned int a, unsigned int b)
{
   unsigned int hash = 5381;
   
   hash = ((hash << 5) + hash) + op;
   hash = ((hash << 5) + hash) + a;
   hash = ((hash << 5) + hash) + b;

   return (hash & 0x7FFFFFFF) % tablesz;
}

// Returns UINT_MAX on failure.
unsigned int OpCache::check(char op, unsigned int a, unsigned int b)
{
   unsigned int hval = hash(op,a,b);
   unsigned int index = indices[hval];

   if( index < members && entries[index].hash == hval )
   {
      // Something is in the table.
      if( entries[index].op == op && entries[index].a == a && entries[index].b == b )
      {
         return entries[index].res;
      }
   }
   return UINT_MAX;
}

void OpCache::insert(char op, unsigned int a, unsigned int b, unsigned int res)
{
   unsigned int hval = hash(op,a,b);
   unsigned int index = indices[hval];

   if( index >= members || entries[index].hash != hval )
   {
      index = members;
      indices[hval] = index;
      members++;
   }
//   else {
//      collisions++;  
//   }
   
   entries[index].hash = hval; 
   entries[index].op = op; 
   entries[index].a = a;
   entries[index].b = b;
   entries[index].res = res;
}
