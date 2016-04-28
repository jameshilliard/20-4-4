#include <assert.h>
#include <stdlib.h>  /* for getenv() */
#include <stdio.h>
#include <stdarg.h>

#include "config.h"
#include "coretypes.h"
#include "system.h"
#include "tm.h"
#include "tree.h"
#include "cp-tree.h"
#include "toplev.h"
#include "diagnostic.h"

#include "tivoDeclIter.h"
#include "tivoRefCount.h"
#include "tivoTable.h"
#include "tivoUtil.h"

#include <string.h>

#if 0
#define TIVO_DEBUG
#endif

#ifdef TIVO_DEBUG
#define REF_DEBUG(args...)          { tivoPrintIndent(); fprintf(stderr, args); }
#define REF_DEBUG_TREE(t)           debug_tree(t)
#define REF_INDENT()                tivoIndent()
#define REF_UNINDENT()              tivoUnindent()
#else
#define REF_DEBUG(args...)          (void) 0
#define REF_DEBUG_TREE(t)           (void) 0
#define REF_INDENT()                (void) 0
#define REF_UNINDENT()              (void) 0
#endif

extern int warn_tivo;

/****************************************************************
 *
 * Helpers
 *
 ****************************************************************/

#ifdef TIVO_DEBUG

static int tivo_indent = 0;

static void tivoIndent(void)
{
    tivo_indent++;
}

static void tivoUnindent(void)
{
    tivo_indent--;
}

static void tivoPrintIndent(void)
{
    int i;
    for (i=0; i<tivo_indent; i++)
        fprintf(stderr, " ");
}

#endif

/*
// returns: true iff we should do tivoRefCount checking
//          to avoid the checkng,
//              setenv TIVO_GCC_REF_COUNT_DISABLE someValue
*/
static tivoBool tivoShouldCheck(void)
{
    return warn_tivo;
}

/*
 * Report a warning or error based on the current command line flags
 * and environment variable policy.  Setting
 * TIVO_GCC_REF_COUNT_NOT_ERROR will force any TiVo warnings to be
 * /only/ warnings, not errors, even if '-Werror' was provided on the
 * command line.
 */
static void tivoWarning( const char *msg, ... )
{
    /*
     * HACK: ugly reach into toplevel.c so we can temporarily disable
     *       warnings_are_errors if we want to.
     */
    extern int warnings_are_errors;

    static tivoBool isSet        = tivoFalse;
    static tivoBool preventError = tivoFalse;

    int oldWarnAreErrors = warnings_are_errors;


    if ( !isSet ) {
        if ( getenv( "TIVO_GCC_REF_COUNT_NOT_ERROR" ) != NULL )
            preventError = tivoTrue;
        isSet = tivoTrue;
    }

    if (preventError) { warnings_are_errors = 0; }

    {
        diagnostic_info diagnostic;
        va_list ap;

        /* This code lifted from diagnostic.c */
        va_start( ap, msg );
        diagnostic_set_info (&diagnostic, msg, &ap, input_location, DK_WARNING);
        diagnostic.option_index = OPT_Wtivo;
        report_diagnostic (&diagnostic);
        va_end( ap );
    }

    if (preventError) { warnings_are_errors = oldWarnAreErrors; }
}

/*
// returns: tivoTrue iff varDecl represents "this"
// requires: varDecl is a VAR_DECL
*/
static tivoBool tivoIsVarDeclThis( tree varDecl )
{
    static tree idOfThis = NULL;

    if ( idOfThis == NULL )
        idOfThis = get_identifier( "this" );

    assert( varDecl != NULL && (TREE_CODE( varDecl ) == VAR_DECL ));

    return ( idOfThis == DECL_NAME(varDecl) ) ? tivoTrue : tivoFalse;
}

