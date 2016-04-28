/*
 * abstract.h
 *
 * Defines Abstract, a subclass of TmkCore
 * 
 *
 * part of the test suite for the TiVo ref-counting
 * rules added to g++.
 *
 *
 *
 */

#ifndef __ABSTRACT_H__
#define __ABSTRACT_H__

#include "tmkCore.h"

template <class T>
class AbstractHolder
{
  public:
    void testThatLocalsInTemplatedInlinesAreChecked( T* pAbs ) {
	T* pArgAbs = pAbs;   // breaks if T is-a TmkCore
    }
  private:
    T* holdingM;   // breaks rules if T is a TmkCore or a subclass of TmkCore!
};

class Abstract: public TmkCore
{
  public:
    Abstract();
    Abstract( const Abstract& );

  protected:
    ~Abstract();

  private:
    Abstract& operator=( const Abstract& );
    bool operator==( const Abstract& );
};

#endif // __ABSTRACT_H__
