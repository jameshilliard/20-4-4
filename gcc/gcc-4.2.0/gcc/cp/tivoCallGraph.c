#include <assert.h>
#include <stdlib.h>  

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include <stdio.h>
#include "cp/cp-tree.h"
#include "tivoDeclIter.h"
#include "tivoCallGraph.h"
#include "tivoTable.h"
#include "tivoUtil.h"
#include "langhooks.h"
/* #include "toplev.h" */

#include <string.h>

static FILE* pCgOut = NULL;
static int   fMustClose = 0;

static tree my_skip_artificial_parms_for PARAMS ((tree, tree));
static int my_copy_args_p PARAMS ((tree));
static const char* function_category PARAMS ((tree));
static int DemangleNames PARAMS ((void));
static FILE* GetCgOut PARAMS ((void));
static const char* GetHumanName PARAMS ((tree));
static const char* GetMangledName PARAMS ((tree));
static const char* GetName PARAMS ((tree));
static int GetVtableIndex PARAMS ((tree));
static const char* tivoGetFunctionName PARAMS ((tree));
static tree GetArg PARAMS ((tree, enum tree_code, int));
static const char* Indent PARAMS ((int));
static int tivoGenClassGraph PARAMS ((tree, int));

//#define CG_DEBUG

/* Given a FUNCTION_DECL FN and a chain LIST, skip as many elements of LIST
   as there are artificial parms in FN.  */

static tree
my_skip_artificial_parms_for (tree fn, tree list)
{
  if (DECL_NONSTATIC_MEMBER_FUNCTION_P (fn))
    list = TREE_CHAIN (list);
  else
    return list;

  if (DECL_HAS_IN_CHARGE_PARM_P (fn))
    list = TREE_CHAIN (list);
  if (DECL_HAS_VTT_PARM_P (fn))
    list = TREE_CHAIN (list);
  return list;
}
/* D is a constructor or overloaded `operator='.  Returns non-zero if
   D's arguments allow it to be a copy constructor, or copy assignment
   operator.  */

static int
my_copy_args_p (tree d)
{
  tree t;

  if (!DECL_FUNCTION_MEMBER_P (d))
    return 0;

  t = my_skip_artificial_parms_for (d, TYPE_ARG_TYPES (TREE_TYPE (d)));
  if (t && TREE_CODE (TREE_VALUE (t)) == REFERENCE_TYPE
      && (TYPE_MAIN_VARIANT (TREE_TYPE (TREE_VALUE (t)))
          == DECL_CONTEXT (d))
      && (TREE_CHAIN (t) == NULL_TREE
          || TREE_CHAIN (t) == void_list_node
          || TREE_PURPOSE (TREE_CHAIN (t))))
    return 1;
  return 0;
}
/* Copied from cp/error.c... */
/* Returns a description of FUNCTION using standard terminology.  */
static const char *
function_category (tree fn)
{
  if (DECL_FUNCTION_MEMBER_P (fn))
      {
        if (DECL_STATIC_FUNCTION_P (fn))
          return "static member function";
        else if (DECL_CONSTRUCTOR_P (fn) && my_copy_args_p(fn)) {
          if (!strcmp(tivoGetId(fn), "__base_ctor"))
            return "base copy constructor";
          else if (!strcmp(tivoGetId(fn), "__comp_ctor"))
            return "complete copy constructor";
          else
            return "copy constructor";
        } else if (DECL_CONSTRUCTOR_P (fn)) {
          if (!strcmp(tivoGetId(fn), "__base_ctor"))
            return "base constructor";
          else if (!strcmp(tivoGetId(fn), "__comp_ctor"))
            return "complete constructor";
          else
            return "constructor";
        }  else if (DECL_DESTRUCTOR_P (fn))
          return "destructor";
        else
          return "member function";
      }
        else 
          return "function";
} 

static int DemangleNames(void)
{
    static int fSet = 0;
    static int fDemangle = 0;

    if ( !fSet )
    {
        fDemangle = ( getenv("TIVO_CALL_GRAPH_DEMANGLE") != NULL );
        fSet = 1;
    }

    return fDemangle;
}


