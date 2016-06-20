
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 25 "flatzinc/parser.yxx"

#define YYPARSE_PARAM parm
#define YYLEX_PARAM static_cast<ParserState*>(parm)->yyscanner

#include <iostream>
#include <fstream>
#include <sstream>

#include "flatzinc/flatzinc.h"
#include "flatzinc/parser.tab.h"

#ifdef HAVE_MMAP
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif

using namespace std;

int yyparse(void*);
int yylex(YYSTYPE*, void* scanner);
int yylex_init (void** scanner);
int yylex_destroy (void* scanner);
int yyget_lineno (void* scanner);
void yyset_extra (void* user_defined ,void* yyscanner );

extern int yydebug;

using namespace FlatZinc;

void yyerror(void* parm, const char *str) {
  ParserState* pp = static_cast<ParserState*>(parm);
  pp->err << "Error: " << str
          << " in line no. " << yyget_lineno(pp->yyscanner)
          << std::endl;
  pp->hadError = true;
}

void yyassert(ParserState* pp, bool cond, const char* str)
{
  if (!cond) {
    pp->err << "Error: " << str
            << " in line no. " << yyget_lineno(pp->yyscanner)
            << std::endl;
    pp->hadError = true;
  }
}

/*
 * The symbol tables
 *
 */

AST::Node* getArrayElement(ParserState* pp, string id, unsigned int offset) {
  if (offset > 0) {
    vector<int> tmp;
    if (pp->intvararrays.get(id, tmp) && offset<=tmp.size())
      return new AST::IntVar(tmp[offset-1]);
    if (pp->boolvararrays.get(id, tmp) && offset<=tmp.size())
      return new AST::BoolVar(tmp[offset-1]);
    if (pp->setvararrays.get(id, tmp) && offset<=tmp.size())
      return new AST::SetVar(tmp[offset-1]);

    if (pp->intvalarrays.get(id, tmp) && offset<=tmp.size())
      return new AST::IntLit(tmp[offset-1]);
    if (pp->boolvalarrays.get(id, tmp) && offset<=tmp.size())
      return new AST::BoolLit(tmp[offset-1]);
    vector<AST::SetLit> tmpS;
    if (pp->setvalarrays.get(id, tmpS) && offset<=tmpS.size())
      return new AST::SetLit(tmpS[offset-1]);    
  }

  pp->err << "Error: array access to " << id << " invalid"
          << " in line no. "
          << yyget_lineno(pp->yyscanner) << std::endl;
  pp->hadError = true;
  return new AST::IntVar(0); // keep things consistent
}
AST::Node* getVarRefArg(ParserState* pp, string id, bool annotation = false) {
  int tmp;
  if (pp->intvarTable.get(id, tmp))
    return new AST::IntVar(tmp);
  if (pp->boolvarTable.get(id, tmp))
    return new AST::BoolVar(tmp);
  if (pp->setvarTable.get(id, tmp))
    return new AST::SetVar(tmp);
  if (annotation)
    return new AST::Atom(id);
  pp->err << "Error: undefined variable " << id
          << " in line no. "
          << yyget_lineno(pp->yyscanner) << std::endl;
  pp->hadError = true;
  return new AST::IntVar(0); // keep things consistent
}

void addDomainConstraint(ParserState* pp, std::string id, AST::Node* var,
                         Option<AST::SetLit* >& dom) {
  if (!dom())
    return;
  AST::Array* args = new AST::Array(2);
  args->a[0] = var;
  args->a[1] = dom.some();
  pp->domainConstraints.push_back(new ConExpr(id, args));
}

/*
 * Initialize the root gecode space
 *
 */

void initfg(ParserState* pp) {
#if EXPOSE_INT_LITS
  static struct {
    const char *int_CMP_reif;
    IntRelType irt;
  } int_CMP_table[] = {
    { "int_eq_reif", IRT_EQ },
    { "int_ne_reif", IRT_NE },
    { "int_ge_reif", IRT_GE },
    { "int_gt_reif", IRT_GT },
    { "int_le_reif", IRT_LE },
    { "int_lt_reif", IRT_LT }
  };

  for (int i = 0; i < static_cast<int>(pp->domainConstraints2.size()); ) {
    ConExpr& c = *pp->domainConstraints2[i].first;
    for (int j = 0; j < 6; ++j)
      if (c.id.compare(int_CMP_table[j].int_CMP_reif) == 0) {
        if (!c[2]->isBoolVar())
          goto not_found;
        int k;
        for (k = c[2]->getBoolVar(); pp->boolvars[k].second->alias; k = pp->boolvars[k].second->i)
          ;
        BoolVarSpec& boolvar = *static_cast<BoolVarSpec *>(pp->boolvars[k].second);
        if (boolvar.alias_var >= 0)
          goto not_found;
        if (c[0]->isIntVar() && c[1]->isInt(boolvar.alias_val)) {
          boolvar.alias_var = c[0]->getIntVar();
          boolvar.alias_irt = int_CMP_table[j].irt;
          goto found;
        }
        if (c[1]->isIntVar() && c[0]->isInt(boolvar.alias_val)) {
          boolvar.alias_var = c[1]->getIntVar();
          boolvar.alias_irt = -int_CMP_table[j].irt;
          goto found;
        }
      }
  not_found:
    ++i;
    continue;
  found:
    delete pp->domainConstraints2[i].first;
    delete pp->domainConstraints2[i].second;
    pp->domainConstraints2.erase(pp->domainConstraints2.begin() + i);
  }
#endif

  if (!pp->hadError)
    pp->fg = new FlatZincSpace(pp->intvars.size(),
                                pp->boolvars.size(),
                                pp->setvars.size());

  for (unsigned int i=0; i<pp->intvars.size(); i++) {
 //fprintf(stderr, "v%d=%s\n", i, pp->intvars[i].first.c_str());
    if (!pp->hadError) {
      try {
        pp->fg->newIntVar(static_cast<IntVarSpec*>(pp->intvars[i].second));
      } catch (FlatZinc::Error& e) {
        yyerror(pp, e.toString().c_str());
      }
    }
    if (pp->intvars[i].first[0] != '[') {
      delete pp->intvars[i].second;
      pp->intvars[i].second = NULL;
    }
  }
  for (unsigned int i=0; i<pp->boolvars.size(); i++) {
    if (!pp->hadError) {
      try {
        pp->fg->newBoolVar(
          static_cast<BoolVarSpec*>(pp->boolvars[i].second));
      } catch (FlatZinc::Error& e) {
        yyerror(pp, e.toString().c_str());
      }
    }
    if (pp->boolvars[i].first[0] != '[') {
      delete pp->boolvars[i].second;
      pp->boolvars[i].second = NULL;
    }
  }
  for (unsigned int i=0; i<pp->setvars.size(); i++) {
    if (!pp->hadError) {
      try {
        pp->fg->newSetVar(static_cast<SetVarSpec*>(pp->setvars[i].second));
      } catch (FlatZinc::Error& e) {
        yyerror(pp, e.toString().c_str());
      }
    }      
    if (pp->setvars[i].first[0] != '[') {
      delete pp->setvars[i].second;
      pp->setvars[i].second = NULL;
    }
  }
  for (unsigned int i=pp->domainConstraints.size(); i--;) {
    if (!pp->hadError) {
      try {
        assert(pp->domainConstraints[i]->args->a.size() == 2);
        pp->fg->postConstraint(*pp->domainConstraints[i], NULL);
        delete pp->domainConstraints[i];
      } catch (FlatZinc::Error& e) {
        yyerror(pp, e.toString().c_str());        
      }
    }
  }
#if EXPOSE_INT_LITS
  for (int i = 0; i < static_cast<int>(pp->domainConstraints2.size()); ++i) {
    if (!pp->hadError) {
      try {
        pp->fg->postConstraint(*pp->domainConstraints2[i].first, pp->domainConstraints2[i].second);
        delete pp->domainConstraints2[i].first;
        delete pp->domainConstraints2[i].second;
      } catch (FlatZinc::Error& e) {
        yyerror(pp, e.toString().c_str());        
      }
    }
  }
#endif
}

AST::Node* arrayOutput(AST::Call* ann) {
  AST::Array* a = NULL;
  
  if (ann->args->isArray()) {
    a = ann->args->getArray();
  } else {
    a = new AST::Array(ann->args);
  }
  
  std::ostringstream oss;
  
  oss << "array" << a->a.size() << "d(";
  for (unsigned int i=0; i<a->a.size(); i++) {
    AST::SetLit* s = a->a[i]->getSet();
    if (s->empty())
      oss << "{}, ";
    else if (s->interval)
      oss << s->min << ".." << s->max << ", ";
    else {
      oss << "{";
      for (unsigned int j=0; j<s->s.size(); j++) {
        oss << s->s[j];
        if (j<s->s.size()-1)
          oss << ",";
      }
      oss << "}, ";
    }
  }

  if (!ann->args->isArray()) {
    a->a[0] = NULL;
    delete a;
  }
  return new AST::String(oss.str());
}

/*
 * The main program
 *
 */

namespace FlatZinc {

  void solve(const std::string& filename, std::ostream& err) {
#ifdef HAVE_MMAP
    int fd;
    char* data;
    struct stat sbuf;
    fd = open(filename.c_str(), O_RDONLY);
    if (fd == -1) {
      err << "Cannot open file " << filename << endl;
      exit(0);
    }
    if (stat(filename.c_str(), &sbuf) == -1) {
      err << "Cannot stat file " << filename << endl;
      return;      
    }
    data = (char*)mmap((caddr_t)0, sbuf.st_size, PROT_READ, MAP_SHARED, fd,0);
    if (data == (caddr_t)(-1)) {
      err << "Cannot mmap file " << filename << endl;
      return;      
    }

    ParserState pp(data, sbuf.st_size, err);
#else
    std::ifstream file;
    file.open(filename.c_str());
    if (!file.is_open()) {
      err << "Cannot open file " << filename << endl;
      exit(0);
    }
    std::string s = string(istreambuf_iterator<char>(file),
                           istreambuf_iterator<char>());
    ParserState pp(s, err);
#endif
    yylex_init(&pp.yyscanner);
    yyset_extra(&pp, pp.yyscanner);
    // yydebug = 1;
    yyparse(&pp);
		FlatZinc::s->output = pp.getOutput();
		FlatZinc::s->setOutput();
    
    if (pp.yyscanner)
      yylex_destroy(pp.yyscanner);
		if (pp.hadError) abort();
  }

  void solve(std::istream& is, std::ostream& err) {
    std::string s = string(istreambuf_iterator<char>(is),
                           istreambuf_iterator<char>());

    ParserState pp(s, err);
    yylex_init(&pp.yyscanner);
    yyset_extra(&pp, pp.yyscanner);
    // yydebug = 1;
    yyparse(&pp);
    FlatZinc::s->output = pp.getOutput();
    
    if (pp.yyscanner)
      yylex_destroy(pp.yyscanner);
		if (pp.hadError) abort();
  }

}



