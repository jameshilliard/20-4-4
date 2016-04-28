/*
 * unrelated.h
 *
 * Defines Unrelated, *not* a subclass of TmkCore
 * 
 *
 * part of the test suite for the TiVo ref-counting
 * rules added to g++.
 *
 *
 *
 */

#ifndef __UNRELATED_H__
#define __UNRELATED_H__

#include "tmkCore.h"

class Abstract;
class Recording;

class SomeRandomBaseClass
{
};

class Unrelated: public SomeRandomBaseClass
{
  public:
    Unrelated();
    Unrelated( const Unrelated& );

    // returns: a Abstract (indirect subclass of TmkCore)
    Abstract* getAbstract();		/* breaks rules, but might not!
					 * catch if haven't seen Abstract.h
					 */

    // returns: a Recording (indirect subclass of TmkCore)
    Recording* getRecording();		/* breaks rules, but might not!
					 * catch if haven't seen Recording.h
					 */

    class nestedInUnrelated {
      private:
	Abstract* nestedsAbstractM;     /* breaks rules, but might not!
					 * catch if haven't seen Recording.h
					 */
    };

  protected:
    ~Unrelated();

  private:
    Unrelated& operator=( const Unrelated& );
    bool operator==( const Unrelated& );
};

#endif // __UNRELATED_H__
