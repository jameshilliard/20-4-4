
#include "tmkCore.h"
#include "ref.h"

class TvModelExpression : public TmkCore {};
class TvModelComparable {};
class TvModelStringPresenter {};
class TvModelBoolPresenter {};

typedef TvModelExpression TvModelSuper;

class TvModel : public TvModelSuper
{
private:
    typedef TvModelSuper Super;
};

template <class S>
class TvModelScalarAbstract :
    public TvModel, public TvModelComparable
{
public:
    virtual S Get() const = 0;
};

template <class I>
class TvModelScalarInt : public TvModelScalarAbstract<I>,
                         public TvModelStringPresenter,
                         public TvModelBoolPresenter
{
    static I Get(const TvModelExpression *pExpr);
};

template <class I>
I TvModelScalarInt<I>::Get(const TvModelExpression *pExpr)
{
    ref<TvModel> pModel;
    ref<TvModelScalarInt<I> > pScalarModel;
}


// Test complicated chain of multiple inheritence to TmkCore...

class GP : public TmkCore {};

template <class T> class P1 : public GP {};
template <class U> class C1 : public P1<U> {
    static void Get() {
        //ref< P1<U> > test1;
        ref< C1<U> > test2;
    }
};

