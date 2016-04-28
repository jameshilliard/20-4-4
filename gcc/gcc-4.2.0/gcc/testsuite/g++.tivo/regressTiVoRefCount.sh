#!/bin/bash

arch=$1
if (`test -z "$arch"`) then
   echo "usage: regressTiVoRefCount.sh arch"
   echo "   where arch == ppc"
   echo "   where arch == mips"
   echo "      or arch == x86"
   exit 1
fi

if (`test $arch = ppc`) then
    COMP="$TOOLROOT/powerpc-TiVo-linux-new"
elif (`test $arch = mips`) then
    COMP="$TOOLROOT/mips-TiVo-linux"
elif (`test $arch = x86`) then
    COMP="$TOOLROOT/i686-pc-linux-gnu"
else
   echo "usage: regressTiVoRefCount.sh arch"
   echo "   where arch == ppc"
   echo "   where arch == mips"
   echo "      or arch == x86"
   exit 1
fi

BIN=$COMP/bin
GCC=$BIN/g++
SRCDIR=`pwd`
INCDIR1=$COMP/include
INCDIR2=$ROOT/include

COMPILE="$GCC -B$BIN/ -nostdinc -nostdinc++ -c -O2 -g -I$SRCDIR -I$INCDIR1 -I$INCDIR2 -Werror -Wall -Wtivo -Wunused"
STRIP_PWD="sed s:$PWD/::"

# some errors come only from the pcc compiler.  rather than have two
# different gold files, i'm just grepping them out.
FILTER="egrep -v might.be.clobbered.by..longjmp..or..vfork|In.function...static.initializers.for"

someFailed=0
failures=""

for file in `ls $SRCDIR/*.cxx`; do
    echo 
    echo 
    echo '** compiling ' $file
    echo 
    echo $COMPILE $file '2>&1 | tee $file.out'
    echo "$COMPILE $file 2>&1 | $STRIP_PWD | $FILTER | tee $file.out > /dev/null"

    if test -e $file.graph.gold
    then
	export TIVO_CALL_GRAPH=$file.graph
	export TIVO_CALL_GRAPH_DEMANGLE=yes
    else
	unset TIVO_CALL_GRAPH
    fi

    $COMPILE $file 2>&1 | $STRIP_PWD | $FILTER | tee $file.out > /dev/null

    diff $file.out.gold $file.out
    outputDiff=$?

    if test -e $file.graph.gold
    then
	diff $file.graph $file.graph.gold
	graphDiff=$?
    else
	graphDiff=0
    fi

    if test $outputDiff != 0 -o $graphDiff != 0
    then
	echo
	echo FAILED: $file -- compare $file.out with its goldens
	someFailed=1
	basename=`basename $file`
	failures="$failures      $basename"
    fi
done


echo 
echo 
echo '**********************************'
echo '**'
if test $someFailed != 0
then
    echo '** THERE WERE SOME FAILURES'
    echo "**   " $failures
else
    echo '** succeeded'
fi
echo '**'
echo '**********************************'

