/////////////////////////////////////////////////////////////////////
//
// TmkDevice.C
//
// Copyright (c) 2000 TiVo, Inc.
//
/////////////////////////////////////////////////////////////////////

#include "TmkDevice.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

long long TmkDevice::getTimeStamp(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == 0) {
        return (long long)tv.tv_sec * 1000000 + (long long)tv.tv_usec;
    } else {
        return 0;
    }
}

TmkDevice::TmkDevice(const char *path) :
    pathM(NULL),
    fdM(-1),
    traceFdM(-1),
    streamFdM(-1),
    prevIoctlRequestM(-1)
{
    pathM = (char *) malloc( strlen(path)+1 );
    strcpy (pathM, path);
}

TmkDevice::~TmkDevice()
{
    this->Close();
    free(pathM);
}

bool
TmkDevice::Open(int flags)
{
    if (fdM < 0) {
        if ((fdM = open(pathM, flags)) < 0) {
            return false;
        }
    }

#ifdef DEBUG     // don't even try to trace in a release build
    char *fpath;
    // XXXmartin need a better mechanism for this
    // perhaps SINK_TRACE_PATH (e.g. "/tmp") specified and then we append
    // the device name and extention... eg "/tmp/mpeg0v.pes"
    if (fpath = getenv("SINK_VIDEO_TRACE")) {
        // right now video sink device is /dev/mpeg0v
        if (!strcmp(pathM, "/dev/mpeg0v")) {
            if ((traceFdM = open(fpath, O_CREAT|O_APPEND|O_WRONLY)) == -1) {
                perror("Failed to open video trace file");
            }
        }
    }
    if (fpath = getenv("SINK_VIDEO_STREAM")) {
        // right now video sink device is /dev/mpeg0v
        if (!strcmp(pathM, "/dev/mpeg0v")) {
            if ((streamFdM = open(fpath, O_CREAT|O_APPEND|O_WRONLY)) == -1) {
                perror("Failed to open video stream file");
            }
        }
    }
    if (fpath = getenv("SINK_AUDIO_TRACE")) {
        // right now video sink device is /dev/mpeg0a
        if (!strcmp(pathM, "/dev/mpeg0a")) {
            if ((traceFdM = open(fpath, O_CREAT|O_APPEND|O_WRONLY)) == -1) {
                perror("Failed to open audio trace file");
            }
        }
    }
    if (fpath = getenv("SINK_AUDIO_STREAM")) {
        // right now video sink device is /dev/mpeg0v
        if (!strcmp(pathM, "/dev/mpeg0a")) {
            if ((streamFdM = open(fpath, O_CREAT|O_APPEND|O_WRONLY)) == -1) {
                perror("Failed to open audio stream file");
            }
        }
    }
#endif // DEBUG
    
    // open succeeded, or file was already open
    return true;        
}

void
TmkDevice::Close()
{
    if (fdM >= 0) {
        // The following comment was moved from TmkSinkVela::RevokeDevice
        //
        // cache away the fd so that we can close the device, we do it
        // this way so that ioctl calls will instantly start "silently
        // failing" since they'll be using an FD of -1.  If we just
        // did close(fd); fd = -1;, then it's possible that
        // someone might try to use the fd after we closed it,
        // but before we set it to zero.
        int cachedFd = fdM;
        fdM = -1;
        close(cachedFd);
    }
    if (traceFdM >= 0) {
        int cachedTraceFdM = traceFdM;
        traceFdM = -1;
        close(cachedTraceFdM);
    }
}

// ssize_t
// TmkDevice::Read(void *buf, size_t count)
// {
// }

ssize_t
TmkDevice::Write(const void *buf, size_t count)
{
    ssize_t result;
    long long now = getTimeStamp();
    
    result = write(fdM, buf, count);

    if (traceFdM >= 0) {
        struct TraceRec traceRec;
        bzero(&traceRec, sizeof(traceRec));
        traceRec.op = TraceWrite;
	traceRec.time.ll = now;
        traceRec.request = count;
        traceRec.result = result;
        write(traceFdM, &traceRec, sizeof(traceRec));
    }
    if (streamFdM >= 0) {
        write(streamFdM, buf, count);
    }
    return result;
}

int
TmkDevice::Ioctl(int request)
{
    int result;
    long long now = getTimeStamp();

    result = ioctl(fdM, request);

    if (traceFdM) {
        struct TraceRec traceRec;
        bzero(&traceRec, sizeof(traceRec));
        traceRec.op = TraceIoctlNoArg;
	traceRec.time.ll = now;
        traceRec.request = request;
        traceRec.result = result;
        write(traceFdM, &traceRec, sizeof(traceRec));
        prevIoctlRequestM = request;
    }
    
    return result;
}


int
TmkDevice::Ioctl(int request, int arg)
{
    int result;
    long long now = getTimeStamp();

    result = ioctl(fdM, request, arg);
    
    if (traceFdM >= 0) {
        struct TraceRec traceRec;
        bzero(&traceRec, sizeof(traceRec));
        traceRec.op = TraceIoctlIntArg;
	traceRec.time.ll = now;
        traceRec.request = request;
        traceRec.result = result;
        traceRec.arg.i = arg;
        write(traceFdM, &traceRec, sizeof(traceRec));
        prevIoctlRequestM = request;
    }
    
    return result;
}
    
int
TmkDevice::Ioctl(int request, void *argp)
{
    int result; 
    long long now = getTimeStamp();
 
    result = ioctl(fdM, request, argp);
    
    if (traceFdM >= 0) {
        struct TraceRec traceRec;
        bzero(&traceRec, sizeof(traceRec));
        traceRec.op = TraceIoctlPtrArg;
	traceRec.time.ll = now;
        traceRec.request = request;
        traceRec.result = result;
        traceRec.arg.p = argp;
        write(traceFdM, &traceRec, sizeof(traceRec));
        prevIoctlRequestM = request;
    }
    
    return result;
}
