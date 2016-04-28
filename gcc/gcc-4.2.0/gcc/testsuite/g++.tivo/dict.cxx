/**
 * This is a distilled down version of the Trio Dict classes for ref
 * testing...
 */

#include "tmkCore.h"
#include "ref.h"

#define REF_GET Get

namespace trio
{
    typedef TmkCore Core;

    class Dict;
    class Tmk1Dict;
    typedef ref<Dict> PDict;
    typedef Tmk1Dict DictImpl;

    class Tmk1Dict
    {
      public:
        Tmk1Dict();
        bool Equals( const Dict* pOther ) const;
    };

    // For the most part, the Dict class itself is platform independent.  In
    // TMK, however, Dict must be a subclass of Core for ref-counting.
    class Dict : public Core
    {
      public:
        static PDict New();

        Dict();

        bool Equals(const Dict* pOther) const;
        bool Equals(PDict const & pOther) const;

        /**
         * Compares two dictionaries for equality: same set of field
         * names, with each field having the same set of values.
         */

        bool operator ==(const Dict& other) const;
        bool operator !=(const Dict& other) const {
            return !(*this == other);
        }

      private:
        DictImpl implM;
    };
}

using namespace trio;

Tmk1Dict::Tmk1Dict()
{
}

bool Tmk1Dict::Equals( const Dict* pOther ) const
{
    return true;
}

PDict Dict::New()
{
    PDict result(new Dict());
    PDict result2 = new Dict();
    ::ref<Dict> result3(new Dict());
    ::ref<Dict> result4 = new Dict();
    return result;
}

Dict::Dict()
{
}

bool Dict::Equals( const Dict* const pOther ) const 
{
    return implM.Equals(pOther);
}

bool Dict::Equals( const PDict& pOther ) const
{
    return this->Equals(dynamic_cast<trio::Dict*>(pOther.REF_GET()));
}

bool Dict::operator ==(const Dict& other) const {
    return true;
}

int main() {
    ref<Dict> pDict = new Dict();
    pDict->Equals( pDict.Get() );
    pDict->Equals( (Dict*) pDict.Get() );
}
