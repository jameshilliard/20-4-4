#include "scheduler.h"
#include "unrelated.h"

/////////////////////////////////////////////////
//
// globals
//
/////////////////////////////////////////////////

Recording* getSomeRecording()	// breaks rules!
{
    return NULL;
}

Abstract*	abstractG  = NULL;	// breaks rules!
Recording*	recordingG = NULL;	// breaks rules!


/////////////////////////////////////////////////
//
// statics
//
/////////////////////////////////////////////////

static Abstract*	abstractStatic  = NULL;    // breaks rules!
static Recording*	recordingStatic = NULL;    // breaks rules!

Abstract*		arrayOfAbstracts[10];      // breaks rules!
ref<Abstract>		arrayOfAbstractRefs[10];

Recording*		arrayOfRecordings[10];     // breaks rules!
ref<Recording>		arrayOfRecordingRefs[10];

AbstractHolder<Unrelated>	unrelatedHolder;
AbstractHolder<Abstract>	abstractHolder;    // breaks rules
AbstractHolder<Recording>	recordingHolder;   // breaks rules
AbstractHolder<Recording>	recordingHolder2;  /* breaks rules 
						    * just one warning for 
						    * AbstractHolder<Recording>?
						    */

static void forceInstantiationOfAbstractHolderMethods()
{
    unrelatedHolder.testThatLocalsInTemplatedInlinesAreChecked( NULL );
    recordingHolder.testThatLocalsInTemplatedInlinesAreChecked( NULL );
}

static Recording* getRandomRecording()		   // breaks rules!
{
    return NULL;
}

// from bug #3802:
//    inlining a function makes "this" look like a local variable.
//    make sure we don't cause a warning about it
static void testInliningAndThis( Recording* pRec )
{
    pRec->ref();  // don't complain about local variable "this"
}


class Foo
{
  public:
    Foo(): qlist(NULL) {
	Recording* pInlined;  /* breaks rules -- make sure we catch it, 
			       * even though this is inlined!
			       */
    }

  private:
    nonRef<Recording> qlist;
};

static void testInliningAndMemberVarInits()
{
    Foo dummy;  // don't complain about ptr (inside qlist's ctor!)
}

/////////////////////////////////////////////////
//
// static helpers
//
/////////////////////////////////////////////////

//
// Test the compiler by calling each of the methods
//

static bool maybe()
{
    return true;
}

static void shouldNoticeThatLocalsAreRawPointers()
{
    Scheduler sched;
    Abstract*  pAbstract = sched.getFavoriteAbstract();  // breaks rules!
    Recording* pFavorite = sched.getFavoriteRecording(); // breaks rules!

    if ( maybe() ) {
	Abstract* pScopedAbstract = NULL;	// breaks rules!
    }
    else {
	Abstract* pScopedAbstractElse = NULL;	// breaks rules!
    }
}

static void shouldBeOk()
{
    Scheduler sched;
    ref<Abstract>  pAbstract = sched.getRefToFavoriteAbstract(); 
    ref<Recording> pFavorite = sched.getRefToFavoriteRecording();
}

static void static_shouldNoticeThatLocalsAreRawPointers()
{
    Abstract*  pAbstract = Scheduler::getClassFavoriteAbstract();  // breaks rules!
    Recording* pFavorite = Scheduler::getClassFavoriteRecording(); // breaks rules!
}

static void static_shouldBeOk()
{
    ref<Abstract>  pAbstract = Scheduler::getClassRefToFavoriteAbstract(); 
    ref<Recording> pFavorite = Scheduler::getClassRefToFavoriteRecording();
}


/////////////////////////////////////////////////
//
// Scheduler
//
/////////////////////////////////////////////////

Abstract*	Scheduler::abstractS  = NULL;  // breaks rules!
Recording*	Scheduler::recordingS = NULL;  // breaks rules!


Scheduler::Scheduler():
    abstractM( NULL ),
    recordingM( NULL )
{
    shouldNoticeThatLocalsAreRawPointers();
}

Scheduler::Scheduler( const Scheduler& ):
    abstractM( NULL ),
    recordingM( NULL )
{
}

Scheduler::~Scheduler()
{
}

Abstract*	Scheduler::getFavoriteAbstract()	// breaks rules!
{
    return abstractM;
}

ref<Abstract>	Scheduler::getRefToFavoriteAbstract()
{
    return abstractM;
}

Recording*	Scheduler::getFavoriteRecording()	// breaks rules!
{
    return recordingS;
}

ref<Recording>	Scheduler::getRefToFavoriteRecording()
{
    return recordingS;
}

Abstract*	Scheduler::getClassFavoriteAbstract()	// breaks rules!
{
    return abstractS;
}

ref<Abstract>	Scheduler::getClassRefToFavoriteAbstract()
{
    return abstractS;
}

Recording*	Scheduler::getClassFavoriteRecording()	// breaks rules!
{
    return recordingS;
}

ref<Recording>	Scheduler::getClassRefToFavoriteRecording()
{
    return recordingS;
}