/* Line 189 of yacc.c  */
#line 414 "flatzinc/parser.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     INT_LIT = 258,
     BOOL_LIT = 259,
     FLOAT_LIT = 260,
     ID = 261,
     STRING_LIT = 262,
     VAR = 263,
     PAR = 264,
     ANNOTATION = 265,
     ANY = 266,
     ARRAY = 267,
     BOOL = 268,
     CASE = 269,
     COLONCOLON = 270,
     CONSTRAINT = 271,
     DEFAULT = 272,
     DOTDOT = 273,
     ELSE = 274,
     ELSEIF = 275,
     ENDIF = 276,
     ENUM = 277,
     FLOAT = 278,
     FUNCTION = 279,
     IF = 280,
     INCLUDE = 281,
     INT = 282,
     LET = 283,
     MAXIMIZE = 284,
     MINIMIZE = 285,
     OF = 286,
     SATISFY = 287,
     OUTPUT = 288,
     PREDICATE = 289,
     RECORD = 290,
     SET = 291,
     SHOW = 292,
     SHOWCOND = 293,
     SOLVE = 294,
     STRING = 295,
     TEST = 296,
     THEN = 297,
     TUPLE = 298,
     TYPE = 299,
     VARIANT_RECORD = 300,
     WHERE = 301
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 365 "flatzinc/parser.yxx"
 int iValue; char* sValue; bool bValue; double dValue;
         std::vector<int>* setValue;
         FlatZinc::AST::SetLit* setLit;
         std::vector<double>* floatSetValue;
         std::vector<FlatZinc::AST::SetLit>* setValueList;
         FlatZinc::Option<FlatZinc::AST::SetLit* > oSet;
         FlatZinc::VarSpec* varSpec;
         FlatZinc::Option<FlatZinc::AST::Node*> oArg;
         std::vector<FlatZinc::VarSpec*>* varSpecVec;
         FlatZinc::Option<std::vector<FlatZinc::VarSpec*>* > oVarSpecVec;
         FlatZinc::AST::Node* arg;
         FlatZinc::AST::Array* argVec;
       