tivoBool tivoIsProbablyInstantiationOfRef( tree node )
{
    const char *name;
    const char *s;
    unsigned int i;

    if (node == NULL_TREE) return tivoFalse;

    /* Handle typedefs by finding the /actual/ type... */
    node = canonical_type_variant(node);

    REF_DEBUG("tivoIsProbablyInstantiationOfRef: %s", tivoGetId(node));

    s = name = tivoGetId(node);

    /* Check if name is a template. */
    while (s[0] != '<' && s[0] != 0) s++;
    if (s[0] == 0) {
        REF_DEBUG(" returns false (start)\n");
        return tivoFalse;
    }
    while (s[0] != 0) s++;
    if (s == name || s[-1] != '>') {
        REF_DEBUG(" returns false (end)\n");
        return tivoFalse;
    }

    /* We only check template names listed in tivoRefList[].
     * See tivoRefCount.h. */

    /* Check if name is in tivoRefList[]. */
    for (i = 0; i < ARRAY_SIZE(tivoRefList); i++) {
      if (!strncmp(name, tivoRefList[i], strlen(tivoRefList[i]))) {
        REF_DEBUG(" returns true\n");
        return tivoTrue;
      }
    }

    REF_DEBUG(" returns false (no match)\n");
    return tivoFalse;
}

/* a nameTypePair is used to pair class names with
 * pointers to the types.
 */
struct _tivoNameTypePair {
    const char* name;
    tree        type;
};

typedef struct _tivoNameTypePair tivoNameTypePair;

/* returns: an array of nameTypePairs which store pointers to
 *          RECORD_TYPE trees for each ref counting base class.
 *          array is terminated by a nameTypePair with a NULL name.
 *
 * WARNING: if we have not yet found a definition for a given baseClass,
 *          it's type member will still be NULL.
 *
 * IMPL NOTE: I'm assuming that once we've seen a class, it's record doesn't change!
 */
static tivoNameTypePair* tivoGetRefCountingBaseClasses(void)
{
    static tivoNameTypePair refCountingBaseClasses[] = {
        { "TmkCore", NULL },
        { NULL, NULL }
    };
    static tivoBool foundAll = tivoFalse;

    /* if we haven't found all the types yet, look up missing ones! */
    if ( !foundAll ) {
        tivoBool          stillMissingOne = tivoFalse;
        tivoNameTypePair* scan            = NULL;

        for( scan = refCountingBaseClasses; scan->name; scan++ ) {
            if ( scan->type == NULL ) {
                /* this type's missing...look for it! */
                tree candidate = lookup_name( get_identifier( (char*) scan->name ) );
                if ( candidate != NULL ) {
                    if ( TREE_CODE(candidate) == TYPE_DECL ) {
                        /* candidate is usually a type declaration, but
                         * we want to store the actual type node!
                         */
                        candidate = TREE_TYPE(candidate);
                    }

                    if ( IS_AGGR_TYPE_CODE( TREE_CODE(candidate) ) ) {
                        /* only remember it if it's a class or union! */
                        scan->type = candidate;
                    }
                }
            }
            if ( scan->type == NULL ) {
                stillMissingOne = tivoTrue;
            }
        }

        /* if we're not missing any, we've found them all and can skip
         * the lookups next time!
         */
        foundAll = !stillMissingOne;
    }

    /* return what we've got -- caller knows we might have NULLs! */
    return refCountingBaseClasses;
}

static tivoBool tivoIsTypeComplete;

static tree tivo_inheritence_checker_r (tree binfo, void *data)
{
    (void) data;

    if ( !COMPLETE_TYPE_P(TREE_TYPE(binfo)) ) {
        tivoIsTypeComplete = tivoFalse;
        return dfs_skip_bases;
    }

    return NULL_TREE;
}

static tivoBool tivoEntireInheritenceHierarchyKnown( tree type )
{
    tivoIsTypeComplete = tivoTrue;
    dfs_walk_once( TYPE_BINFO(type), tivo_inheritence_checker_r, NULL, NULL );
    return tivoIsTypeComplete;
}

