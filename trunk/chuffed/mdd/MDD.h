#ifndef __MDD_H__
#define __MDD_H__

// #define USE_POOL
// #define SPLIT_CACHE
// #define USE_MAP
#include <cassert>

#ifdef USE_POOL
#include <boost/pool/pool_alloc.hpp>
#endif

#include <map>

#include <ext/hash_map>

#include <vector>

#include <chuffed/support/vec.h>

typedef unsigned int MDD;
// MDD node format: var dest_0 dest_1 dest_2 ... dest_{dom-1}
typedef unsigned int MDDNodeEl;
typedef MDDNodeEl* MDDNode;

typedef std::pair< unsigned int, MDD > edgepair;
#define MDDTRUE 1
#define MDDFALSE 0

#ifdef USE_POOL
typedef boost::pool_allocator< MDDNodeEl > NodePool;
#endif

extern unsigned int nodelen;

struct ltnode 
{
   bool operator()(const MDDNode a1, const MDDNode a2) const
   {
      assert( 0 ); // FIXME: out of date.
      unsigned int i;
      for( i = 0; i < nodelen && a1[i] == a2[i]; i++ ) /* NOOP */;

      return a1[i] < a2[i];
   }
};

struct eqnode
{
   bool operator()(const MDDNode a1, const MDDNode a2) const
   {
      if( a1[0] != a2[0] )
          return false;
      for( int i = a1[0]; i > 0; i-- )
      {
          if( a1[i] != a2[i] )
              return false;
      }
      return true;
   }
};

struct hashnode
{
   unsigned int operator()(const MDDNode a1) const
   {
      unsigned int hash = 5381;
      
      for(int i = a1[0]; i > 0; i--)
      {
         hash = ((hash << 5) + hash) + a1[i];
      }
      return (hash & 0x7FFFFFFF);
   }
};

#ifdef USE_MAP
typedef std::map<const MDDNode, int, ltnode> NodeCache;
#else
typedef __gnu_cxx::hash_map<const MDDNode, int, hashnode, eqnode> NodeCache;
#endif

class OpCache
{
public:
   enum { OP_AND, OP_OR };

   OpCache(unsigned int size);
   ~OpCache(void);
   
   unsigned int check(char op, unsigned int a, unsigned int b); // Returns UINT_MAX on failure.
   void insert(char op, unsigned int a, unsigned int b, unsigned int res);
   
   typedef struct {
      unsigned int hash;
      char op;
      unsigned int a;
      unsigned int b;
      unsigned int res;
   } cache_entry;

private:
   inline unsigned int hash(char op, unsigned int a, unsigned int b);
   
   // Implemented with sparse-array stuff. 
   unsigned int tablesz;

   unsigned int members;
   unsigned int* indices;
   cache_entry* entries;
};

class MDDTable
{
public:
   MDDTable(int nvars, int domain); // Assumes same sized domain. Fix later.
   ~MDDTable(void);
   
   const std::vector<MDDNode>& getNodes(void)
   {
       return nodes;
   }

   std::vector<int>& getStatus(void)
   {
       return status;
   }

   void print_node(MDD r);
   void print_nodes(void);    
   void print_mdd(MDD r);
   void print_mdd_tikz(MDD r);
#if 1
   int cache_sz(void)
   {
      return cache.size();
   }
#endif

   MDD insert(MDDNode*);

//   MDD tuple(std::vector<unsigned int>&);
   template <class T> MDD tuple(vec<T>&);


   MDD mdd_vareq(int var, int val);
   MDD mdd_varlt(int var, int val);
   MDD mdd_vargt(int var, int val);
   MDD mdd_case(int var, std::vector<edgepair>& cases);

   MDD mdd_or(MDD, MDD);
   MDD mdd_and(MDD, MDD);
   MDD mdd_not(MDD);
   MDD mdd_not(std::vector<unsigned int>&, MDD);

   MDD mdd_true(std::vector<unsigned int>&, int var);
private:
   inline MDDNode allocNode(int sz);
   inline void deallocNode(MDDNode node, int sz);
   

   int nvars;
   int domain; 

   OpCache opcache;
#ifdef USE_POOL      
   NodePool alloc;
#endif
#ifdef SPLIT_CACHE
   NodeCache* cache;
#else
   NodeCache cache;
#endif

   std::vector<MDDNode> nodes;
   std::vector<int> status;
};

#endif
