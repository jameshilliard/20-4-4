/*
 * scheduler.h
 *
 * Defines Scheduler, a class to bang on the rules
 * 
 *
 * part of the test suite for the TiVo ref-counting
 * rules added to g++.
 *
 *
 *
 */

#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include "recording.h"
#include "ref.h"

class Scheduler
{
  public:
    Scheduler();
    Scheduler( const Scheduler& );
    ~Scheduler();

    //
    // member methods...
    //

    // return a raw Abstract* (a direct subclass of TmkCore)
    Abstract*		getFavoriteAbstract();		// breaks rules!
    ref<Abstract>	getRefToFavoriteAbstract();

    // return a raw Recording* (an indirect subclass of TmkCore)
    Recording*		getFavoriteRecording();		// breaks rules!
    ref<Recording>	getRefToFavoriteRecording();

    //
    // static methods...
    //

    // return a raw Abstract* (a direct subclass of TmkCore)
    static Abstract*		getClassFavoriteAbstract();	     // breaks rules!
    static ref<Abstract>	getClassRefToFavoriteAbstract();

    // return a raw Recording* (an indirect subclass of TmkCore)
    static Recording*		getClassFavoriteRecording();	     // breaks rules!
    static ref<Recording>	getClassRefToFavoriteRecording();


  private:
    Scheduler& operator=( const Scheduler& );
    bool operator==( const Scheduler& );

    Abstract*		abstractM;		// breaks rules!!
    Recording*		recordingM;		// breaks rules!!

    nonRef<Abstract>	nonRefAbstractM;
    nonRef<Recording>	nonRefRecordingM;

    ref<Abstract>	refAbstractM;
    ref<Recording>	refRecordingM;

    static Abstract*	abstractS;		// breaks rules!
    static Recording*	recordingS;		// breaks rules!
};

#endif // __SCHEDULER_H__