static tivoBool tivoHasRefCountingBaseClass( tree type )
{
    tivoNameTypePair* scan = NULL;

    if ( !tivoIsClassType( type ) ) {
        assert( tivoFalse && "see requirements!" );
        return tivoFalse;
    }

    /* I am the base class.  Goo goo g'joob. */
    if ( !TYPE_BINFO(type) ) {
        /* cannot be derived from TmkCore...! */
        return tivoFalse;
    }

    for( scan = tivoGetRefCountingBaseClasses(); scan->name; scan++ ) {
        if ( scan->type == NULL ) {
            /* we haven't seen a non-forward declaration for this puppy yet! */
            continue;
        }
        if ( DERIVED_FROM_P( scan->type, type ) ) {
            return tivoTrue;
        }
    }

    return tivoFalse;
}

static tivoBool tivoIsRefCountable( tree type )
{
    /* make sure we're working on a type! */
    if ( !TYPE_P(type) ) {
        assert( tivoFalse && "read requirements!" );
        return tivoFalse;
    }

    if ( !tivoIsClassType(type) ) {
        /* if it's not a class, how could it be refCounted? */
        return tivoFalse;
    }

    if ( tivoHasRefCountingBaseClass(type) ) {
        /* is a TmkCore type... */
        return tivoTrue;
    }

    return tivoFalse;
}

/****************************************************************
 *
 * Higher level local functions
 *
 ****************************************************************/

/*
 * Returns tivoTrue if given function declaration is a constructor or
 * assignment operator.
 */
static tivoBool tivoIsFuncCtorOrAssignOp( tree func )
{
    /* Special case: constructors return the type, but we don't want
       to complain about their return type! */
    {
        tivoBool isCtor = ( DECL_LANG_SPECIFIC(func) != NULL &&
                            DECL_CONSTRUCTOR_P(func) );
        if ( isCtor ) {
            REF_DEBUG( "tivoIsFuncCtorOrAssignOp: %s is a constructor!\n",
                       tivoGetId(func) );
            /* no audit for constructors... */
            return tivoTrue;
        }
    }

    /* Special case: assignment operators return the type, but we
       don't want to complain about their return type! */
    {
        tivoBool isAssignment = (!strcmp(tivoGetId(func), "operator="));
        if ( isAssignment ) {
            REF_DEBUG( "tivoIsFuncCtorOrAssignOp: %s is an assignment operator!\n",
                       tivoGetId(func) );
            /* no audit for assignment operators... */
            return tivoTrue;
        }
    }

    return tivoFalse;
}

/*
 * Returns tivoTrue if given function declaration is a dereference
 * operator.
 */
static tivoBool tivoIsFuncDerefOp( tree func )
{
    /* Special case: dereference operators return the type, but we
       don't want to complain about their return type! */
    {
        tivoBool isDeref = (!strcmp(tivoGetId(func), "operator->"));
        if ( isDeref ) {
            REF_DEBUG( "tivoIsFuncDerefOp: %s is a dereference operator!\n",
                       tivoGetId(func) );
            /* no audit for derefence operators... */
            return tivoTrue;
        }
    }

    return tivoFalse;
}

/*
 * Returns tivoTrue if given function declaration is an indexing
 * operator.
 */
static tivoBool tivoIsFuncIndexingOp( tree func )
{
    /* Special case: indexing operators return the type, but we don't
       want to complain about their return type! */
    {
        tivoBool isDeref = (!strcmp(tivoGetId(func), "operator[]"));
        if ( isDeref ) {
            REF_DEBUG( "tivoIsFuncIndexingOp: %s is an indexing operator!\n",
                       tivoGetId(func) );
            /* no audit for indexing operators... */
            return tivoTrue;
        }
    }

    return tivoFalse;
}

/*
 * Tree node must be a variable or field declaration.  Audit only
 * "fails" and returns tivoFalse iff the variable/field declaration is
 * a TmkCore (or derived) and it is not within a ref class.  Also
 * excludes case where tree is a variable declaration and the variable
 * is 'this'.
 */
