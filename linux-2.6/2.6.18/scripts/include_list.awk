BEGIN {
  outfile1 = ARGV[1]
  outfile2 = ARGV[2]
  printf("#!/bin/sh\n") > outfile1
  printf("set -e\n") > outfile1
  printf("if [ \"$KBUILD_WRITABLE\" = \"y\" ]\n") > outfile1
  printf("then\n") > outfile1
  printf("  rm -rf .tmp_include\n") > outfile1
  printf("fi\n") > outfile1
  obj = ENVIRON["KBUILD_OBJTREE"]
  if (obj == "") {
    printf("echo KBUILD_OBJTREE is not defined\n") > outfile1
    exit(1)
  }
  gsub(/\/*$/, "", obj)
  obj = obj "/"
  printf("override KBUILD_OBJTREE := %s\n", obj) > outfile2
  paths = ENVIRON["KBUILD_INCLUDE_PATHS"]
  if (paths == "") {
    printf("echo KBUILD_INCLUDE_PATHS is not defined\n") > outfile1
    exit(1)
  }
  split(paths, path)
  for (p in path) {
    gsub(/\/*$/, "", path[p])
    path[p] = path[p] "/"
  }
  dirs = ENVIRON["KBUILD_INCLUDE_DIRS"]
  if (dirs == "") {
    printf("echo KBUILD_INCLUDE_DIRS is not defined\n") > outfile1
    exit(1)
  }
  split(dirs, dir)
  for (d in dir) {
    gsub(/\/*$/, "", dir[d])
    # dir cannot have trailing '/', ln -s complains
  }
  arch = ENVIRON["ARCH"]
  if (arch == "") {
    printf("echo ARCH is not defined\n") > outfile1
    exit(1)
  }
  for (p in path) {
    # objtree comes first to read generated include files.
    printf("echo -I%s\n", path[p]) > outfile1
    if (common_src_obj) {
      printf("rm -f %s%sasm\n", src, path[p]) > outfile1
      printf("ln -s asm-%s %s%sasm\n", arch, src, path[p]) > outfile1
    }	
    else {
      for (i = 998; i >= 0; --i) {
	srcname = sprintf("KBUILD_SRCTREE_%03d", i)
	src = ENVIRON[srcname]
	if (src != "") {
	  common_src_obj = src == obj
	  gsub(/\/*$/, "", src)
	  src = src "/"
	  printf("override %s := %s\n", srcname, src) > outfile2
	  printf("result=\n") > outfile1
	  printf("if [ -d %s%sasm-%s ]\n", src, path[p], arch) > outfile1
	  printf("then\n") > outfile1
	  printf("  if [ \"$KBUILD_WRITABLE\" = \"y\" ]\n") > outfile1
	  printf("  then\n") > outfile1
	  printf("    mkdir -p .tmp_include/src_%03d/%s\n", i, path[p]) > outfile1
	  printf("    ln -s %s%sasm-%s .tmp_include/src_%03d/%sasm\n", src, path[p], arch, i, path[p]) > outfile1
	  printf("  fi\n") > outfile1
	  printf("  result='-I.tmp_include/src_%03d/%s'\n", i, path[p]) > outfile1
	  printf("fi\n") > outfile1
	  for (d in dir) {
	    printf("if [ -d %s%s%s ]\n", src, path[p], dir[d]) > outfile1
	    printf("then\n") > outfile1
	    printf("  if [ \"$KBUILD_WRITABLE\" = \"y\" ]\n") > outfile1
	    printf("  then\n") > outfile1
	    printf("    mkdir -p .tmp_include/src_%03d/%s\n", i, path[p]) > outfile1
	    printf("    ln -s %s%s%s .tmp_include/src_%03d/%s%s\n", src, path[p], dir[d], i, path[p], dir[d]) > outfile1
	    printf("  fi\n") > outfile1
	    printf("  result='-I.tmp_include/src_%03d/%s'\n", i, path[p]) > outfile1
	    printf("fi\n") > outfile1
	  }
	  printf("echo $result | sed -e 's:/*$::'\n") > outfile1
	}
      }
    }
  }
  stop
}