/* Line 214 of yacc.c  */
#line 512 "flatzinc/parser.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 524 "flatzinc/parser.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  7
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   342

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  57
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  66
/* YYNRULES -- Number of rules.  */
#define YYNRULES  155
/* YYNRULES -- Number of states.  */
#define YYNSTATES  338

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   301

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      48,    49,     2,     2,    50,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    51,    47,
       2,    54,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    52,     2,    53,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    55,     2,    56,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     9,    10,    12,    15,    19,    20,    22,
      25,    29,    30,    32,    35,    39,    45,    46,    49,    51,
      55,    59,    66,    74,    77,    79,    81,    85,    87,    89,
      91,    95,    97,   101,   108,   115,   122,   131,   138,   145,
     154,   168,   182,   196,   212,   228,   244,   260,   278,   280,
     282,   287,   288,   291,   293,   297,   298,   300,   304,   306,
     308,   313,   314,   317,   319,   323,   327,   329,   331,   336,
     337,   340,   342,   346,   350,   352,   354,   359,   360,   363,
     365,   369,   373,   374,   377,   378,   381,   382,   385,   386,
     389,   396,   400,   407,   411,   416,   418,   422,   426,   428,
     433,   435,   440,   444,   448,   449,   452,   454,   458,   459,
     462,   464,   468,   469,   472,   474,   478,   479,   482,   484,
     488,   490,   494,   496,   500,   501,   504,   506,   508,   510,
     512,   514,   519,   520,   523,   525,   529,   531,   536,   538,
     540,   541,   543,   546,   550,   555,   557,   559,   563,   565,
     569,   571,   573,   575,   577,   579
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      58,     0,    -1,    59,    61,    63,    96,    47,    -1,    -1,
      60,    -1,    65,    47,    -1,    60,    65,    47,    -1,    -1,
      62,    -1,    73,    47,    -1,    62,    73,    47,    -1,    -1,
      64,    -1,    95,    47,    -1,    64,    95,    47,    -1,    34,
       6,    48,    66,    49,    -1,    -1,    67,    77,    -1,    68,
      -1,    67,    50,    68,    -1,    69,    51,     6,    -1,    12,
      52,    71,    53,    31,    70,    -1,    12,    52,    71,    53,
      31,     8,    70,    -1,     8,    70,    -1,    70,    -1,    97,
      -1,    36,    31,    97,    -1,    13,    -1,    23,    -1,    72,
      -1,    71,    50,    72,    -1,    27,    -1,     3,    18,     3,
      -1,     8,    97,    51,     6,   117,   111,    -1,     8,    98,
      51,     6,   117,   111,    -1,     8,    99,    51,     6,   117,
     111,    -1,     8,    36,    31,    97,    51,     6,   117,   111,
      -1,    97,    51,     6,   117,    54,   112,    -1,    13,    51,
       6,   117,    54,   112,    -1,    36,    31,    97,    51,     6,
     117,    54,   112,    -1,    12,    52,     3,    18,     3,    53,
      31,     8,    97,    51,     6,   117,    91,    -1,    12,    52,
       3,    18,     3,    53,    31,     8,    98,    51,     6,   117,
      92,    -1,    12,    52,     3,    18,     3,    53,    31,     8,
      99,    51,     6,   117,    93,    -1,    12,    52,     3,    18,
       3,    53,    31,     8,    36,    31,    97,    51,     6,   117,
      94,    -1,    12,    52,     3,    18,     3,    53,    31,    97,
      51,     6,   117,    54,    52,   101,    53,    -1,    12,    52,
       3,    18,     3,    53,    31,    13,    51,     6,   117,    54,
      52,   103,    53,    -1,    12,    52,     3,    18,     3,    53,
      31,    23,    51,     6,   117,    54,    52,   105,    53,    -1,
      12,    52,     3,    18,     3,    53,    31,    36,    31,    97,
      51,     6,   117,    54,    52,   107,    53,    -1,     3,    -1,
       6,    -1,     6,    52,     3,    53,    -1,    -1,    76,    77,
      -1,    74,    -1,    76,    50,    74,    -1,    -1,    50,    -1,
      52,    75,    53,    -1,     5,    -1,     6,    -1,     6,    52,
       3,    53,    -1,    -1,    81,    77,    -1,    79,    -1,    81,
      50,    79,    -1,    52,    80,    53,    -1,     4,    -1,     6,
      -1,     6,    52,     3,    53,    -1,    -1,    85,    77,    -1,
      83,    -1,    85,    50,    83,    -1,    52,    84,    53,    -1,
     100,    -1,     6,    -1,     6,    52,     3,    53,    -1,    -1,
      89,    77,    -1,    87,    -1,    89,    50,    87,    -1,    52,
      88,    53,    -1,    -1,    54,    78,    -1,    -1,    54,    86,
      -1,    -1,    54,    82,    -1,    -1,    54,    90,    -1,    16,
       6,    48,   109,    49,   117,    -1,    16,     6,   117,    -1,
      16,     6,    52,     3,    53,   117,    -1,    39,   117,    32,
      -1,    39,   117,   116,   115,    -1,    27,    -1,    55,   101,
      56,    -1,     3,    18,     3,    -1,    13,    -1,    55,   104,
      77,    56,    -1,    23,    -1,    55,   106,    77,    56,    -1,
      55,   101,    56,    -1,     3,    18,     3,    -1,    -1,   102,
      77,    -1,     3,    -1,   102,    50,     3,    -1,    -1,   104,
      77,    -1,     4,    -1,   104,    50,     4,    -1,    -1,   106,
      77,    -1,     5,    -1,   106,    50,     5,    -1,    -1,   108,
      77,    -1,   100,    -1,   108,    50,   100,    -1,   110,    -1,
     109,    50,   110,    -1,   112,    -1,    52,   113,    53,    -1,
      -1,    54,   112,    -1,     4,    -1,     3,    -1,     5,    -1,
     100,    -1,     6,    -1,     6,    52,   112,    53,    -1,    -1,
     114,    77,    -1,   112,    -1,   114,    50,   112,    -1,     6,
      -1,     6,    52,     3,    53,    -1,    30,    -1,    29,    -1,
      -1,   118,    -1,    15,   119,    -1,   118,    15,   119,    -1,
       6,    48,   120,    49,    -1,   121,    -1,   119,    -1,   120,
      50,   119,    -1,   122,    -1,    52,   120,    53,    -1,     4,
      -1,     3,    -1,     5,    -1,   100,    -1,     6,    -1,     6,
      52,   122,    53,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   464,   464,   466,   468,   471,   472,   476,   481,   489,
     490,   494,   499,   507,   508,   515,   517,   519,   522,   523,
     526,   529,   530,   531,   532,   535,   536,   537,   538,   541,
     542,   545,   546,   553,   582,   610,   615,   644,   668,   677,
     689,   748,   800,   807,   862,   875,   888,   895,   909,   913,
     928,   952,   953,   957,   959,   962,   962,   964,   968,   970,
     985,  1009,  1010,  1014,  1016,  1020,  1024,  1026,  1041,  1065,
    1066,  1070,  1072,  1075,  1078,  1080,  1095,  1119,  1120,  1124,
    1126,  1129,  1134,  1135,  1140,  1141,  1146,  1147,  1152,  1153,
    1157,  1175,  1196,  1218,  1226,  1243,  1245,  1247,  1253,  1255,
    1268,  1269,  1276,  1278,  1285,  1286,  1290,  1292,  1297,  1298,
    1302,  1304,  1309,  1310,  1314,  1316,  1321,  1322,  1326,  1328,
    1336,  1338,  1342,  1344,  1349,  1350,  1354,  1356,  1358,  1360,
    1362,  1411,  1425,  1426,  1430,  1432,  1440,  1451,  1473,  1474,
    1482,  1483,  1487,  1489,  1493,  1497,  1501,  1503,  1507,  1509,
    1513,  1515,  1517,  1519,  1521,  1564
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INT_LIT", "BOOL_LIT", "FLOAT_LIT", "ID",
  "STRING_LIT", "VAR", "PAR", "ANNOTATION", "ANY", "ARRAY", "BOOL", "CASE",
  "COLONCOLON", "CONSTRAINT", "DEFAULT", "DOTDOT", "ELSE", "ELSEIF",
  "ENDIF", "ENUM", "FLOAT", "FUNCTION", "IF", "INCLUDE", "INT", "LET",
  "MAXIMIZE", "MINIMIZE", "OF", "SATISFY", "OUTPUT", "PREDICATE", "RECORD",
  "SET", "SHOW", "SHOWCOND", "SOLVE", "STRING", "TEST", "THEN", "TUPLE",
  "TYPE", "VARIANT_RECORD", "WHERE", "';'", "'('", "')'", "','", "':'",
  "'['", "']'", "'='", "'{'", "'}'", "$accept", "model", "preddecl_items",
  "preddecl_items_head", "vardecl_items", "vardecl_items_head",
  "constraint_items", "constraint_items_head", "preddecl_item",
  "pred_arg_list", "pred_arg_list_head", "pred_arg", "pred_arg_type",
  "pred_arg_simple_type", "pred_array_init", "pred_array_init_arg",
  "vardecl_item", "int_init", "int_init_list", "int_init_list_head",
  "list_tail", "int_var_array_literal", "float_init", "float_init_list",
  "float_init_list_head", "float_var_array_literal", "bool_init",
  "bool_init_list", "bool_init_list_head", "bool_var_array_literal",
  "set_init", "set_init_list", "set_init_list_head",
  "set_var_array_literal", "vardecl_int_var_array_init",
  "vardecl_bool_var_array_init", "vardecl_float_var_array_init",
  "vardecl_set_var_array_init", "constraint_item", "solve_item",
  "int_ti_expr_tail", "bool_ti_expr_tail", "float_ti_expr_tail",
  "set_literal", "int_list", "int_list_head", "bool_list",
  "bool_list_head", "float_list", "float_list_head", "set_literal_list",
  "set_literal_list_head", "flat_expr_list", "flat_expr",
  "non_array_expr_opt", "non_array_expr", "non_array_expr_list",
  "non_array_expr_list_head", "solve_expr", "minmax", "annotations",
  "annotations_head", "annotation", "annotation_list", "annotation_expr",
  "ann_non_array_expr", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,    59,    40,    41,
      44,    58,    91,    93,    61,   123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    57,    58,    59,    59,    60,    60,    61,    61,    62,
      62,    63,    63,    64,    64,    65,    66,    66,    67,    67,
      68,    69,    69,    69,    69,    70,    70,    70,    70,    71,
      71,    72,    72,    73,    73,    73,    73,    73,    73,    73,
      73,    73,    73,    73,    73,    73,    73,    73,    74,    74,
      74,    75,    75,    76,    76,    77,    77,    78,    79,    79,
      79,    80,    80,    81,    81,    82,    83,    83,    83,    84,
      84,    85,    85,    86,    87,    87,    87,    88,    88,    89,
      89,    90,    91,    91,    92,    92,    93,    93,    94,    94,
      95,    95,    95,    96,    96,    97,    97,    97,    98,    98,
      99,    99,   100,   100,   101,   101,   102,   102,   103,   103,
     104,   104,   105,   105,   106,   106,   107,   107,   108,   108,
     109,   109,   110,   110,   111,   111,   112,   112,   112,   112,
     112,   112,   113,   113,   114,   114,   115,   115,   116,   116,
     117,   117,   118,   118,   119,   119,   120,   120,   121,   121,
     122,   122,   122,   122,   122,   122
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     5,     0,     1,     2,     3,     0,     1,     2,
       3,     0,     1,     2,     3,     5,     0,     2,     1,     3,
       3,     6,     7,     2,     1,     1,     3,     1,     1,     1,
       3,     1,     3,     6,     6,     6,     8,     6,     6,     8,
      13,    13,    13,    15,    15,    15,    15,    17,     1,     1,
       4,     0,     2,     1,     3,     0,     1,     3,     1,     1,
       4,     0,     2,     1,     3,     3,     1,     1,     4,     0,
       2,     1,     3,     3,     1,     1,     4,     0,     2,     1,
       3,     3,     0,     2,     0,     2,     0,     2,     0,     2,
       6,     3,     6,     3,     4,     1,     3,     3,     1,     4,
       1,     4,     3,     3,     0,     2,     1,     3,     0,     2,
       1,     3,     0,     2,     1,     3,     0,     2,     1,     3,
       1,     3,     1,     3,     0,     2,     1,     1,     1,     1,
       1,     4,     0,     2,     1,     3,     1,     4,     1,     1,
       0,     1,     2,     3,     4,     1,     1,     3,     1,     3,
       1,     1,     1,     1,     1,     4
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       3,     0,     0,     7,     4,     0,     0,     1,     0,     0,
       0,     0,    95,     0,   104,    11,     8,     0,     0,     0,
       5,    16,     0,    98,   100,     0,   104,     0,     0,     0,
       0,     0,     0,   106,     0,    55,     0,     0,    12,     0,
       0,     9,     0,     6,     0,     0,    27,    28,     0,     0,
      55,    18,     0,    24,    25,    97,     0,   110,   114,    55,
      55,     0,     0,     0,     0,   140,     0,    96,    56,   105,
     140,   140,     0,     0,    13,    10,   140,    23,     0,     0,
      15,    56,    17,     0,     0,    56,     0,    56,     0,   140,
     140,   140,     0,     0,     0,   141,     0,   107,     0,     0,
      91,     0,     2,    14,     0,     0,    31,     0,    29,    26,
      19,    20,     0,   111,    99,   115,   101,   124,   124,   124,
       0,   151,   150,   152,   154,     0,   104,   153,   142,   145,
     148,     0,     0,   140,   127,   126,   128,   130,   132,   129,
       0,   120,   122,     0,   139,   138,    93,     0,     0,     0,
       0,     0,   140,     0,    33,    34,    35,     0,     0,     0,
       0,   146,     0,     0,    38,   143,     0,     0,   134,     0,
      55,   140,     0,   140,   136,    94,    37,    32,    30,     0,
     124,   125,     0,   103,     0,   154,     0,     0,   149,   102,
       0,     0,   123,    56,   133,    90,   121,    92,     0,     0,
      21,    36,     0,     0,     0,     0,     0,   144,   155,   147,
      39,   131,   135,     0,    22,     0,     0,     0,     0,     0,
       0,     0,     0,   137,     0,     0,     0,     0,   140,   140,
       0,   140,     0,   140,   140,   140,     0,     0,     0,     0,
       0,    82,    84,    86,     0,     0,   140,     0,   140,     0,
      40,     0,    41,     0,    42,   108,   112,     0,   104,    88,
      51,    83,    69,    85,    61,    87,     0,    55,     0,    55,
       0,     0,     0,    43,    48,    49,    53,     0,    55,    66,
      67,    71,     0,    55,    58,    59,    63,     0,    55,    45,
     109,    46,   113,   116,    44,    77,    89,     0,    57,    56,
      52,     0,    73,    56,    70,     0,    65,    56,    62,     0,
     118,     0,    55,    75,    79,     0,    55,    74,     0,    54,
       0,    72,     0,    64,    47,    56,   117,     0,    81,    56,
      78,    50,    68,    60,   119,     0,    80,    76
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     3,     4,    15,    16,    37,    38,     5,    49,
      50,    51,    52,    53,   107,   108,    17,   276,   277,   278,
      69,   261,   286,   287,   288,   265,   281,   282,   283,   263,
     314,   315,   316,   296,   250,   252,   254,   273,    39,    72,
      54,    28,    29,   139,    34,    35,   266,    59,   268,    60,
     311,   312,   140,   141,   154,   142,   169,   170,   175,   147,
      94,    95,   161,   162,   129,   130
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -122
static const yytype_int16 yypact[] =
{
     -23,    11,    28,    44,   -23,     1,     5,  -122,    41,    98,
      10,    32,  -122,    58,    93,    78,    44,    66,    73,    70,
    -122,    37,   126,  -122,  -122,   118,   142,    88,   111,   112,
     161,   159,    27,  -122,   110,   117,   162,   130,    78,   128,
     133,  -122,   167,  -122,    99,   131,  -122,  -122,   153,   136,
     137,  -122,   135,  -122,  -122,  -122,    27,  -122,  -122,   138,
     140,   186,   187,   188,   177,   181,   146,  -122,   195,  -122,
      75,   181,   152,   157,  -122,  -122,   181,  -122,     9,    27,
    -122,    37,  -122,   194,   151,   202,   154,   203,   156,   181,
     181,   181,   204,   100,   160,   196,   207,  -122,   127,   206,
    -122,    56,  -122,  -122,   164,   197,  -122,    90,  -122,  -122,
    -122,  -122,   210,  -122,  -122,  -122,  -122,   168,   168,   168,
     171,   208,  -122,  -122,   -32,   100,    93,  -122,  -122,  -122,
    -122,    20,   100,   181,   208,  -122,  -122,   169,    20,  -122,
     -12,  -122,  -122,   172,  -122,  -122,  -122,   221,    20,   226,
       9,   199,   181,    20,  -122,  -122,  -122,   200,   229,   100,
     104,  -122,    91,   178,  -122,  -122,   182,    20,  -122,   184,
     190,   181,   127,   181,   189,  -122,  -122,  -122,  -122,    38,
     168,  -122,    64,  -122,   108,   191,   192,   100,  -122,  -122,
      20,   193,  -122,    20,  -122,  -122,  -122,  -122,   239,    99,
    -122,  -122,   115,   198,   201,   213,   205,  -122,  -122,  -122,
    -122,  -122,  -122,   211,  -122,   216,   209,   212,   214,   242,
     244,    27,   245,  -122,    27,   247,   248,   249,   181,   181,
     217,   181,   218,   181,   181,   181,   219,   220,   251,   222,
     252,   223,   224,   225,   215,   228,   181,   230,   181,   231,
    -122,   232,  -122,   233,  -122,   255,   256,   227,    93,   234,
      15,  -122,   144,  -122,   155,  -122,   236,   138,   237,   140,
     235,   238,   240,  -122,  -122,   241,  -122,   243,   250,  -122,
     246,  -122,   253,   254,  -122,   257,  -122,   258,   260,  -122,
    -122,  -122,  -122,    24,  -122,    60,  -122,   267,  -122,    15,
    -122,   268,  -122,   144,  -122,   269,  -122,   155,  -122,   208,
    -122,   259,   263,   262,  -122,   264,   265,  -122,   266,  -122,
     270,  -122,   271,  -122,  -122,    24,  -122,   272,  -122,    60,
    -122,  -122,  -122,  -122,  -122,   273,  -122,  -122
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -122,  -122,  -122,  -122,  -122,  -122,  -122,  -122,   282,  -122,
    -122,   261,  -122,   -43,  -122,   145,   278,    -2,  -122,  -122,
     -50,  -122,    -8,  -122,  -122,  -122,     0,  -122,  -122,  -122,
     -28,  -122,  -122,  -122,  -122,  -122,  -122,  -122,   280,  -122,
      -1,   103,   105,   -90,  -121,  -122,  -122,    47,  -122,    52,
    -122,  -122,  -122,   148,  -112,  -109,  -122,  -122,  -122,  -122,
     -57,  -122,   -89,   163,  -122,   165
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint16 yytable[] =
{
      82,    77,    18,   127,   128,   163,   155,   156,    27,    86,
      88,     1,   105,   100,   101,    18,   159,     6,   274,   104,
     160,   275,   164,   134,   135,   136,   137,   309,     7,   168,
       8,    66,   117,   118,   119,   127,   106,   171,   172,   176,
       8,     8,   127,   165,   181,    44,   199,     8,    20,    45,
      46,    46,     9,    21,    12,    84,    10,    11,   191,    22,
      47,    47,    30,   309,    12,    12,   313,     8,   201,   127,
     127,    12,   202,    48,    48,   126,   166,   203,   109,   126,
      13,   210,    14,    31,   212,   144,   145,   204,   146,    32,
      93,    12,    14,    14,    36,   180,    33,   127,   209,    14,
     205,     8,     8,   121,   122,   123,   124,   121,   122,   123,
     185,    23,    46,    41,   195,   126,   197,    43,     8,    14,
     194,    24,    47,    98,    42,    12,    12,    99,    23,    55,
     134,   135,   136,   137,    25,    48,   200,   271,    24,    61,
     150,   187,    12,   151,   188,    33,    57,    58,   279,    56,
     280,   215,   125,    26,    14,   126,   214,   207,   187,   126,
     284,   285,    62,    63,    64,    65,    67,    68,    70,    71,
      26,   236,   237,    76,   239,    74,   241,   242,   243,   138,
      75,   206,   126,    78,    79,    80,    83,    81,    85,   257,
      87,   259,    89,    90,    91,    92,    93,    96,    97,   102,
     111,   216,   112,   310,   103,   317,   113,   120,   115,   143,
     114,   132,   116,   133,   131,   149,   152,   290,   148,   292,
     230,   167,   153,   232,   157,   173,   158,   174,   300,   177,
     179,   182,   183,   304,   189,   334,   190,   192,   308,   317,
     193,   198,   213,   160,   221,   208,   211,   224,   228,   219,
     229,   231,   220,   233,   234,   235,   222,   246,   248,    57,
     225,    58,   326,   226,   223,   227,   330,   255,   238,   240,
     318,   320,   322,   244,   245,   335,   247,   249,   251,   253,
     256,   270,   258,   260,   262,   264,    19,   293,   272,   289,
     291,   294,   295,   297,    40,   178,   298,   319,   301,   323,
     299,   336,   267,   321,   303,   217,   302,   218,   269,   305,
     307,   306,   324,   325,   327,   329,     0,   328,    73,   331,
     196,     0,   184,   332,   333,   186,   337,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   110
};

static const yytype_int16 yycheck[] =
{
      50,    44,     3,    93,    93,   126,   118,   119,     9,    59,
      60,    34,     3,    70,    71,    16,    48,     6,     3,    76,
      52,     6,   131,     3,     4,     5,     6,     3,     0,   138,
       3,    32,    89,    90,    91,   125,    27,    49,    50,   148,
       3,     3,   132,   132,   153,     8,     8,     3,    47,    12,
      13,    13,     8,    48,    27,    56,    12,    13,   167,    18,
      23,    23,    52,     3,    27,    27,     6,     3,   180,   159,
     160,    27,     8,    36,    36,    55,   133,    13,    79,    55,
      36,   190,    55,    51,   193,    29,    30,    23,    32,    31,
      15,    27,    55,    55,    16,   152,     3,   187,   187,    55,
      36,     3,     3,     3,     4,     5,     6,     3,     4,     5,
       6,    13,    13,    47,   171,    55,   173,    47,     3,    55,
     170,    23,    23,    48,    51,    27,    27,    52,    13,     3,
       3,     4,     5,     6,    36,    36,   179,   258,    23,    51,
      50,    50,    27,    53,    53,     3,     4,     5,     4,    31,
       6,    36,    52,    55,    55,    55,   199,    49,    50,    55,
       5,     6,    51,    51,     3,     6,    56,    50,     6,    39,
      55,   228,   229,     6,   231,    47,   233,   234,   235,    52,
      47,   182,    55,    52,    31,    49,    51,    50,    50,   246,
      50,   248,     6,     6,     6,    18,    15,    51,     3,    47,
       6,   202,    51,   293,    47,   295,     4,     3,     5,     3,
      56,    15,    56,     6,    54,    18,     6,   267,    54,   269,
     221,    52,    54,   224,    53,    53,    18,     6,   278,     3,
      31,    31,     3,   283,    56,   325,    54,    53,   288,   329,
      50,    52,     3,    52,    31,    53,    53,    31,     6,    51,
       6,     6,    51,     6,     6,     6,    51,     6,     6,     4,
      51,     5,   312,    51,    53,    51,   316,    52,    51,    51,
       3,     3,     3,    54,    54,     3,    54,    54,    54,    54,
      52,    54,    52,    52,    52,    52,     4,    52,    54,    53,
      53,    53,    52,    52,    16,   150,    53,   299,    52,   307,
      50,   329,   255,   303,    50,   202,    53,   202,   256,    52,
      50,    53,    53,    50,    52,    50,    -1,    53,    38,    53,
     172,    -1,   159,    53,    53,   160,    53,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    81
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    34,    58,    59,    60,    65,     6,     0,     3,     8,
      12,    13,    27,    36,    55,    61,    62,    73,    97,    65,
      47,    48,    18,    13,    23,    36,    55,    97,    98,    99,
      52,    51,    31,     3,   101,   102,    16,    63,    64,    95,
      73,    47,    51,    47,     8,    12,    13,    23,    36,    66,
      67,    68,    69,    70,    97,     3,    31,     4,     5,   104,
     106,    51,    51,    51,     3,     6,    97,    56,    50,    77,
       6,    39,    96,    95,    47,    47,     6,    70,    52,    31,
      49,    50,    77,    51,    97,    50,    77,    50,    77,     6,
       6,     6,    18,    15,   117,   118,    51,     3,    48,    52,
     117,   117,    47,    47,   117,     3,    27,    71,    72,    97,
      68,     6,    51,     4,    56,     5,    56,   117,   117,   117,
       3,     3,     4,     5,     6,    52,    55,   100,   119,   121,
     122,    54,    15,     6,     3,     4,     5,     6,    52,   100,
     109,   110,   112,     3,    29,    30,    32,   116,    54,    18,
      50,    53,     6,    54,   111,   111,   111,    53,    18,    48,
      52,   119,   120,   101,   112,   119,   117,    52,   112,   113,
     114,    49,    50,    53,     6,   115,   112,     3,    72,    31,
     117,   112,    31,     3,   120,     6,   122,    50,    53,    56,
      54,   112,    53,    50,    77,   117,   110,   117,    52,     8,
      70,   111,     8,    13,    23,    36,    97,    49,    53,   119,
     112,    53,   112,     3,    70,    36,    97,    98,    99,    51,
      51,    31,    51,    53,    31,    51,    51,    51,     6,     6,
      97,     6,    97,     6,     6,     6,   117,   117,    51,   117,
      51,   117,   117,   117,    54,    54,     6,    54,     6,    54,
      91,    54,    92,    54,    93,    52,    52,   117,    52,   117,
      52,    78,    52,    86,    52,    82,   103,   104,   105,   106,
      54,   101,    54,    94,     3,     6,    74,    75,    76,     4,
       6,    83,    84,    85,     5,     6,    79,    80,    81,    53,
      77,    53,    77,    52,    53,    52,    90,    52,    53,    50,
      77,    52,    53,    50,    77,    52,    53,    50,    77,     3,
     100,   107,   108,     6,    87,    88,    89,   100,     3,    74,
       3,    83,     3,    79,    53,    50,    77,    52,    53,    50,
      77,    53,    53,    53,   100,     3,    87,    53
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (parm, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, parm); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, void *parm)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, parm)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    void *parm;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (parm);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, void *parm)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, parm)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    void *parm;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, parm);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule, void *parm)
#else
static void
yy_reduce_print (yyvsp, yyrule, parm)
    YYSTYPE *yyvsp;
    int yyrule;
    void *parm;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       , parm);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule, parm); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, void *parm)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, parm)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    void *parm;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (parm);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void *parm);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */





