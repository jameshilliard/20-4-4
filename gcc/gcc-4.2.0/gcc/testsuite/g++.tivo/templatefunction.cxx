#include "abstract.h"
#include "ref.h"

class NonReffable
{
};

template <class T>
T* EvilTemplateFunction(T* P)       // Return is illegal if T reffable.
{
    T* pLocal = P;		    // Should be illegal if T is reffable.
    ref<NonReffable> pNonReffable;  // Always illegal
    EvilTemplateFunction(new T);    // Illegal if T is reffable. XXX Not caught...
    return pLocal;		    
}

void EvilFunction()
{
    ref<NonReffable> pNonReffable;  // Always illegal
}

void
Testing()
{
    EvilTemplateFunction<Abstract> (new Abstract);
    EvilTemplateFunction<NonReffable> (new NonReffable);
}

int main()
{
    Testing();
}