static tivoBool tivoAuditCoreIsReffed( tree node )
{
    char *nuggetDecl;
    tree nuggetType = tivoGetNuggetType( TREE_TYPE(node) );
    int code = TREE_CODE(node);

    if ( code != VAR_DECL && code != FIELD_DECL ) {
        assert( tivoFalse && "see requirements!" );
        return tivoTrue;
    }

    REF_DEBUG("tivoAuditCoreIsReffed: %s, %s\n",
              tree_code_name[code], tivoGetId(node));
    REF_INDENT();

    /* Special case: it turns out that when you inline a method,
       "this" appears as a local variable.  Since you can't ref
       'this', we don't want to audit this case. */
    if ( code == VAR_DECL && tivoIsVarDeclThis( node ) ) {
        REF_DEBUG("ignore isThis case\n");
        /* reference to 'this' is okay... */
        goto done;
    }

    if ( TREE_CODE(nuggetType) == TEMPLATE_TYPE_PARM ) {
        REF_DEBUG("ignore not yet instantiated template decl\n");
        /* Not a TmkCore type, so okay... */
        goto done;
    }

    if ( !tivoIsRefCountable( nuggetType ) ) {
        REF_DEBUG("ignore non-TmkCore case\n");
        /* Not a TmkCore type, so okay... */
        goto done;
    }

    if (!DECL_NAME(node)) {
        REF_DEBUG("ignore non-user created object\n");
        /* Not a user code object, ignore... */
        goto done;
    }

    REF_DEBUG("NOT REF'D in var/field declaration...\n");
    REF_DEBUG_TREE(node);

    nuggetDecl = xstrdup(decl_as_string(nuggetType,TFF_PLAIN_IDENTIFIER));
    tivoWarning("TiVo-Ref: %s '%s' is of type '%s' and should "
                "be reference counted!",
                (code == VAR_DECL) ? "variable" : "field",
                tivoGetId(node), nuggetDecl);
    free(nuggetDecl);

    REF_UNINDENT();
    return tivoFalse;

  done:
    REF_UNINDENT();
    return tivoTrue;
}

/*
 * Tree node should be a variable, type, field or parameter
 * declaration, otherwise audit "succeeds" and returns tivoTrue.
 * Audit only "fails" and returns tivoFalse iff the
 * variable/type/field/parameter declaration is a ref class and the
 * variable type is *not* TmkCore (or derived).
 */
static tivoBool tivoAuditRefHasCoreParam( tree node )
{
    int code = TREE_CODE(node);
    tree type = TREE_TYPE(node);
    tree templateArgs = NULL;
    tree reffedType = NULL;

    if ( code != VAR_DECL   && code != TYPE_DECL &&
         code != FIELD_DECL && code != PARM_DECL ) {
        /* not a declaration type that we care about... */
        return tivoTrue;
    }

    REF_DEBUG("tivoAuditRefHasCoreParam: %s, %s\n",
              tree_code_name[code], tivoGetId(node));
    REF_INDENT();

    if ( TREE_CODE(type) != RECORD_TYPE ) {
        REF_DEBUG("ignore non-struct type\n");
        /* not a struct, so not a ref class... */
        goto done;
    }

    if ( !tivoIsProbablyInstantiationOfRef(type) ) {
        REF_DEBUG("ignore non-ref class\n");
        /* apparently not a ref class... */
        goto done;
    }

    if ( !TYPE_LANG_SPECIFIC(type) ) {
        REF_DEBUG("ignore non-C++ construct\n");
        /* not language specfic (C++)... */
        goto done;
    }

    if ( !CLASSTYPE_USE_TEMPLATE(type) ) {
        REF_DEBUG("ignore class, not template specialization\n");
        /* not a template, but ref classes are so... */
        goto done;
    }

    templateArgs = CLASSTYPE_TI_ARGS(type);

    if ( TREE_CODE(templateArgs) != TREE_VEC ||
         TREE_VEC_LENGTH(templateArgs) != 1 )
    {
        REF_DEBUG("ignore wrong number of template parameters\n");
        /* our ref classes have exactly one param, this one doesn't... */
        goto done;
    }

    reffedType = complete_type(TREE_VEC_ELT( templateArgs, 0 ));

    if ( !COMPLETE_TYPE_P(reffedType) ) {
        REF_DEBUG("ignore incomplete template parameter type\n");
        /* type is still incomplete, allow to pass (for now)... */
        goto done;
    }

    if (code == FIELD_DECL) {
        REF_DEBUG("tivoAuditRefHasCoreParam: field\n");
        REF_DEBUG("  reffedType:\n");
        REF_DEBUG_TREE(reffedType);
        REF_DEBUG("  node:\n");
        REF_DEBUG_TREE(node);
    }

    if ( !tivoEntireInheritenceHierarchyKnown(reffedType) ) {
        REF_DEBUG("ignore incomplete inheritence hierarchy\n");
        /* we cannot check now, assume okay... */
        goto done;
    }

    if ( tivoIsRefCountable(reffedType) )
    {
        REF_DEBUG("ignore TmkCore type parameter\n");
        /* type is a TmkCore, so that is okay... */
        goto done;
    }

    REF_DEBUG("BAD TYPE in ref class param...\n");
    REF_DEBUG_TREE(node);

    tivoWarning("TiVo-Ref: reference counting template class "
                "parameter '%s' must be a subtype of TmkCore!",
                tivoGetId(reffedType));

    REF_UNINDENT();
    return tivoFalse;

  done:

    REF_UNINDENT();
    return tivoTrue;
}

