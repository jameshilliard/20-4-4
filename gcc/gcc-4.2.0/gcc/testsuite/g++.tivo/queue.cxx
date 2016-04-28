
#include "tmkCore.h"
#include "ref.h"

class TmkGenericQueue : public TmkCore
{
};

template <class Item>
class TmkQueue : public TmkGenericQueue
{
};

class Msg : public TmkCore
{
};

class MsgQueue
{
  public:
    void Test();
    static void ProcessQueue( TmkQueue<Msg> *q );

    ref< TmkQueue<Msg> > pQueueM;
};

void MsgQueue::Test()
{
    ProcessQueue( pQueueM );
}

void MsgQueue::ProcessQueue( TmkQueue<Msg> *q )
{
    (void) q;
}

int main()
{
    MsgQueue m;
    m.Test();
}