static FILE* GetCgOut(void)
{
    static int   fSet = 0;
    static char* path = NULL;
    int fAppend = 0;

    if ( fSet )
        return pCgOut;

    /* we only want to compute this once... */
    fSet = 1;

    path = getenv( "TIVO_CALL_GRAPH" );
    fAppend = (getenv( "TIVO_CALL_GRAPH_APPEND" ) != NULL);

    if ( NULL == path )
        return pCgOut;

    if ( strcmp( path, "stdout" ) == 0 )
    {
        pCgOut = stdout;
    }
    else if ( strcmp( path, "stderr" ) == 0 )
    {
        pCgOut = stderr;
    }
    else
    {
        pCgOut = fopen( path, fAppend ? "a" : "w" );
        if ( NULL == pCgOut )
        {
            fprintf( stderr,
                     "CALL_GRAPH: can't open '%s' for call graph!\n",
                     path );
        }
        else
        {
            fMustClose = 1;
        }
    }
#ifdef CG_DEBUG
    fprintf( stderr, "CG path = %s (%x)\n", path, (int)pCgOut );
#endif

    return pCgOut;
}

void tivoCallGraphClose(void)
{
#ifdef CG_DEBUG
    fprintf( stderr, "tivoCallGraphClose()  %d %x\n", fMustClose, (int)pCgOut );
#endif
    if ( fMustClose && pCgOut != NULL )
    {
        fclose( pCgOut );
    }

    pCgOut = NULL;
}

static const char* GetHumanName( tree node )
{
    return (*lang_hooks.decl_printable_name)( node, 2 );
}

static const char* GetMangledName( tree node )
{
    tree mangledNode = DECL_ASSEMBLER_NAME( node );
    const char* mangled = mangledNode ? IDENTIFIER_POINTER( mangledNode ) : NULL;

    return mangled ? mangled : "NULL";
}

static char gnbuf[256];

static const char* GetName( tree node )
{
    if (TREE_CODE(node) == RECORD_TYPE)
    {
        return GetHumanName(node);
    }
    else if (DemangleNames())
    {
        snprintf(gnbuf, sizeof(gnbuf), "%s [%s]", GetMangledName(node),
            GetHumanName(node));
        return gnbuf;
    }
    else
    {
        return GetMangledName(node);
    }
}

static int GetVtableIndex( tree intConst )
{
    int vtblIndex = TREE_INT_CST_LOW(intConst);
    if (TREE_INT_CST_HIGH(intConst) != 0)
    {
        if ( GetCgOut() )
            fprintf( GetCgOut(), "CALL_GRAPH: vtable index seems too big %ld %lu!\n",
                     TREE_INT_CST_HIGH(intConst),
                     TREE_INT_CST_LOW(intConst) );
    }
    return vtblIndex;
}

const char* tivoGetId( tree node )
{
    if (DECL_P(node))
    {
        if ( DECL_NAME(node) )
            return IDENTIFIER_POINTER( DECL_NAME(node) );
        else if (TREE_CODE(node) == LABEL_DECL
                 && LABEL_DECL_UID (node) != -1)
            return "*label*";
        else
            return TREE_CODE (node) == CONST_DECL
                ? "*constant*" : "*other decl*";
    }
    else if (TYPE_P(node))
    {
        if ( TYPE_NAME(node) )
        {
            if ( TREE_CODE( TYPE_NAME(node) ) == IDENTIFIER_NODE )
            {
                return IDENTIFIER_POINTER( TYPE_NAME(node) );
            }
            else if (TREE_CODE (TYPE_NAME (node)) == TYPE_DECL
                     && DECL_NAME (TYPE_NAME (node)))
            {
                return IDENTIFIER_POINTER( DECL_NAME( TYPE_NAME( node ) ) );
            }
        }
    }

    if ( TREE_CODE(node) == IDENTIFIER_NODE )
    {
        return IDENTIFIER_POINTER(node);
    }

    return "*unknown*";
}

