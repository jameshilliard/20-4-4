
#include "tmkCore.h"
#include "ref.h"

class Foo : public TmkCore
{
};

Foo* Fcn1()
{
    return 0;
}

void Fcn2( Foo* )
{
}

void Test()
{
    Fcn2( Fcn1() );            // error: passing TmkCore as pointer...
}

class Class
{
    void Test();
    Foo* Meth1() { return 0; }
    void Meth2(TmkCore*) {}

    Foo *pBad;
};

void Class::Test()
{
    Fcn2( Fcn1() );            // error: passing TmkCore as pointer...
    Meth2( Meth1() );          // error: passing TmkCore as pointer...

    Fcn2( ref<Foo>(Fcn1()) );                   // ok, ref counted...
    Meth2( ref<Foo>(Meth1()) );                 // ok, ref counted...
}
