/*
 * tmkCore.h
 *
 * Defines TmkCore, the base ref-counted class!
 * 
 *
 * part of the test suite for the TiVo ref-counting
 * rules added to g++.
 *
 *
 *
 */

#ifndef __TMKCORE_H__
#define __TMKCORE_H__

class TmkCore
{
  public:
    TmkCore();
    TmkCore( const TmkCore& );

    void ref()   { refCount++; }
    void unref() { if (--refCount == 0) delete this; }

  protected:
    virtual ~TmkCore();

  private:
    TmkCore& operator=( const TmkCore& );
    bool operator==( const TmkCore& );

    int refCount;
};

#endif // __TMKCORE_H__
