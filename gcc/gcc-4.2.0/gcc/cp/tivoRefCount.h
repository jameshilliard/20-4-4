/*
 * tivoRefCount.h
 *
 * This file contains declarations for TiVo's extensions
 * to gcc for checking our use of ref counted objects.
 *
 */

#ifndef __TIVO_REF_COUNT_H__
#define __TIVO_REF_COUNT_H__

#define tivoBool  int
#define tivoFalse 0
#define tivoTrue  1

const char* tivoGetId( tree node );

tivoBool tivoIsProbablyInstantiationOfRef( tree node );

/*
// effects: if tivoRefCounting warnings are enabled, causes warnings
//          for any violations found in this function/method call.  if
//          this is a function (not method) call, inst may be
//          NULL_TREE.
// note: this should be called on each function or method call to
//       ensure that each is checked, because they're not available at
//       the end of parsing!
*/
void tivoCheckCall( tree inst, tree func, tree params );

/*
// effects: if tivoRefCounting warnings are enabled, causes warnings
//          for any violations found in this declaration
// note: this should be called on non-global declarations to ensure
//       that they are checked, because they're not available at the
//       end of parsing!
*/
void tivoCheckDecl( tree decl );

/*
// effects: if tivoRefCounting warnings are enabled,
//          causes warnings for any violations found
//          in the global namespace.
// note: for best results, this should be called once,
//       after the whole translation unit's been parsed.
*/
void tivoCheckRefCountingRules(void);

/*
// effects: if lhs is parameter, this checks that it's not
//          a pointer to a ref-counted object type.  call it
//          on parameters you see assignements to.
// requires: lhs is a PARM_DECL
*/
void tivoCheckForAssignmentToRefCountedParameterType( tree lhs );

/*
// tivoCheckTemplateInstantiation(tree pre_expand, tree post_expand)
// Called to check the instantiation of a template.  pre_expand is the
// template tree and post_expand is the instiation.
*/
void tivoCheckTemplateInstantiation(tree pre_expand, tree post_expand);

/*
// List of ref<> templates we check against.
*/
static const char * const tivoRefList[] =
  {
    "ref<",
    "nonRef<",
    "constRef<",
    "constNonRef<"
  };

#endif /* __TIVO_REF_COUNT_H__ */