/*
 * Tree node should be a function declaration, otherwise audit
 * "succeeds" and returns tivoTrue.  Audit only "fails" and returns
 * tivoFalse iff the function declaration returns a TmkCore (or
 * derived) and it is *not* returned in a ref class.
 */
static tivoBool tivoAuditFuncCheckReturnType( tree node )
{
    char *nuggetDecl;
    int code = TREE_CODE(node);

    if ( code != FUNCTION_DECL ) {
        assert( tivoFalse && "see requirements!" );
        return tivoTrue;
    }

    REF_DEBUG("tivoAuditFuncCheckReturnType: %s, %s\n",
              tree_code_name[code], tivoGetId(node));
    REF_INDENT();

    /* Special cases: constructors and assignment and dereference
       operators return the type, but we don't want to complain about
       their return type! */
    if ( tivoIsFuncCtorOrAssignOp(node) )
    {
        REF_DEBUG("ignore constructor or assignment operator\n");
        goto done;
    }

    /* Check for special no TiVo audit attribute... */
    if ( DECL_NO_TIVO_AUDITS(node) ) {
        REF_DEBUG("ignore function with 'no_tivo_audits' attribute\n");
        goto done;
    }

    /* The rules don't apply to ref classes... */
    if (TREE_CODE(TREE_TYPE(node)) == METHOD_TYPE) {

        tree args = DECL_ARGUMENTS( node );
        if ( 0 != args ) {
            tree nuggetType = tivoGetNuggetType( TREE_TYPE(args) );
            if ( tivoIsProbablyInstantiationOfRef(nuggetType) ) {
                REF_DEBUG("ignore ref'd return type\n");
                goto done;
            }
        }
    }

    /* Finally, check that the return type is not a TmkCore... */
    {
        tree retType = TREE_TYPE(TREE_TYPE(node));
        tree nuggetType = tivoGetNuggetType(retType);

        if ( nuggetType == NULL ) {
            REF_DEBUG("ignore function with no return type\n");
            /* no return type means no problem... */
            goto done;
        }

        if ( !tivoIsRefCountable( nuggetType ) ) {
            REF_DEBUG("ignore non-TmkCore return type\n");
            /* return type is not a TmkCore type... */
            goto done;
        }

        nuggetDecl = xstrdup(decl_as_string(nuggetType,TFF_PLAIN_IDENTIFIER));
    }

    REF_DEBUG("BAD RETURN TYPE in function...\n");
    REF_DEBUG_TREE(node);

    tivoWarning("%JTiVo-Ref: the return type '%s' of function '%s' "
                "should be assigned to a reference counting "
                "template class!", node, nuggetDecl, tivoGetId(node));
    free(nuggetDecl);

    REF_UNINDENT();
    return tivoFalse;

  done:
    REF_UNINDENT();
    return tivoTrue;
}

/*
 * Audit a /declaration/.  Return tivoFalse if the audit "fails".  The
 * type of audit depends upon the type of tree node.
 */
