#include "tmkCore.h"
#include "ref.h"

ref<TmkCore> Bad( TmkCore* pCore )
{
    pCore = new TmkCore();   // don't assign to "reffable*"

    ref<TmkCore>    pA = pCore;
    nonRef<TmkCore> pB = pCore;

    pCore = pA; // don't assign to "reffable*"
    pCore = pB; // don't assign to "reffable*"

    return pCore;
}

void Ok( TmkCore* pCore, ref<TmkCore> pRefCore )
{
    ref<TmkCore>    pA = pCore;
    nonRef<TmkCore> pB = pCore;

    pA = pCore;
    pB = pCore;

    pA = pB;
    pB = pA;

    pRefCore = pB;
    pRefCore = pA;
}

int main()
{
    return 0;
};