/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *parm)
#else
int
yyparse (parm)
    void *parm;
#endif
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 7:

/* Line 1455 of yacc.c  */
#line 476 "flatzinc/parser.yxx"
    {
#if !EXPOSE_INT_LITS
        initfg(static_cast<ParserState*>(parm));
#endif
      ;}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 482 "flatzinc/parser.yxx"
    {
#if !EXPOSE_INT_LITS
        initfg(static_cast<ParserState*>(parm));
#endif
      ;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 494 "flatzinc/parser.yxx"
    {
#if EXPOSE_INT_LITS
        initfg(static_cast<ParserState*>(parm));
#endif
      ;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 500 "flatzinc/parser.yxx"
    {
#if EXPOSE_INT_LITS
        initfg(static_cast<ParserState*>(parm));
#endif
      ;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 554 "flatzinc/parser.yxx"
    {
        ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, !(yyvsp[(2) - (6)].oSet)() || !(yyvsp[(2) - (6)].oSet).some()->empty(), "Empty var int domain.");
        bool print = (yyvsp[(5) - (6)].argVec)->hasAtom("output_var");
        pp->intvarTable.put((yyvsp[(4) - (6)].sValue), pp->intvars.size());
        if (print) {
          pp->output(std::string((yyvsp[(4) - (6)].sValue)), new AST::IntVar(pp->intvars.size()));
        }
        if ((yyvsp[(6) - (6)].oArg)()) {
          AST::Node* arg = (yyvsp[(6) - (6)].oArg).some();
          if (arg->isInt()) {
            pp->intvars.push_back(varspec((yyvsp[(4) - (6)].sValue),
              new IntVarSpec(arg->getInt(),!print)));
          } else if (arg->isIntVar()) {
            pp->intvars.push_back(varspec((yyvsp[(4) - (6)].sValue),
              new IntVarSpec(Alias(arg->getIntVar()),!print)));
          } else {
            yyassert(pp, false, "Invalid var int initializer.");
          }
          if (!pp->hadError)
            addDomainConstraint(pp, "set_in",
                                new AST::IntVar(pp->intvars.size()-1), (yyvsp[(2) - (6)].oSet));
          delete arg;
        } else {
          pp->intvars.push_back(varspec((yyvsp[(4) - (6)].sValue), new IntVarSpec((yyvsp[(2) - (6)].oSet),!print)));
        }
        delete (yyvsp[(5) - (6)].argVec); free((yyvsp[(4) - (6)].sValue));
      ;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 583 "flatzinc/parser.yxx"
    {
        ParserState* pp = static_cast<ParserState*>(parm);
        bool print = (yyvsp[(5) - (6)].argVec)->hasAtom("output_var");
        pp->boolvarTable.put((yyvsp[(4) - (6)].sValue), pp->boolvars.size());
        if (print) {
          pp->output(std::string((yyvsp[(4) - (6)].sValue)), new AST::BoolVar(pp->boolvars.size()));
        }
        if ((yyvsp[(6) - (6)].oArg)()) {
          AST::Node* arg = (yyvsp[(6) - (6)].oArg).some();
          if (arg->isBool()) {
            pp->boolvars.push_back(varspec((yyvsp[(4) - (6)].sValue),
              new BoolVarSpec(arg->getBool(),!print)));            
          } else if (arg->isBoolVar()) {
            pp->boolvars.push_back(varspec((yyvsp[(4) - (6)].sValue),
              new BoolVarSpec(Alias(arg->getBoolVar()),!print)));
          } else {
            yyassert(pp, false, "Invalid var bool initializer.");
          }
          if (!pp->hadError)
            addDomainConstraint(pp, "set_in",
                                new AST::BoolVar(pp->boolvars.size()-1), (yyvsp[(2) - (6)].oSet));
          delete arg;
        } else {
          pp->boolvars.push_back(varspec((yyvsp[(4) - (6)].sValue), new BoolVarSpec((yyvsp[(2) - (6)].oSet),!print)));
        }
        delete (yyvsp[(5) - (6)].argVec); free((yyvsp[(4) - (6)].sValue));
      ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 611 "flatzinc/parser.yxx"
    { ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, false, "Floats not supported.");
        delete (yyvsp[(5) - (6)].argVec); free((yyvsp[(4) - (6)].sValue));
      ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 616 "flatzinc/parser.yxx"
    { 
        ParserState* pp = static_cast<ParserState*>(parm);
        bool print = (yyvsp[(7) - (8)].argVec)->hasAtom("output_var");
        pp->setvarTable.put((yyvsp[(6) - (8)].sValue), pp->setvars.size());
        if (print) {
          pp->output(std::string((yyvsp[(6) - (8)].sValue)), new AST::SetVar(pp->setvars.size()));
        }
        if ((yyvsp[(8) - (8)].oArg)()) {
          AST::Node* arg = (yyvsp[(8) - (8)].oArg).some();
          if (arg->isSet()) {
            pp->setvars.push_back(varspec((yyvsp[(6) - (8)].sValue),
              new SetVarSpec(arg->getSet(),!print)));            
          } else if (arg->isSetVar()) {
            pp->setvars.push_back(varspec((yyvsp[(6) - (8)].sValue),
              new SetVarSpec(Alias(arg->getSetVar()),!print)));
            delete arg;
          } else {
            yyassert(pp, false, "Invalid var set initializer.");
            delete arg;
          }
          if (!pp->hadError)
            addDomainConstraint(pp, "set_subset",
                                new AST::SetVar(pp->setvars.size()-1), (yyvsp[(4) - (8)].oSet));
        } else {
          pp->setvars.push_back(varspec((yyvsp[(6) - (8)].sValue), new SetVarSpec((yyvsp[(4) - (8)].oSet),!print)));
        }
        delete (yyvsp[(7) - (8)].argVec); free((yyvsp[(6) - (8)].sValue));
      ;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 645 "flatzinc/parser.yxx"
    {
        ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, !(yyvsp[(1) - (6)].oSet)() || !(yyvsp[(1) - (6)].oSet).some()->empty(), "Empty int domain.");
        yyassert(pp, (yyvsp[(6) - (6)].arg)->isInt(), "Invalid int initializer.");
        int i = -1;
        bool isInt = (yyvsp[(6) - (6)].arg)->isInt(i);
        if ((yyvsp[(1) - (6)].oSet)() && isInt) {
          AST::SetLit* sl = (yyvsp[(1) - (6)].oSet).some();
          if (sl->interval) {
            yyassert(pp, i >= sl->min && i <= sl->max, "Empty int domain.");
          } else {
            bool found = false;
            for (unsigned int j=0; j<sl->s.size(); j++)
              if (sl->s[j] == i) {
                found = true;
                break;
              }
            yyassert(pp, found, "Empty int domain.");
          }
        }
        pp->intvals.put((yyvsp[(3) - (6)].sValue), i);
        delete (yyvsp[(4) - (6)].argVec); free((yyvsp[(3) - (6)].sValue));        
      ;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 669 "flatzinc/parser.yxx"
    {
        ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, (yyvsp[(6) - (6)].arg)->isBool(), "Invalid bool initializer.");
        if ((yyvsp[(6) - (6)].arg)->isBool()) {
          pp->boolvals.put((yyvsp[(3) - (6)].sValue), (yyvsp[(6) - (6)].arg)->getBool());
        }
        delete (yyvsp[(4) - (6)].argVec); free((yyvsp[(3) - (6)].sValue));        
      ;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 678 "flatzinc/parser.yxx"
    {
        ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, !(yyvsp[(3) - (8)].oSet)() || !(yyvsp[(3) - (8)].oSet).some()->empty(), "Empty set domain.");
        yyassert(pp, (yyvsp[(8) - (8)].arg)->isSet(), "Invalid set initializer.");
        AST::SetLit* set = NULL;
        if ((yyvsp[(8) - (8)].arg)->isSet())
          set = (yyvsp[(8) - (8)].arg)->getSet();
        pp->setvals.put((yyvsp[(5) - (8)].sValue), *set);
        delete set;
        delete (yyvsp[(6) - (8)].argVec); free((yyvsp[(5) - (8)].sValue));        
      ;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 691 "flatzinc/parser.yxx"
    {
        ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, (yyvsp[(3) - (13)].iValue)==1, "Arrays must start at 1");
        if (!pp->hadError) {
          bool print = (yyvsp[(12) - (13)].argVec)->hasCall("output_array");
          vector<int> vars((yyvsp[(5) - (13)].iValue));
          yyassert(pp, !(yyvsp[(9) - (13)].oSet)() || !(yyvsp[(9) - (13)].oSet).some()->empty(),
                   "Empty var int domain.");
          if (!pp->hadError) {
            if ((yyvsp[(13) - (13)].oVarSpecVec)()) {
              vector<VarSpec*>* vsv = (yyvsp[(13) - (13)].oVarSpecVec).some();
              yyassert(pp, vsv->size() == static_cast<unsigned int>((yyvsp[(5) - (13)].iValue)),
                       "Initializer size does not match array dimension");
              if (!pp->hadError) {
                for (int i=0; i<(yyvsp[(5) - (13)].iValue); i++) {
                  IntVarSpec* ivsv = static_cast<IntVarSpec*>((*vsv)[i]);
                  if (ivsv->alias) {
                    vars[i] = ivsv->i;
                  } else {
                    vars[i] = pp->intvars.size();
                    pp->intvars.push_back(varspec((yyvsp[(11) - (13)].sValue), ivsv));
                  }
                  if (!pp->hadError && (yyvsp[(9) - (13)].oSet)()) {
                    Option<AST::SetLit*> opt =
                      Option<AST::SetLit*>::some(new AST::SetLit(*(yyvsp[(9) - (13)].oSet).some()));                    
                    addDomainConstraint(pp, "set_in",
                                        new AST::IntVar(vars[i]),
                                        opt);
                  }
                }
              }
              delete vsv;
            } else {
              IntVarSpec* ispec = new IntVarSpec((yyvsp[(9) - (13)].oSet),!print);
              string arrayname = "["; arrayname += (yyvsp[(11) - (13)].sValue);
              for (int i=0; i<(yyvsp[(5) - (13)].iValue)-1; i++) {
                vars[i] = pp->intvars.size();
                pp->intvars.push_back(varspec(arrayname, ispec));
              }          
              vars[(yyvsp[(5) - (13)].iValue)-1] = pp->intvars.size();
              pp->intvars.push_back(varspec((yyvsp[(11) - (13)].sValue), ispec));
            }
          }
          if (print) {
            AST::Array* a = new AST::Array();
            a->a.push_back(arrayOutput((yyvsp[(12) - (13)].argVec)->getCall("output_array")));
            AST::Array* output = new AST::Array();
            for (int i=0; i<(yyvsp[(5) - (13)].iValue); i++)
              output->a.push_back(new AST::IntVar(vars[i]));
            a->a.push_back(output);
            a->a.push_back(new AST::String(")"));
            pp->output(std::string((yyvsp[(11) - (13)].sValue)), a);
          }
          pp->intvararrays.put((yyvsp[(11) - (13)].sValue), vars);
        }
        delete (yyvsp[(12) - (13)].argVec); free((yyvsp[(11) - (13)].sValue));
      ;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 750 "flatzinc/parser.yxx"
    {
        ParserState* pp = static_cast<ParserState*>(parm);
        bool print = (yyvsp[(12) - (13)].argVec)->hasCall("output_array");
        yyassert(pp, (yyvsp[(3) - (13)].iValue)==1, "Arrays must start at 1");
        if (!pp->hadError) {
          vector<int> vars((yyvsp[(5) - (13)].iValue));
          if ((yyvsp[(13) - (13)].oVarSpecVec)()) {
            vector<VarSpec*>* vsv = (yyvsp[(13) - (13)].oVarSpecVec).some();
            yyassert(pp, vsv->size() == static_cast<unsigned int>((yyvsp[(5) - (13)].iValue)),
                     "Initializer size does not match array dimension");
            if (!pp->hadError) {
              for (int i=0; i<(yyvsp[(5) - (13)].iValue); i++) {
                BoolVarSpec* bvsv = static_cast<BoolVarSpec*>((*vsv)[i]);
                if (bvsv->alias)
                  vars[i] = bvsv->i;
                else {
                  vars[i] = pp->boolvars.size();
                  pp->boolvars.push_back(varspec((yyvsp[(11) - (13)].sValue), (*vsv)[i]));
                }
                if (!pp->hadError && (yyvsp[(9) - (13)].oSet)()) {
                  Option<AST::SetLit*> opt =
                    Option<AST::SetLit*>::some(new AST::SetLit(*(yyvsp[(9) - (13)].oSet).some()));                    
                  addDomainConstraint(pp, "set_in",
                                      new AST::BoolVar(vars[i]),
                                      opt);
                }
              }
            }
            delete vsv;
          } else {
            for (int i=0; i<(yyvsp[(5) - (13)].iValue); i++) {
              vars[i] = pp->boolvars.size();
              pp->boolvars.push_back(varspec((yyvsp[(11) - (13)].sValue),
                                             new BoolVarSpec((yyvsp[(9) - (13)].oSet),!print)));
            }          
          }
          if (print) {
            AST::Array* a = new AST::Array();
            a->a.push_back(arrayOutput((yyvsp[(12) - (13)].argVec)->getCall("output_array")));
            AST::Array* output = new AST::Array();
            for (int i=0; i<(yyvsp[(5) - (13)].iValue); i++)
              output->a.push_back(new AST::BoolVar(vars[i]));
            a->a.push_back(output);
            a->a.push_back(new AST::String(")"));
            pp->output(std::string((yyvsp[(11) - (13)].sValue)), a);
          }
          pp->boolvararrays.put((yyvsp[(11) - (13)].sValue), vars);
        }
        delete (yyvsp[(12) - (13)].argVec); free((yyvsp[(11) - (13)].sValue));
      ;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 802 "flatzinc/parser.yxx"
    { 
        ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, false, "Floats not supported.");
        delete (yyvsp[(12) - (13)].argVec); free((yyvsp[(11) - (13)].sValue));
      ;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 809 "flatzinc/parser.yxx"
    { 
        ParserState* pp = static_cast<ParserState*>(parm);
        bool print = (yyvsp[(14) - (15)].argVec)->hasCall("output_array");
        yyassert(pp, (yyvsp[(3) - (15)].iValue)==1, "Arrays must start at 1");
        if (!pp->hadError) {
          vector<int> vars((yyvsp[(5) - (15)].iValue));
          if ((yyvsp[(15) - (15)].oVarSpecVec)()) {
            vector<VarSpec*>* vsv = (yyvsp[(15) - (15)].oVarSpecVec).some();
            yyassert(pp, vsv->size() == static_cast<unsigned int>((yyvsp[(5) - (15)].iValue)),
                     "Initializer size does not match array dimension");
            if (!pp->hadError) {
              for (int i=0; i<(yyvsp[(5) - (15)].iValue); i++) {
                SetVarSpec* svsv = static_cast<SetVarSpec*>((*vsv)[i]);
                if (svsv->alias)
                  vars[i] = svsv->i;
                else {
                  vars[i] = pp->setvars.size();
                  pp->setvars.push_back(varspec((yyvsp[(13) - (15)].sValue), (*vsv)[i]));
                }
                if (!pp->hadError && (yyvsp[(11) - (15)].oSet)()) {
                  Option<AST::SetLit*> opt =
                    Option<AST::SetLit*>::some(new AST::SetLit(*(yyvsp[(11) - (15)].oSet).some()));                    
                  addDomainConstraint(pp, "set_subset",
                                      new AST::SetVar(vars[i]),
                                      opt);
                }
              }
            }
            delete vsv;
          } else {
            SetVarSpec* ispec = new SetVarSpec((yyvsp[(11) - (15)].oSet),!print);
            string arrayname = "["; arrayname += (yyvsp[(13) - (15)].sValue);
            for (int i=0; i<(yyvsp[(5) - (15)].iValue)-1; i++) {
              vars[i] = pp->setvars.size();
              pp->setvars.push_back(varspec(arrayname, ispec));
            }          
            vars[(yyvsp[(5) - (15)].iValue)-1] = pp->setvars.size();
            pp->setvars.push_back(varspec((yyvsp[(13) - (15)].sValue), ispec));
          }
          if (print) {
            AST::Array* a = new AST::Array();
            a->a.push_back(arrayOutput((yyvsp[(14) - (15)].argVec)->getCall("output_array")));
            AST::Array* output = new AST::Array();
            for (int i=0; i<(yyvsp[(5) - (15)].iValue); i++)
              output->a.push_back(new AST::SetVar(vars[i]));
            a->a.push_back(output);
            a->a.push_back(new AST::String(")"));
            pp->output(std::string((yyvsp[(13) - (15)].sValue)), a);
          }
          pp->setvararrays.put((yyvsp[(13) - (15)].sValue), vars);
        }
        delete (yyvsp[(14) - (15)].argVec); free((yyvsp[(13) - (15)].sValue));
      ;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 864 "flatzinc/parser.yxx"
    {
        ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, (yyvsp[(3) - (15)].iValue)==1, "Arrays must start at 1");
        yyassert(pp, (yyvsp[(14) - (15)].setValue)->size() == static_cast<unsigned int>((yyvsp[(5) - (15)].iValue)),
                 "Initializer size does not match array dimension");
        if (!pp->hadError)
          pp->intvalarrays.put((yyvsp[(10) - (15)].sValue), *(yyvsp[(14) - (15)].setValue));
        delete (yyvsp[(14) - (15)].setValue);
        free((yyvsp[(10) - (15)].sValue));
        delete (yyvsp[(11) - (15)].argVec);
      ;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 877 "flatzinc/parser.yxx"
    {
        ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, (yyvsp[(3) - (15)].iValue)==1, "Arrays must start at 1");
        yyassert(pp, (yyvsp[(14) - (15)].setValue)->size() == static_cast<unsigned int>((yyvsp[(5) - (15)].iValue)),
                 "Initializer size does not match array dimension");
        if (!pp->hadError)
          pp->boolvalarrays.put((yyvsp[(10) - (15)].sValue), *(yyvsp[(14) - (15)].setValue));
        delete (yyvsp[(14) - (15)].setValue);
        free((yyvsp[(10) - (15)].sValue));
        delete (yyvsp[(11) - (15)].argVec);
      ;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 890 "flatzinc/parser.yxx"
    {
        ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, false, "Floats not supported.");
        delete (yyvsp[(11) - (15)].argVec); free((yyvsp[(10) - (15)].sValue));
      ;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 897 "flatzinc/parser.yxx"
    {
        ParserState* pp = static_cast<ParserState*>(parm);
        yyassert(pp, (yyvsp[(3) - (17)].iValue)==1, "Arrays must start at 1");
        yyassert(pp, (yyvsp[(16) - (17)].setValueList)->size() == static_cast<unsigned int>((yyvsp[(5) - (17)].iValue)),
                 "Initializer size does not match array dimension");
        if (!pp->hadError)
          pp->setvalarrays.put((yyvsp[(12) - (17)].sValue), *(yyvsp[(16) - (17)].setValueList));
        delete (yyvsp[(16) - (17)].setValueList);
        delete (yyvsp[(13) - (17)].argVec); free((yyvsp[(12) - (17)].sValue));
      ;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 910 "flatzinc/parser.yxx"
    { 
        (yyval.varSpec) = new IntVarSpec((yyvsp[(1) - (1)].iValue),false);
      ;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 914 "flatzinc/parser.yxx"
    { 
        int v = 0;
        ParserState* pp = static_cast<ParserState*>(parm);
        if (pp->intvarTable.get((yyvsp[(1) - (1)].sValue), v))
          (yyval.varSpec) = new IntVarSpec(Alias(v),false);
        else {
          pp->err << "Error: undefined identifier " << (yyvsp[(1) - (1)].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
          (yyval.varSpec) = new IntVarSpec(0,false); // keep things consistent
        }
        free((yyvsp[(1) - (1)].sValue));
      ;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 929 "flatzinc/parser.yxx"
    { 
        vector<int> v;
        ParserState* pp = static_cast<ParserState*>(parm);
        if (pp->intvararrays.get((yyvsp[(1) - (4)].sValue), v)) {
          yyassert(pp,static_cast<unsigned int>((yyvsp[(3) - (4)].iValue)) > 0 && 
                      static_cast<unsigned int>((yyvsp[(3) - (4)].iValue)) <= v.size(),
                   "array access out of bounds");
          if (!pp->hadError)
            (yyval.varSpec) = new IntVarSpec(Alias(v[(yyvsp[(3) - (4)].iValue)-1]),false);
          else
            (yyval.varSpec) = new IntVarSpec(0,false); // keep things consistent
        } else {
          pp->err << "Error: undefined array identifier " << (yyvsp[(1) - (4)].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
          (yyval.varSpec) = new IntVarSpec(0,false); // keep things consistent
        }
        free((yyvsp[(1) - (4)].sValue));
      ;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 952 "flatzinc/parser.yxx"
    { (yyval.varSpecVec) = new vector<VarSpec*>(0); ;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 954 "flatzinc/parser.yxx"
    { (yyval.varSpecVec) = (yyvsp[(1) - (2)].varSpecVec); ;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 958 "flatzinc/parser.yxx"
    { (yyval.varSpecVec) = new vector<VarSpec*>(1); (*(yyval.varSpecVec))[0] = (yyvsp[(1) - (1)].varSpec); ;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 960 "flatzinc/parser.yxx"
    { (yyval.varSpecVec) = (yyvsp[(1) - (3)].varSpecVec); (yyval.varSpecVec)->push_back((yyvsp[(3) - (3)].varSpec)); ;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 965 "flatzinc/parser.yxx"
    { (yyval.varSpecVec) = (yyvsp[(2) - (3)].varSpecVec); ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 969 "flatzinc/parser.yxx"
    { (yyval.varSpec) = new FloatVarSpec((yyvsp[(1) - (1)].dValue),false); ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 971 "flatzinc/parser.yxx"
    { 
        int v = 0;
        ParserState* pp = static_cast<ParserState*>(parm);
        if (pp->floatvarTable.get((yyvsp[(1) - (1)].sValue), v))
          (yyval.varSpec) = new FloatVarSpec(Alias(v),false);
        else {
          pp->err << "Error: undefined identifier " << (yyvsp[(1) - (1)].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
          (yyval.varSpec) = new FloatVarSpec(0.0,false);
        }
        free((yyvsp[(1) - (1)].sValue));
      ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 986 "flatzinc/parser.yxx"
    { 
        vector<int> v;
        ParserState* pp = static_cast<ParserState*>(parm);
        if (pp->floatvararrays.get((yyvsp[(1) - (4)].sValue), v)) {
          yyassert(pp,static_cast<unsigned int>((yyvsp[(3) - (4)].iValue)) > 0 && 
                      static_cast<unsigned int>((yyvsp[(3) - (4)].iValue)) <= v.size(),
                   "array access out of bounds");
          if (!pp->hadError)
            (yyval.varSpec) = new FloatVarSpec(Alias(v[(yyvsp[(3) - (4)].iValue)-1]),false);
          else
            (yyval.varSpec) = new FloatVarSpec(0.0,false);
        } else {
          pp->err << "Error: undefined array identifier " << (yyvsp[(1) - (4)].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
          (yyval.varSpec) = new FloatVarSpec(0.0,false);
        }
        free((yyvsp[(1) - (4)].sValue));
      ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 1009 "flatzinc/parser.yxx"
    { (yyval.varSpecVec) = new vector<VarSpec*>(0); ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 1011 "flatzinc/parser.yxx"
    { (yyval.varSpecVec) = (yyvsp[(1) - (2)].varSpecVec); ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 1015 "flatzinc/parser.yxx"
    { (yyval.varSpecVec) = new vector<VarSpec*>(1); (*(yyval.varSpecVec))[0] = (yyvsp[(1) - (1)].varSpec); ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 1017 "flatzinc/parser.yxx"
    { (yyval.varSpecVec) = (yyvsp[(1) - (3)].varSpecVec); (yyval.varSpecVec)->push_back((yyvsp[(3) - (3)].varSpec)); ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 1021 "flatzinc/parser.yxx"
    { (yyval.varSpecVec) = (yyvsp[(2) - (3)].varSpecVec); ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 1025 "flatzinc/parser.yxx"
    { (yyval.varSpec) = new BoolVarSpec((yyvsp[(1) - (1)].iValue),false); ;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 1027 "flatzinc/parser.yxx"
    { 
        int v = 0;
        ParserState* pp = static_cast<ParserState*>(parm);
        if (pp->boolvarTable.get((yyvsp[(1) - (1)].sValue), v))
          (yyval.varSpec) = new BoolVarSpec(Alias(v),false);
        else {
          pp->err << "Error: undefined identifier " << (yyvsp[(1) - (1)].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
          (yyval.varSpec) = new BoolVarSpec(false,false);
        }
        free((yyvsp[(1) - (1)].sValue));
      ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 1042 "flatzinc/parser.yxx"
    { 
        vector<int> v;
        ParserState* pp = static_cast<ParserState*>(parm);
        if (pp->boolvararrays.get((yyvsp[(1) - (4)].sValue), v)) {
          yyassert(pp,static_cast<unsigned int>((yyvsp[(3) - (4)].iValue)) > 0 && 
                      static_cast<unsigned int>((yyvsp[(3) - (4)].iValue)) <= v.size(),
                   "array access out of bounds");
          if (!pp->hadError)
            (yyval.varSpec) = new BoolVarSpec(Alias(v[(yyvsp[(3) - (4)].iValue)-1]),false);
          else
            (yyval.varSpec) = new BoolVarSpec(false,false);
        } else {
          pp->err << "Error: undefined array identifier " << (yyvsp[(1) - (4)].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
          (yyval.varSpec) = new BoolVarSpec(false,false);
        }
        free((yyvsp[(1) - (4)].sValue));
      ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 1065 "flatzinc/parser.yxx"
    { (yyval.varSpecVec) = new vector<VarSpec*>(0); ;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 1067 "flatzinc/parser.yxx"
    { (yyval.varSpecVec) = (yyvsp[(1) - (2)].varSpecVec); ;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 1071 "flatzinc/parser.yxx"
    { (yyval.varSpecVec) = new vector<VarSpec*>(1); (*(yyval.varSpecVec))[0] = (yyvsp[(1) - (1)].varSpec); ;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 1073 "flatzinc/parser.yxx"
    { (yyval.varSpecVec) = (yyvsp[(1) - (3)].varSpecVec); (yyval.varSpecVec)->push_back((yyvsp[(3) - (3)].varSpec)); ;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 1075 "flatzinc/parser.yxx"
    { (yyval.varSpecVec) = (yyvsp[(2) - (3)].varSpecVec); ;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 1079 "flatzinc/parser.yxx"
    { (yyval.varSpec) = new SetVarSpec(Option<AST::SetLit*>::some((yyvsp[(1) - (1)].setLit)),false); ;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 1081 "flatzinc/parser.yxx"
    { 
        ParserState* pp = static_cast<ParserState*>(parm);
        int v = 0;
        if (pp->setvarTable.get((yyvsp[(1) - (1)].sValue), v))
          (yyval.varSpec) = new SetVarSpec(Alias(v),false);
        else {
          pp->err << "Error: undefined identifier " << (yyvsp[(1) - (1)].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
          (yyval.varSpec) = new SetVarSpec(Alias(0),false);
        }
        free((yyvsp[(1) - (1)].sValue));
      ;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 1096 "flatzinc/parser.yxx"
    { 
        vector<int> v;
        ParserState* pp = static_cast<ParserState*>(parm);
        if (pp->setvararrays.get((yyvsp[(1) - (4)].sValue), v)) {
          yyassert(pp,static_cast<unsigned int>((yyvsp[(3) - (4)].iValue)) > 0 && 
                      static_cast<unsigned int>((yyvsp[(3) - (4)].iValue)) <= v.size(),
                   "array access out of bounds");
          if (!pp->hadError)
            (yyval.varSpec) = new SetVarSpec(Alias(v[(yyvsp[(3) - (4)].iValue)-1]),false);
          else
            (yyval.varSpec) = new SetVarSpec(Alias(0),false);
        } else {
          pp->err << "Error: undefined array identifier " << (yyvsp[(1) - (4)].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
          (yyval.varSpec) = new SetVarSpec(Alias(0),false);
        }
        free((yyvsp[(1) - (4)].sValue));
      ;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 1119 "flatzinc/parser.yxx"
    { (yyval.varSpecVec) = new vector<VarSpec*>(0); ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 1121 "flatzinc/parser.yxx"
    { (yyval.varSpecVec) = (yyvsp[(1) - (2)].varSpecVec); ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 1125 "flatzinc/parser.yxx"
    { (yyval.varSpecVec) = new vector<VarSpec*>(1); (*(yyval.varSpecVec))[0] = (yyvsp[(1) - (1)].varSpec); ;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 1127 "flatzinc/parser.yxx"
    { (yyval.varSpecVec) = (yyvsp[(1) - (3)].varSpecVec); (yyval.varSpecVec)->push_back((yyvsp[(3) - (3)].varSpec)); ;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 1130 "flatzinc/parser.yxx"
    { (yyval.varSpecVec) = (yyvsp[(2) - (3)].varSpecVec); ;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 1134 "flatzinc/parser.yxx"
    { (yyval.oVarSpecVec) = Option<vector<VarSpec*>* >::none(); ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 1136 "flatzinc/parser.yxx"
    { (yyval.oVarSpecVec) = Option<vector<VarSpec*>* >::some((yyvsp[(2) - (2)].varSpecVec)); ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 1140 "flatzinc/parser.yxx"
    { (yyval.oVarSpecVec) = Option<vector<VarSpec*>* >::none(); ;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 1142 "flatzinc/parser.yxx"
    { (yyval.oVarSpecVec) = Option<vector<VarSpec*>* >::some((yyvsp[(2) - (2)].varSpecVec)); ;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 1146 "flatzinc/parser.yxx"
    { (yyval.oVarSpecVec) = Option<vector<VarSpec*>* >::none(); ;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 1148 "flatzinc/parser.yxx"
    { (yyval.oVarSpecVec) = Option<vector<VarSpec*>* >::some((yyvsp[(2) - (2)].varSpecVec)); ;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 1152 "flatzinc/parser.yxx"
    { (yyval.oVarSpecVec) = Option<vector<VarSpec*>* >::none(); ;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 1154 "flatzinc/parser.yxx"
    { (yyval.oVarSpecVec) = Option<vector<VarSpec*>* >::some((yyvsp[(2) - (2)].varSpecVec)); ;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 1158 "flatzinc/parser.yxx"
    { 
        ParserState *pp = static_cast<ParserState*>(parm);
#if EXPOSE_INT_LITS
        pp->domainConstraints2.push_back(std::pair<ConExpr*, AST::Node*>(new ConExpr((yyvsp[(2) - (6)].sValue), (yyvsp[(4) - (6)].argVec)), (yyvsp[(6) - (6)].argVec)));
#else
        ConExpr c((yyvsp[(2) - (6)].sValue), (yyvsp[(4) - (6)].argVec));
        if (!pp->hadError) {
          try {
            pp->fg->postConstraint(c, (yyvsp[(6) - (6)].argVec));
          } catch (FlatZinc::Error& e) {
            yyerror(pp, e.toString().c_str());
          }
        }
        delete (yyvsp[(6) - (6)].argVec);
#endif
        free((yyvsp[(2) - (6)].sValue));
      ;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 1176 "flatzinc/parser.yxx"
    {
        ParserState *pp = static_cast<ParserState*>(parm);
        AST::Array* args = new AST::Array(2);
        args->a[0] = getVarRefArg(pp,(yyvsp[(2) - (3)].sValue));
        args->a[1] = new AST::BoolLit(true);
#if EXPOSE_INT_LITS
        pp->domainConstraints2.push_back(std::pair<ConExpr*, AST::Node*>(new ConExpr("bool_eq", args), (yyvsp[(3) - (3)].argVec)));
#else
        ConExpr c("bool_eq", args);
        if (!pp->hadError) {
          try {
            pp->fg->postConstraint(c, (yyvsp[(3) - (3)].argVec));
          } catch (FlatZinc::Error& e) {
            yyerror(pp, e.toString().c_str());
          }
        }
        delete (yyvsp[(3) - (3)].argVec);
#endif
        free((yyvsp[(2) - (3)].sValue));
      ;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 1197 "flatzinc/parser.yxx"
    { 
          ParserState *pp = static_cast<ParserState*>(parm);
          AST::Array* args = new AST::Array(2);
          args->a[0] = getArrayElement(pp,(yyvsp[(2) - (6)].sValue),(yyvsp[(4) - (6)].iValue));
          args->a[1] = new AST::BoolLit(true);
#if EXPOSE_INT_LITS
          pp->domainConstraints2.push_back(std::pair<ConExpr*, AST::Node*>(new ConExpr("bool_eq", args), (yyvsp[(6) - (6)].argVec)));
#else
          ConExpr c("bool_eq", args);
          if (!pp->hadError) {
            try {
              pp->fg->postConstraint(c, (yyvsp[(6) - (6)].argVec));
            } catch (FlatZinc::Error& e) {
              yyerror(pp, e.toString().c_str());
            }
          }
          delete (yyvsp[(6) - (6)].argVec);
#endif
          free((yyvsp[(2) - (6)].sValue));
        ;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 1219 "flatzinc/parser.yxx"
    { 
        ParserState *pp = static_cast<ParserState*>(parm);
        if (!pp->hadError) {
          pp->fg->solve((yyvsp[(2) - (3)].argVec));
        }
        delete (yyvsp[(2) - (3)].argVec);
      ;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 1227 "flatzinc/parser.yxx"
    { 
        ParserState *pp = static_cast<ParserState*>(parm);
        if (!pp->hadError) {
          if ((yyvsp[(3) - (4)].bValue))
            pp->fg->minimize((yyvsp[(4) - (4)].iValue),(yyvsp[(2) - (4)].argVec));
          else
            pp->fg->maximize((yyvsp[(4) - (4)].iValue),(yyvsp[(2) - (4)].argVec));
        }
        delete (yyvsp[(2) - (4)].argVec);
      ;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 1244 "flatzinc/parser.yxx"
    { (yyval.oSet) = Option<AST::SetLit* >::none(); ;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 1246 "flatzinc/parser.yxx"
    { (yyval.oSet) = Option<AST::SetLit* >::some(new AST::SetLit(*(yyvsp[(2) - (3)].setValue))); ;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 1248 "flatzinc/parser.yxx"
    { 
        (yyval.oSet) = Option<AST::SetLit* >::some(new AST::SetLit((yyvsp[(1) - (3)].iValue), (yyvsp[(3) - (3)].iValue)));
      ;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 1254 "flatzinc/parser.yxx"
    { (yyval.oSet) = Option<AST::SetLit* >::none(); ;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 1256 "flatzinc/parser.yxx"
    { bool haveTrue = false;
        bool haveFalse = false;
        for (int i=(yyvsp[(2) - (4)].setValue)->size(); i--;) {
          haveTrue |= ((*(yyvsp[(2) - (4)].setValue))[i] == 1);
          haveFalse |= ((*(yyvsp[(2) - (4)].setValue))[i] == 0);
        }
        delete (yyvsp[(2) - (4)].setValue);
        (yyval.oSet) = Option<AST::SetLit* >::some(
          new AST::SetLit(!haveFalse,haveTrue));
      ;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 1277 "flatzinc/parser.yxx"
    { (yyval.setLit) = new AST::SetLit(*(yyvsp[(2) - (3)].setValue)); ;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 1279 "flatzinc/parser.yxx"
    { (yyval.setLit) = new AST::SetLit((yyvsp[(1) - (3)].iValue), (yyvsp[(3) - (3)].iValue)); ;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 1285 "flatzinc/parser.yxx"
    { (yyval.setValue) = new vector<int>(0); ;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 1287 "flatzinc/parser.yxx"
    { (yyval.setValue) = (yyvsp[(1) - (2)].setValue); ;}
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 1291 "flatzinc/parser.yxx"
    { (yyval.setValue) = new vector<int>(1); (*(yyval.setValue))[0] = (yyvsp[(1) - (1)].iValue); ;}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 1293 "flatzinc/parser.yxx"
    { (yyval.setValue) = (yyvsp[(1) - (3)].setValue); (yyval.setValue)->push_back((yyvsp[(3) - (3)].iValue)); ;}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 1297 "flatzinc/parser.yxx"
    { (yyval.setValue) = new vector<int>(0); ;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 1299 "flatzinc/parser.yxx"
    { (yyval.setValue) = (yyvsp[(1) - (2)].setValue); ;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 1303 "flatzinc/parser.yxx"
    { (yyval.setValue) = new vector<int>(1); (*(yyval.setValue))[0] = (yyvsp[(1) - (1)].iValue); ;}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 1305 "flatzinc/parser.yxx"
    { (yyval.setValue) = (yyvsp[(1) - (3)].setValue); (yyval.setValue)->push_back((yyvsp[(3) - (3)].iValue)); ;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 1309 "flatzinc/parser.yxx"
    { (yyval.floatSetValue) = new vector<double>(0); ;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 1311 "flatzinc/parser.yxx"
    { (yyval.floatSetValue) = (yyvsp[(1) - (2)].floatSetValue); ;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 1315 "flatzinc/parser.yxx"
    { (yyval.floatSetValue) = new vector<double>(1); (*(yyval.floatSetValue))[0] = (yyvsp[(1) - (1)].dValue); ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 1317 "flatzinc/parser.yxx"
    { (yyval.floatSetValue) = (yyvsp[(1) - (3)].floatSetValue); (yyval.floatSetValue)->push_back((yyvsp[(3) - (3)].dValue)); ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 1321 "flatzinc/parser.yxx"
    { (yyval.setValueList) = new vector<AST::SetLit>(0); ;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 1323 "flatzinc/parser.yxx"
    { (yyval.setValueList) = (yyvsp[(1) - (2)].setValueList); ;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 1327 "flatzinc/parser.yxx"
    { (yyval.setValueList) = new vector<AST::SetLit>(1); (*(yyval.setValueList))[0] = *(yyvsp[(1) - (1)].setLit); delete (yyvsp[(1) - (1)].setLit); ;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 1329 "flatzinc/parser.yxx"
    { (yyval.setValueList) = (yyvsp[(1) - (3)].setValueList); (yyval.setValueList)->push_back(*(yyvsp[(3) - (3)].setLit)); delete (yyvsp[(3) - (3)].setLit); ;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 1337 "flatzinc/parser.yxx"
    { (yyval.argVec) = new AST::Array((yyvsp[(1) - (1)].arg)); ;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 1339 "flatzinc/parser.yxx"
    { (yyval.argVec) = (yyvsp[(1) - (3)].argVec); (yyval.argVec)->append((yyvsp[(3) - (3)].arg)); ;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1343 "flatzinc/parser.yxx"
    { (yyval.arg) = (yyvsp[(1) - (1)].arg); ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1345 "flatzinc/parser.yxx"
    { (yyval.arg) = (yyvsp[(2) - (3)].argVec); ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1349 "flatzinc/parser.yxx"
    { (yyval.oArg) = Option<AST::Node*>::none(); ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1351 "flatzinc/parser.yxx"
    { (yyval.oArg) = Option<AST::Node*>::some((yyvsp[(2) - (2)].arg)); ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1355 "flatzinc/parser.yxx"
    { (yyval.arg) = new AST::BoolLit((yyvsp[(1) - (1)].iValue)); ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1357 "flatzinc/parser.yxx"
    { (yyval.arg) = new AST::IntLit((yyvsp[(1) - (1)].iValue)); ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1359 "flatzinc/parser.yxx"
    { (yyval.arg) = new AST::FloatLit((yyvsp[(1) - (1)].dValue)); ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1361 "flatzinc/parser.yxx"
    { (yyval.arg) = (yyvsp[(1) - (1)].setLit); ;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1363 "flatzinc/parser.yxx"
    { 
        vector<int> as;
        ParserState* pp = static_cast<ParserState*>(parm);
        if (pp->intvararrays.get((yyvsp[(1) - (1)].sValue), as)) {
          AST::Array *ia = new AST::Array(as.size());
          for (int i=as.size(); i--;)
            ia->a[i] = new AST::IntVar(as[i]);
          (yyval.arg) = ia;
        } else if (pp->boolvararrays.get((yyvsp[(1) - (1)].sValue), as)) {
          AST::Array *ia = new AST::Array(as.size());
          for (int i=as.size(); i--;)
            ia->a[i] = new AST::BoolVar(as[i]);
          (yyval.arg) = ia;
        } else if (pp->setvararrays.get((yyvsp[(1) - (1)].sValue), as)) {
          AST::Array *ia = new AST::Array(as.size());
          for (int i=as.size(); i--;)
            ia->a[i] = new AST::SetVar(as[i]);
          (yyval.arg) = ia;
        } else {
          std::vector<int> is;
          std::vector<AST::SetLit> isS;
          int ival = 0;
          bool bval = false;
          if (pp->intvalarrays.get((yyvsp[(1) - (1)].sValue), is)) {
            AST::Array *v = new AST::Array(is.size());
            for (int i=is.size(); i--;)
              v->a[i] = new AST::IntLit(is[i]);
            (yyval.arg) = v;
          } else if (pp->boolvalarrays.get((yyvsp[(1) - (1)].sValue), is)) {
            AST::Array *v = new AST::Array(is.size());
            for (int i=is.size(); i--;)
              v->a[i] = new AST::BoolLit(is[i]);
            (yyval.arg) = v;
          } else if (pp->setvalarrays.get((yyvsp[(1) - (1)].sValue), isS)) {
            AST::Array *v = new AST::Array(isS.size());
            for (int i=isS.size(); i--;)
              v->a[i] = new AST::SetLit(isS[i]);
            (yyval.arg) = v;            
          } else if (pp->intvals.get((yyvsp[(1) - (1)].sValue), ival)) {
            (yyval.arg) = new AST::IntLit(ival);
          } else if (pp->boolvals.get((yyvsp[(1) - (1)].sValue), bval)) {
            (yyval.arg) = new AST::BoolLit(bval);
          } else {
            (yyval.arg) = getVarRefArg(pp,(yyvsp[(1) - (1)].sValue));
          }
        }
        free((yyvsp[(1) - (1)].sValue));
      ;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1412 "flatzinc/parser.yxx"
    { 
        ParserState* pp = static_cast<ParserState*>(parm);
        int i = -1;
        yyassert(pp, (yyvsp[(3) - (4)].arg)->isInt(i), "Non-integer array index.");
        if (!pp->hadError)
          (yyval.arg) = getArrayElement(static_cast<ParserState*>(parm),(yyvsp[(1) - (4)].sValue),i);
        else
          (yyval.arg) = new AST::IntLit(0); // keep things consistent
        free((yyvsp[(1) - (4)].sValue));
      ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1425 "flatzinc/parser.yxx"
    { (yyval.argVec) = new AST::Array(0); ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1427 "flatzinc/parser.yxx"
    { (yyval.argVec) = (yyvsp[(1) - (2)].argVec); ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1431 "flatzinc/parser.yxx"
    { (yyval.argVec) = new AST::Array((yyvsp[(1) - (1)].arg)); ;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1433 "flatzinc/parser.yxx"
    { (yyval.argVec) = (yyvsp[(1) - (3)].argVec); (yyval.argVec)->append((yyvsp[(3) - (3)].arg)); ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1441 "flatzinc/parser.yxx"
    { 
        ParserState *pp = static_cast<ParserState*>(parm);
        if (!pp->intvarTable.get((yyvsp[(1) - (1)].sValue), (yyval.iValue))) {
          pp->err << "Error: unknown integer variable " << (yyvsp[(1) - (1)].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
        }
        free((yyvsp[(1) - (1)].sValue));
      ;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1452 "flatzinc/parser.yxx"
    {
        vector<int> tmp;
        ParserState *pp = static_cast<ParserState*>(parm);
        if (!pp->intvararrays.get((yyvsp[(1) - (4)].sValue), tmp)) {
          pp->err << "Error: unknown integer variable array " << (yyvsp[(1) - (4)].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
        }
        if ((yyvsp[(3) - (4)].iValue) == 0 || static_cast<unsigned int>((yyvsp[(3) - (4)].iValue)) > tmp.size()) {
          pp->err << "Error: array index out of bounds for array " << (yyvsp[(1) - (4)].sValue)
                  << " in line no. "
                  << yyget_lineno(pp->yyscanner) << std::endl;
          pp->hadError = true;
        } else {
          (yyval.iValue) = tmp[(yyvsp[(3) - (4)].iValue)-1];
        }
        free((yyvsp[(1) - (4)].sValue));
      ;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1482 "flatzinc/parser.yxx"
    { (yyval.argVec) = NULL; ;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1484 "flatzinc/parser.yxx"
    { (yyval.argVec) = (yyvsp[(1) - (1)].argVec); ;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1488 "flatzinc/parser.yxx"
    { (yyval.argVec) = new AST::Array((yyvsp[(2) - (2)].arg)); ;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1490 "flatzinc/parser.yxx"
    { (yyval.argVec) = (yyvsp[(1) - (3)].argVec); (yyval.argVec)->append((yyvsp[(3) - (3)].arg)); ;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1494 "flatzinc/parser.yxx"
    { 
        (yyval.arg) = new AST::Call((yyvsp[(1) - (4)].sValue), AST::extractSingleton((yyvsp[(3) - (4)].arg))); free((yyvsp[(1) - (4)].sValue));
      ;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1498 "flatzinc/parser.yxx"
    { (yyval.arg) = (yyvsp[(1) - (1)].arg); ;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 1502 "flatzinc/parser.yxx"
    { (yyval.arg) = new AST::Array((yyvsp[(1) - (1)].arg)); ;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 1504 "flatzinc/parser.yxx"
    { (yyval.arg) = (yyvsp[(1) - (3)].arg); (yyval.arg)->append((yyvsp[(3) - (3)].arg)); ;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 1508 "flatzinc/parser.yxx"
    { (yyval.arg) = (yyvsp[(1) - (1)].arg); ;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 1510 "flatzinc/parser.yxx"
    { (yyval.arg) = (yyvsp[(2) - (3)].arg); ;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1514 "flatzinc/parser.yxx"
    { (yyval.arg) = new AST::BoolLit((yyvsp[(1) - (1)].iValue)); ;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 1516 "flatzinc/parser.yxx"
    { (yyval.arg) = new AST::IntLit((yyvsp[(1) - (1)].iValue)); ;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 1518 "flatzinc/parser.yxx"
    { (yyval.arg) = new AST::FloatLit((yyvsp[(1) - (1)].dValue)); ;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 1520 "flatzinc/parser.yxx"
    { (yyval.arg) = (yyvsp[(1) - (1)].setLit); ;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 1522 "flatzinc/parser.yxx"
    { 
        vector<int> as;
        ParserState* pp = static_cast<ParserState*>(parm);
        if (pp->intvararrays.get((yyvsp[(1) - (1)].sValue), as)) {
          AST::Array *ia = new AST::Array(as.size());
          for (int i=as.size(); i--;)
            ia->a[i] = new AST::IntVar(as[i]);
          (yyval.arg) = ia;
        } else if (pp->boolvararrays.get((yyvsp[(1) - (1)].sValue), as)) {
          AST::Array *ia = new AST::Array(as.size());
          for (int i=as.size(); i--;)
            ia->a[i] = new AST::BoolVar(as[i]);
          (yyval.arg) = ia;
        } else if (pp->setvararrays.get((yyvsp[(1) - (1)].sValue), as)) {
          AST::Array *ia = new AST::Array(as.size());
          for (int i=as.size(); i--;)
            ia->a[i] = new AST::SetVar(as[i]);
          (yyval.arg) = ia;
        } else {
          std::vector<int> is;
          int ival = 0;
          bool bval = false;
          if (pp->intvalarrays.get((yyvsp[(1) - (1)].sValue), is)) {
            AST::Array *v = new AST::Array(is.size());
            for (int i=is.size(); i--;)
              v->a[i] = new AST::IntLit(is[i]);
            (yyval.arg) = v;
          } else if (pp->boolvalarrays.get((yyvsp[(1) - (1)].sValue), is)) {
            AST::Array *v = new AST::Array(is.size());
            for (int i=is.size(); i--;)
              v->a[i] = new AST::BoolLit(is[i]);
            (yyval.arg) = v;
          } else if (pp->intvals.get((yyvsp[(1) - (1)].sValue), ival)) {
            (yyval.arg) = new AST::IntLit(ival);
          } else if (pp->boolvals.get((yyvsp[(1) - (1)].sValue), bval)) {
            (yyval.arg) = new AST::BoolLit(bval);
          } else {
            (yyval.arg) = getVarRefArg(pp,(yyvsp[(1) - (1)].sValue),true);
          }
        }
        free((yyvsp[(1) - (1)].sValue));
      ;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1565 "flatzinc/parser.yxx"
    { 
        ParserState* pp = static_cast<ParserState*>(parm);
        int i = -1;
        yyassert(pp, (yyvsp[(3) - (4)].arg)->isInt(i), "Non-integer array index.");
        if (!pp->hadError)
          (yyval.arg) = getArrayElement(static_cast<ParserState*>(parm),(yyvsp[(1) - (4)].sValue),i);
        else
          (yyval.arg) = new AST::IntLit(0); // keep things consistent
        free((yyvsp[(1) - (4)].sValue));
      ;}
    break;



/* Line 1455 of yacc.c  */
#line 3563 "flatzinc/parser.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (parm, YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (parm, yymsg);
	  }
	else
	  {
	    yyerror (parm, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, parm);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, parm);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (parm, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, parm);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, parm);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