static int tivoCheck( tree node, int fStart )
{
    int code;
    int recurse = 1;  /* we almost always recurse */

    if ( node == NULL ) {
        /* not sure when this happens...but it does! */
        return recurse;
    }

    /* get the node's code to dispatch below! */
    code = TREE_CODE( node );

    REF_DEBUG("tivoCheck[%d]: %s(%s)\n", fStart,
              tree_code_name[code], tivoGetId(node));
    REF_INDENT();

#if 0
    if (!fStart) {
        switch (code) {
          case VAR_DECL:
            REF_DEBUG("VAR_DECL:\n");
            REF_DEBUG_TREE(node);
            break;
          case FIELD_DECL:
            REF_DEBUG("FIELD_DECL:\n");
            REF_DEBUG_TREE(node);
            break;
          case TYPE_DECL:
            REF_DEBUG("TYPE_DECL:\n");
            REF_DEBUG_TREE(node);
            break;
          case PARM_DECL:
            REF_DEBUG("PARM_DECL:\n");
            REF_DEBUG_TREE(node);
            break;
          case FUNCTION_DECL:
            REF_DEBUG("FUNCTION_DECL:\n");
            REF_DEBUG_TREE(node);
            break;
          case RECORD_TYPE:
            REF_DEBUG("RECORD_TYPE:\n");
            REF_DEBUG_TREE(node);
            break;
          case TREE_VEC:
            REF_DEBUG("TREE_VEC:\n");
            REF_DEBUG_TREE(node);
            break;
          default:
            REF_DEBUG("<<UNKNOWN>>>:\n");
            REF_DEBUG_TREE(node);
            break;
        }
    }
#endif

    switch (code) {
      case VAR_DECL:
      case FIELD_DECL:

        /* Check that TmkCore vars/fields are ref'd... */
        if (!fStart)
            (void) tivoAuditCoreIsReffed( node );

        /* fall through...! */

      case PARM_DECL:
      case TYPE_DECL:

        /* Check that ref<>s have a TmkCore as their arg... */
        if (!fStart)
            (void) tivoAuditRefHasCoreParam( node );

        if ( TREE_CODE(TREE_TYPE(node)) == RECORD_TYPE &&
             tivoIsProbablyInstantiationOfRef(TREE_TYPE(node)) ) {
            /* Do not recurse on ref classes... */
            recurse = 0;
        }
        break;

      case FUNCTION_DECL:
        (void) tivoAuditFuncCheckReturnType( node );
        break;

      case RECORD_TYPE:
        if ( tivoIsProbablyInstantiationOfRef(node) ) {
            REF_DEBUG("  SHORT CIRCUIT TmkCore RECORD\n");
            recurse = 0;
        }
        break;

      case TREE_VEC:
        /* nothing yet */
        break;

      default:
        break;
    }

    REF_UNINDENT();
    return recurse;
}


static tivoBool tivoIsCallOnRefClass( tree node )
{
    int code = TREE_CODE(node);

    assert( code == CALL_EXPR );

    REF_DEBUG("tivoIsCallOnRefClass: %s, %s\n",
              tree_code_name[code], tivoGetId(node));
    REF_INDENT();

    {
        tree func = TREE_TYPE(TREE_TYPE(TREE_OPERAND(node,0)));
        if ( TREE_CODE(func) != METHOD_TYPE ) {
            REF_DEBUG("ignore non-method call\n");
            goto done;
        }
    }

    {
        tree arg1 = TREE_VALUE(TREE_OPERAND(node,1));
        tree nuggetType = tivoGetNuggetType( TREE_TYPE(arg1) );
        REF_DEBUG_TREE(arg1);
        REF_DEBUG_TREE(nuggetType);
        if ( ! tivoIsProbablyInstantiationOfRef( nuggetType ) ) {
            REF_DEBUG("is not ref class\n");
            goto done;
        }
    }

    REF_DEBUG("*is* ref class\n");
    REF_UNINDENT();
    return tivoTrue;

  done:
    REF_UNINDENT();
    return tivoFalse;
}

/*
 * Audit a function or method /call/.  Return tivoFalse if the audit
 * "fails" and one or more parameters is a TmkCore type that has been
 * assigned a function or method return value.
 */