static const char* tivoGetFunctionName( tree function_decl)
{
    const char* name = "*topLevel*";

    if ( NULL != function_decl )
    {
        /* Allow functions to be nameless (such as artificial ones).  */
        if ( TREE_CODE(function_decl) == FUNCTION_DECL )
        {
            name = GetName(function_decl);
        } 
    }

    return name;    
}

/**
 * if expr is NULL, returns NULL
 * if expr isn't the given treeCode, prints an error, and returns NULL
 * otherwise, returns the ith argument.
 */
static tree GetArg( tree expr, enum tree_code treeCode, int i )
{
    enum tree_code gotCode = 0;
    if ( NULL == expr )
        return NULL_TREE;

    gotCode = TREE_CODE( expr );
    if ( gotCode != treeCode )
    {
        if ( GetCgOut() )
            fprintf( GetCgOut(), "CALL_GRAPH: expected type %s (%d), got %s (%d)\n",
                     tree_code_name[treeCode], treeCode,
                     tree_code_name[gotCode], gotCode );
        return NULL_TREE;
    }

    return TREE_OPERAND( expr, i );
}

void tivoReportFunctionCall( tree callee_function )
{
    const char* caller = "";
    const char* callee = "";
    tree function = NULL;

    if ( ! GetCgOut() )
    {
        return;
    }


    caller = tivoGetFunctionName( current_function_decl );

#ifdef CG_DEBUG
    if (TREE_CODE (current_function_decl) == OVERLOAD )
        fprintf(stderr, "tivoReportFunctionFunctionCall with OVERLOAD.\n");
#endif

    function = callee_function;

    if ( (TREE_CODE(function) == INDIRECT_REF) && 
         (TREE_OPERAND(function,0)) &&
         (TREE_CODE(TREE_OPERAND( function, 0)) == PLUS_EXPR) )
    {
        tree plusExpr    = GetArg( function,    INDIRECT_REF, 0 );
        tree compRef     = GetArg( plusExpr,     PLUS_EXPR, 0 );
        tree indirectRef = GetArg( compRef,     COMPONENT_REF, 0 );
        tree intConst    = GetArg( plusExpr,    PLUS_EXPR, 1 );

        if ( indirectRef && intConst )
        {
            tree type = TREE_TYPE( indirectRef );
            int vtblIndex = GetVtableIndex( intConst );
            if ( GetCgOut() )
                fprintf( GetCgOut(), "vcall {%s} --> {%s} %d\n",
                         caller, tivoGetId( type ), vtblIndex/4 );
        }
        else
        {
            if ( GetCgOut() )
                fprintf( GetCgOut(), "CALL_GRAPH: internal error!" );
        }

        return;
    } else if ( ( TREE_CODE(function) == INDIRECT_REF ) &&
                ( TREE_OPERAND(function, 0)) &&
                ( TREE_CODE(TREE_OPERAND(function, 0)) == COMPONENT_REF) )
    {
        tree compRef = GetArg( function, INDIRECT_REF, 0);
        tree indirectRef = GetArg( compRef, COMPONENT_REF, 0);
        tree intConst = GetArg( compRef, COMPONENT_REF, 1);
        if ( indirectRef && intConst )
        {
            tree type = TREE_TYPE( indirectRef );
            if ( GetCgOut() )
                fprintf( GetCgOut(), "vcall {%s} --> {%s} 0\n",
                         caller, tivoGetId( type ));
        }
        else
        {
            if ( GetCgOut() )
                fprintf( GetCgOut(), "CALL_GRAPH: internal error!" );
        }

        return;
    }

    if ( TREE_CODE (function) == ADDR_EXPR &&
         TREE_CODE (TREE_OPERAND (function, 0)) == FUNCTION_DECL )
    {
        function = TREE_OPERAND (function, 0);
    }

    if ( function )
    {
        if ( GetCgOut() )
            fprintf( GetCgOut(), "call {%s} ", caller);

        callee = tivoGetFunctionName( function );

        if ( GetCgOut() )
            fprintf( GetCgOut(), "--> {%s}\n", callee );
    }
}
 
