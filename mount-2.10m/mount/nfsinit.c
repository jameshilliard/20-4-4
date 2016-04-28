/* 
 * nfsinit.
 * NOTE: This file is duplicated in mount-2.71 and mount-2.10m/mount
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>

extern int mount_main(int,char**);
static int invoke_mount(char **, char **, int, char **);

#undef NFSINIT_DEBUG

int 
main(int argc, char **argv, char **envp)
{
  int rval;
  int nfsinit_argc = 0;
  char *nfsinit_argv[10];
  int i;
  char nfshandle[128];
  char *nfsinitstr = 0, *srcrootstr = 0, *nfsopts = 0;
  char *hpkimplementationstr = 0;
  int fIsXnfsinit = 0;

  /* Assmble mount arglist. */
  /* We think we are the mount command. */
#ifdef NFSINIT_DEBUG
  printf("nfsinit startup\n");
  fflush(stdout);
#endif
  nfsinit_argv[nfsinit_argc++] = "mount";

  nfsinit_argv[nfsinit_argc++] = "-n";

  nfsinit_argv[nfsinit_argc++] = "-r";

  for(i=0;;i++) {
    if (!envp[i]) break;
    if (!strncmp(envp[i], "nfsinit=", 8)) {
      nfsinitstr = envp[i]+8;
      fIsXnfsinit = 0;
    } else if (!strncmp(envp[i], "xnfsinit=", 9)) {
      nfsinitstr = envp[i]+9;
      fIsXnfsinit = 1;
    } else if (!strncmp(envp[i], "srcroot=", 8)) {
      srcrootstr = envp[i]+8;
    } else if (!strncmp(envp[i], "HpkImpl=", 8)) {
      hpkimplementationstr = envp[i]+8;
    }
  }

  /* Parse NFS options, if any, from kernel param to mount arg */
  if (srcrootstr && index(srcrootstr, ':')) {
    nfsopts = index(srcrootstr, ',');
  } else if (nfsinitstr) {
    nfsopts = index(nfsinitstr, ',');
  }
  if (nfsopts) {
    *nfsopts = 0;
    nfsopts++;
    nfsinit_argv[nfsinit_argc++] = "-o";
    nfsinit_argv[nfsinit_argc++] = nfsopts;
  }

  /* NFS handle. */
  nfsinit_argv[nfsinit_argc] = 0;
  /* Skip past handle and mount point */
  nfsinit_argc += 2;
  /* Terminate the list. */
  nfsinit_argv[nfsinit_argc] = 0;

  if (!fIsXnfsinit) {
    if (srcrootstr) {
      char *cp = index(srcrootstr, ':');

      if (cp)
        nfsinit_argv[nfsinit_argc-2] = srcrootstr;
      else {
        strcpy(nfshandle, nfsinitstr);
        cp = index(nfshandle, ':');
        if (!cp) {
          printf("No ip address in nfsinit\n");
          return 1;
        }
        cp++;
        strcpy(cp, srcrootstr);
        nfsinit_argv[nfsinit_argc-2] = nfshandle;
      }
    } else if (nfsinitstr) {
      strcpy(nfshandle,nfsinitstr);
      strcat(nfshandle, "/../../srcroot");
      nfsinit_argv[nfsinit_argc-2] = nfshandle;
    } else {
      fprintf(stderr, 
              "\n***\n"
              "*** ERROR: neither SRCROOT nor NFSINIT found,\n"
              "*** sleeping...\n"
              "***\n\n");

      for (;;) {
          pause();
      }
    }

    /* Local mount point is always /srcroot */
    nfsinit_argv[nfsinit_argc-1] = "/srcroot";

    if ((rval = invoke_mount(argv, envp, nfsinit_argc, nfsinit_argv))) {
      return rval;
    }
  }

  if (hpkimplementationstr) {
    strcpy(nfshandle,nfsinitstr);
    strcat(nfshandle,"/platform/");
    strcat(nfshandle,hpkimplementationstr);

    nfsinit_argv[nfsinit_argc-2] = nfshandle;
    nfsinit_argv[nfsinit_argc-1] = "/platform";

    if(invoke_mount(argv, envp, nfsinit_argc, nfsinit_argv) != 0) {
      /* mount of platform subdir is allowed to fail */
      fprintf(stderr, 
              "\n***\n"
                "*** WARNING: mount of /platform/%s failed,\n"
                "*** Using default HPK implementation.\n"
                "***\n\n", hpkimplementationstr);
    }
  } else {
    fprintf(stderr,
            "\n***\n"
              "*** WARNING: HpkImpl not set, using default HPK implementation\n"
              "***\n\n");
  }

  i =  execve("/sbin/init", argv, envp);
  printf("execve returns %d\n", i);
  perror("nfsinit execve error: ");
  return(1);
}

static int
invoke_mount(char **argv, char **envp, int nfsinit_argc, char **nfsinit_argv) {
  int rval;
#ifdef NFSINIT_DEBUG
  int i;
  printf("NFS Args: \n");
  for(i=0;;i++) {
    if (!nfsinit_argv[i]) break;
    printf("a[%d]=%s\n", i, nfsinit_argv[i]);
  }
  printf("Args: \n");
  for(i=0;;i++) {
    if (!argv[i]) break;
    printf("a[%d]=%s\n", i, argv[i]);
  }
  printf("Env: \n");
  for(i=0;;i++) {
    if (!envp[i]) break;
    printf("a[%d]=%s\n", i, envp[i]);
  }
  fflush(stdout);
#endif
  if ((rval = mount_main(nfsinit_argc,nfsinit_argv))) {
    perror("nfsinit mount error: ");
    return rval;
  }
#ifdef NFSINIT_DEBUG
  printf("nfsinit, exec init\n");
  fflush(stdout);
#endif
  return 0;
}
