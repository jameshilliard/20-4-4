
#include "tmkCore.h"
#include "ref.h"

#define TLG_GLOBAL( ThisClass )                                 \
    private:                                                    \
        MyVariableType valueM;                                  \
                                                                \
    public:                                                     \
        ThisClass()                                             \
        {                                                       \
            /* valueM is initialized to 0 by the linker */      \
        }                                                       \
                                                                \
        ~ThisClass()                                            \
        {                                                       \
        }                                                       \
                                                                \
        MyVariableType& GetReference()                          \
        {                                                       \
            return valueM;                                      \
        }                                                       \
                                                                \
        MyVariableType * operator & ()                          \
        {                                                       \
            return & this->GetReference();                      \
        }                                                       \
                                                                \
        bool FAllocated()                                       \
        {                                                       \
            return true;                                        \
	}

#define TLG_REF_OR_NONREF()                                     \
       operator MyCoreType *() __attribute__((no_tivo_audits))  \
        {                                                       \
            return (MyCoreType*) this->GetReference();          \
        }                                                       \
	operator MyVariableType ()                              \
	{                                                       \
            return this->GetReference();                        \
	}                                                       \
      MyCoreType* operator ->() __attribute__((no_tivo_audits)) \
        {                                                       \
            return (MyCoreType*) this->GetReference();          \
        }                                                       \
        const MyVariableType& operator = ( MyCoreType * V )     \
        {                                                       \
            return this->GetReference() = V;                    \
        }

#define TmkDefineThreadGlobalClassRef( TT, N )  \
    template <class TT> class TmkTGR_##N {      \
      private:                                  \
        typedef ref<TT> MyVariableType;         \
        typedef TT MyCoreType;                  \
                                                \
      public:                                   \
        TLG_GLOBAL( TmkTGR_##N );               \
        TLG_REF_OR_NONREF();                    \
    }

#define TmkStaticThreadGlobalRef( TT, N )       \
    TmkDefineThreadGlobalClassRef( TT, N );     \
    static TmkTGR_##N<TT> N


class TmkActivityManager : public TmkCore {
  public:
    void Test() {}
};

TmkStaticThreadGlobalRef( TmkActivityManager, pActivityManagerG );

ref<TmkActivityManager> pActivityManager2G;

void Func1(TmkActivityManager *pArg)
{
}

void Test1()
{
    Func1(pActivityManagerG);                   // still valid...

    // These are three valid replacements for the accepted call above.
    // (1) and (2) are preferred since (3) just looks strange...

    // (1)...
    Func1(pActivityManagerG.GetReference());

    // (2)...
    ref<TmkActivityManager> pMgr = pActivityManagerG;
    Func1( pMgr );

    // (3)...
    Func1((&pActivityManagerG)->Get());

    // This is also legal as long as TmkStaticThreadGlobalRef's
    // operator->() method returns a ref<> not a TmkCore*!
    if ( pActivityManagerG != 0 )
        pActivityManagerG->Test();
}
