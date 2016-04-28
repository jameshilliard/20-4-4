/*
 * recording.h
 *
 * Defines Recording, a subclass of Abstract
 * 
 *
 * part of the test suite for the TiVo ref-counting
 * rules added to g++.
 *
 *
 *
 */

#ifndef __RECORDING_H__
#define __RECORDING_H__

#include "abstract.h"

class Recording: public Abstract
{
  public:
    Recording();
    Recording( const Recording& );

  protected:
    ~Recording();

  private:
    Recording& operator=( const Recording& );
    bool operator==( const Recording& );
};

#endif // __RECORDING_H__
