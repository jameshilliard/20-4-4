#include "tmkCore.h"
#include "ref.h"

void Foo(TmkCore*)
{
}

ref<TmkCore> Bar()
{
    ref<TmkCore> pRef = new TmkCore();
    return pRef;
}

void mikeysTest()
{
    Foo( Bar() );   // should not fail...
}
