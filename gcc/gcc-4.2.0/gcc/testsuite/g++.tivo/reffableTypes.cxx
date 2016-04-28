#include "abstract.h"
#include "ref.h"

class Reffable;  // it is a TmkCore, but we haven't seen the declaration yet!

void Ok()
{
    ref<TmkCore>  pCore;
    ref<Abstract> pAbstract;
    ref<Reffable> pReffable;
}

//
// some non-TmkCore types...
//

class Bar
{
};

class TmkMempool
{
};

void Bad()
{
    ref<Bar>          pBar;
    ref<TmkMempool>   pMempool;
}

class ContainsSomeMembers
{
    ref<TmkCore>  pCore;
    ref<Abstract> pAbstract;
    ref<Reffable> pReffable;
    ref<Bar>          pBar;
    ref<TmkMempool>   pMempool;
};