static tivoBool tivoAuditCall( tree inst, tree func, tree args )
{
    tivoBool okay = tivoTrue;
    tree p = args;
    int argNum = 1;
    (void) inst;

    REF_DEBUG("tivoAuditCall: %s%s%s()\n",
              (inst == NULL_TREE) ? "" : tivoGetId(inst),
              (inst == NULL_TREE) ? "" : ".",
              (func == NULL_TREE) ? "null" : tivoGetId(func));
    REF_INDENT();

    if ( TREE_CODE(func) == FUNCTION_DECL &&
         tivoIsFuncCtorOrAssignOp(func) ) {
        REF_DEBUG("ignore constructor or assignment operator\n");
        /* no audit for constructors or assignment operators... */
        goto done;
    }

    if ( TREE_CODE(TREE_TYPE(func)) == METHOD_TYPE ) {
        /* skip first arg ('this' arg)... */
        p = TREE_CHAIN(p);
    }

    while ( p != NULL )
    {
        tivoBool argOkay = tivoTrue;
        tree arg = TREE_VALUE(p);
        int code = TREE_CODE(arg);
        tree op  = arg;

        /* this logic should really be hunting for any part of this
           expression that is computing a TmkCore type that isn't
           from a ref class.  we could possibly just hunt for a
           constructor that produces a TmkCore type... */
        while ( code == NOP_EXPR ) {
            op = TREE_OPERAND( op, 0 );
            code = TREE_CODE(op);
        }

        /* argument value comes from function/method call, so check it
         * if isn't being made on a ref base class... */
        if ( code == CALL_EXPR && ! tivoIsCallOnRefClass(op) )
        {
            tree retType = TREE_TYPE(arg);

            /* Check for special no TiVo audit attribute... */
            if ( TREE_CODE(TREE_OPERAND(arg,0)) == ADDR_EXPR ) {
                tree func = TREE_OPERAND(TREE_OPERAND(arg,0),0);
                if ( TREE_CODE(func) == FUNCTION_DECL &&
                     DECL_NO_TIVO_AUDITS(func) ) {
                    REF_DEBUG("ignore function with 'no_tivo_audits' attribute\n");
                    goto done;
                }
            }

            if ( retType != 0 )
            {
                /* check if return is ref countable type... */
                if ( tivoIsRefCountable( tivoGetNuggetType( retType ) ) )
                {
                    char* str = xstrdup( type_as_string( retType, 0 ) );
                    tivoWarning("TiVo-Ref: passing function return value "
                                "that is a pointer to reference counted "
                                "type '%s' as an argument!", str );
                    free( str );
                    argOkay = tivoFalse;
                }
            }
        }

        REF_DEBUG("arg #%d: %s (%s)\n", argNum, argOkay ? "pass" : "fail",
                  tree_code_name[code]);
        argNum++;

        if (!argOkay) {
            okay = tivoFalse;
        }

        p = TREE_CHAIN(p);
    }

  done:

    REF_UNINDENT();
    return okay;
}

/**********************************************************
 *
 * High-level checking routines!
 *
 **********************************************************/

extern int expanding_inline_function;

/*
 * Recursively check a /declaration/.  This will check all function,
 * variable, class, method and field declarations within this one.
 */
void tivoCheckDecl( tree decl )
{
    if ( !tivoShouldCheck() ) {
        return;
    }

    if ( expanding_inline_function > 0 ) {
        return;
    }

    REF_DEBUG("tivoCheckDecl: %s\n",
              (decl == NULL_TREE) ? "<<null?>>" : tivoGetId(decl));
    REF_INDENT();

    tivoIterateOver( &tivoCheck, decl );

    REF_UNINDENT();
}

/*
 * Check a function or method /call/; specifically, verify that the
 * actual parameters used do not violate our rules.
 */
