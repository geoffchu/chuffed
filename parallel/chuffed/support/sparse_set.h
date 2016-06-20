// U indicates whether to use FAST_FSET.

template<int U = 0>
class SparseSet {

public:
   SparseSet(unsigned int size) : dom(size), sparse(new unsigned int[size]), dense(new unsigned int[size]), members( 0 )
   {
      if( U&1 )
      {
         assert( members == 0 );
         for( unsigned int i = 0; i < dom; i++ )
         {
             sparse[i] = i;
             dense[i] = i;
         }
      }
   }

   ~SparseSet() {
      delete[] sparse;
      delete[] dense;  
   }

   bool elem(unsigned int value) {
      if( U&1 )
      {
         return (sparse[value] < members);
      } else {
         unsigned int a = sparse[value];

         if( a < members && dense[a] == value )
            return true;
         return false;
      }
   }
   
   bool elemLim(unsigned int lim, unsigned int el)
   {
      return (sparse[el] < lim) && elem(el);
   }

   virtual bool insert(unsigned int value)
   {
      if( U&1 )
      {
         unsigned int old_dense = sparse[value];
         unsigned int lost_val = dense[members];

         sparse[value] = members;
         dense[members] = value;

         sparse[lost_val] = old_dense;
         dense[old_dense] = lost_val;
      } else {
        assert( !elem(value) );

        sparse[value] = members;
        dense[members] = value;
      }
      members++;
      
      return true;
   }
   
   void clear(void) {
      members = 0;
   }

   unsigned int pos(unsigned int val)
   {
      assert( elem(val) );
      return sparse[val];
   }
    
   unsigned int operator [] (unsigned int index) {
      assert(index < members);
      return dense[index];
   }

   unsigned int size(void) {
      return members;
   }
       
protected:
   unsigned int dom;
   unsigned int* sparse;
   unsigned int* dense;
   unsigned int members;
};

#if 0
class TimedSet : public SparseSet {
public:
   TimedSet(int);

   void step_time(void);
   void unstep_time(void);
   void set_time(int);
   void clear_to_time(int);
   
   int get_time(void);

protected:
   vec<int> levels;
   int current_time;
}
#endif

// template< int U = 0 >
class TrailedSet : public SparseSet<0> {
public:
   TrailedSet(int sz) : SparseSet<0>( sz )
   {

   }

   bool insert(int value)
   {
       // Assumes not FFSET.
#if 0
      if( U&1 )
      {
         int old_dense = sparse[value];
         int lost_val = dense[members];

         sparse[value] = members;
         dense[members] = value;

         sparse[lost_val] = old_dense;
         dense[old_dense] = lost_val;
      } else {
#endif
        assert( !elem(value) );

        sparse[value] = members;
        dense[members] = value;
#if 0
      }
#endif
      trailChange(members,members+1);
      
      return true;
   }
};
