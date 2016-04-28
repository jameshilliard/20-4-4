/////////////////////////////////////////////////////////////////////
//
// TmkDevice.h
//
// Description:
//	This class provides an abstraction for a Unix device driver.
//      It provides the familiar read/write/ioctl interface.
//      The primary reason for introducing this class is to provide
//      a tracing mechanism for the MOM output devices.
//
//      Given that these are wrappers for system calls I'm not too
//      worried about the extra function call overhead.
//
// Copyright (c) 2000 TiVo, Inc.
//
/////////////////////////////////////////////////////////////////////

#ifndef _TMK_DEVICE_H_
#define _TMK_DEVICE_H_

#include "tmkCore.h"

#include <sys/types.h>

class TmkDevice : public TmkCore {

public:

    TmkDevice(const char *path);
    ~TmkDevice();

    const char *GetPath() const { return pathM; }
    int GetFd() const { return fdM; }
    bool IsOpen() const { return (fdM >= 0); }
    long long getTimeStamp(void);
    
    // these functions duplicate the Unix device interface
    virtual bool    Open(int flags);
    virtual void    Close();
    // virtual ssize_t Read(void *buf, size_t count);
    virtual ssize_t Write(const void *buf, size_t count);
    virtual int     Ioctl(int request);
    virtual int     Ioctl(int request, int arg);
    virtual int     Ioctl(int request, void *argp);

    // tracing stuff
    enum TraceOp {
        TraceRead,
        TraceWrite,
        TraceIoctlNoArg,
        TraceIoctlIntArg,
        TraceIoctlPtrArg
    };
    
    union TraceArg {
        int i;
        void *p;
        long l;
        long long ll;
        struct {
            long la, lb;
        } lls;
    };
        
    struct TraceRec {
        TraceOp op;     // 4 bytes
        int request;    // 4 bytes
        int result;     // 4 bytes
        TraceArg arg;   // 8 bytes
        TraceArg time;  // 8 bytes
        int pad;        // 4 bytes
    };

protected:

    char *pathM;
    int fdM;

    int traceFdM;
    int streamFdM;
    int prevIoctlRequestM;
};

#endif // _TMK_DEVICE_H_