void tivoCheckCall( tree inst, tree func, tree args )
{
    if ( !tivoShouldCheck() ) {
        return;
    }

    REF_DEBUG("tivoCheckCall: %s%s%s()\n",
              (inst == NULL_TREE) ? "" : tivoGetId(inst),
              (inst == NULL_TREE) ? "" : ".",
              (func == NULL_TREE) ? "null" : tivoGetId(func));
    REF_INDENT();

#if 0
    REF_DEBUG_TREE(func);
    REF_DEBUG_TREE(args);
#endif

    if (inst != NULL_TREE)
    {
        tree type = TREE_TYPE( inst );          /* instance type */
        if ( tivoIsProbablyInstantiationOfRef(type) ) {
            /* do not audit calls on ref classes themselves... */
            goto done;
        }
    }

    (void) tivoAuditCall( inst, func, args );

  done:
    REF_UNINDENT();
}

void tivoCheckTemplateInstantiation(tree pre, tree post)
{
    int code = TREE_CODE(pre);

    if (code != FUNCTION_DECL && code != TYPE_DECL) {
        return;
    }

    REF_DEBUG("tivoCheckTemplateInstantiation: %s, %s\n",
              tree_code_name[code],
              (post == NULL_TREE) ? "<<null?>>" : tivoGetId(post));
    REF_INDENT();

#if 0
    REF_DEBUG("preexpansion tree:\n");
    REF_DEBUG_TREE(pre);
    REF_DEBUG("postexpansion tree:\n");
    REF_DEBUG_TREE(post);
#endif

    if (code == FUNCTION_DECL)
    {
        /* Method decls are checked with the record type so don't
           check them here. */
        if (TREE_CODE(TREE_TYPE(pre)) != METHOD_TYPE)
        {
            REF_DEBUG("check (non-method) template function declaration...\n");
            REF_INDENT();
            tivoCheckDecl(post);
            REF_UNINDENT();
        }
    } else if (code == TYPE_DECL)
    {
         if ((TREE_CODE(TREE_TYPE(pre)) == RECORD_TYPE) &&
            !tivoIsProbablyInstantiationOfRef(TREE_TYPE(pre)))
         {
            REF_DEBUG("check (non-ref class) template class declaration...\n");
            REF_INDENT();
            tivoCheckDecl(post);
            REF_UNINDENT();
         }
    }

    REF_UNINDENT();
}

void tivoCheckForAssignmentToRefCountedParameterType( tree lhs )
{
    tree varType       = NULL;
    tree varNuggetType = NULL;

    if ( !tivoShouldCheck() ) {
        return;
    }

    if ( TREE_CODE( lhs ) != PARM_DECL )
    {
        /* we only check parameters here! */
        return;
    }

    REF_DEBUG("tivoCheckForAssignmentToRefCountedParameterType: %s(%s)\n", tree_code_name[TREE_CODE(lhs)],tivoGetId(lhs));

    varType       = TREE_TYPE( lhs );
    varNuggetType = tivoGetNuggetType( varType );

    if ( tivoIsRefCountable( varNuggetType ) )
    {
        char *nuggetDecl = xstrdup(decl_as_string(varNuggetType,TFF_PLAIN_IDENTIFIER));
        tivoWarning("TiVo-Ref: illegal assignment to a pointer to a "
                    "reference counted parameter '%s' that is of a type "
                    "'%s'; such assignments must be reference counted!",
                    tivoGetId( lhs ), nuggetDecl);
        /* REF_DEBUG_TREE( lhs ); */
        free(nuggetDecl);
    }
}

/* effects: checks the rules on everything in the global scope */
void tivoCheckRefCountingRules(void)
{
    /* Disable this global checking for now, since we catch things as
     * we go in my newer updated version.  I'm leaving this here in
     * case we find problems in the future that encourage us to
     * reconsider this tactic. -- tmk */
#if 0
    if ( !tivoShouldCheck() )
        return;

    REF_DEBUG("tivoCheckRefCountingRules\n");
    REF_INDENT();

    /* Trick diagnostics into outputting 'At global scope:' text... */
    current_function_decl = build_empty_stmt();
    diagnostic_set_last_function( global_dc );
    current_function_decl = NULL;

    tivoIterateOverGlobal( &tivoCheck );

    REF_UNINDENT();
#endif
}