/******************************
 *
 * Check the global scope...
 *
 */

static int nIndent = 0;

#define INDENT_DIFF 3

static const char* Indent( int nIndent )
{
#define BUFLEN 300
    static char buf[BUFLEN];
    static int  curIndent = -1;

    if ( curIndent != nIndent )
    {
        int i = 0;
        for( i=0; (i < nIndent) && (i < (BUFLEN-1)); i++ )
        {
            buf[i] = ' ';
        }
        buf[i] = '\0';
        curIndent = nIndent;
    }

    return buf;
#undef BUFLEN
}

static int tivoGenClassGraph( tree node, int fStart )
{
    /* get the node's code to dispatch below! */
    int nodeCode = TREE_CODE( node );

    assert( GetCgOut() != NULL && "should NOT have called!" );
#ifdef CG_DEBUG
    fprintf( stderr, "---> tivoGenClassGraph: %s(%s)\n",
             tree_code_name[nodeCode], tivoGetId(node) );
#endif

    if ( nodeCode == VAR_DECL )
    {
    }
    else if ( nodeCode == FIELD_DECL )
    {
    }
    else if ( nodeCode == FUNCTION_DECL )
    {
        tree vindex = DECL_VINDEX( node );
        int vtblIndex = -1;
        int fVirtual = 0;
        tree context = DECL_CONTEXT( node );
        const char* className = NULL;
        
        if ( vindex )
        {
            if ( TREE_CODE( vindex ) == INTEGER_CST )
            {
                vtblIndex = GetVtableIndex( vindex );
                fVirtual = 1;
            }
        }
        
        if ( NULL != context )
            className = GetName(context);
        
        fprintf( GetCgOut(), "%s", Indent( nIndent ) );

        if (DemangleNames())
        {
            if ( fVirtual )
                fprintf( GetCgOut(), "virtual<%d> " , vtblIndex);
            
            fprintf( GetCgOut(), "%s ", function_category(node));
            
        } 
        else
        {
            if ( fVirtual )
            {
                fprintf( GetCgOut(), "vfunction %d", vtblIndex );
            }
            else
            {
                fprintf( GetCgOut(), "function " );
            }
        }
        if ( NULL != className )
            fprintf( GetCgOut(), "in {%s} ", className );
            
        fprintf( GetCgOut(), "{%s}\n", tivoGetFunctionName(node) );
    }

    else if ( nodeCode == TYPE_DECL )
    {
    }
    else if ( nodeCode == RECORD_TYPE )
    {
        if ( fStart )
        {
            if ( NULL != node &&
                 tivoIsClassType(node) &&
                 TYPE_BINFO(node) != NULL)
            {
                fprintf( GetCgOut(), "record {%s}:",
                         GetName(node));
                {
#ifdef TMK_DISABLING_CALLGRAPH_FOR_NOW
                    int nBases = CLASSTYPE_N_BASECLASSES(node);
                    int iBase  = 0;
                    tree binfos = TYPE_BINFO_BASETYPES(node);
                    
                    for( iBase=0; iBase < nBases; iBase++ )
                    {
                        tree aBase = BINFO_TYPE( TREE_VEC_ELT( binfos, iBase ) );
                        fprintf( GetCgOut(), "{%s} ",
                                 GetName(aBase) );
                    }
#endif
                }
                fprintf( GetCgOut(), "\n" );
            }
        }
    }

    return 1;  /* we always recurse into subtrees! */
}

void tivoGenCallGraphForGlobal(void)
{
    if ( NULL != GetCgOut() ) {
        tivoIterateOverGlobal( tivoGenClassGraph );
    }
}


void tivoReportAddressOf( tree arg )
{
  if ( NULL == GetCgOut() )
    return;

  if ( FUNCTION_DECL == TREE_CODE( arg ) )
  {
    fprintf( GetCgOut(), "addressOf in {%s} --> ", 
      tivoGetFunctionName( current_function_decl ));
    fprintf( GetCgOut(), "{%s}\n", tivoGetFunctionName(arg) );
  }
}
