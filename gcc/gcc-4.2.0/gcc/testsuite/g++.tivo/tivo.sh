#!/usr/bin/env bash

arch=$1
compiler=$2
test=$3
srcdir=$4


if [[ -z $arch ]]; then
    echo "usage: $0 <arch>"
    echo "   where arch == ppc"
    echo "   where arch == mips"
    echo "      or arch == x86"
    exit 1
fi

case $arch in
  ppc*|powerpc*) TARGET=powerpc-TiVo-linux-gnu;;
  mips*) TARGET=mips-TiVo-linux-gnu;;
  native|x86|i[3456789]86*) TARGET=;;
  *) echo "$0: Error: Unknown arch \"$arch\"."; exit 1;;
esac

WARN="-Wall -Werror -Wunused -Wtivo"
CXX=${compiler:-"$TOOLROOT/bin/g++"}
CXXFLAGS="-O2 -I. $WARN -fmessage-length=0"
export CC CFLAGS CXX CXXFLAGS

if [[ ! -x $CXX ]]; then echo "$0: Error: $CXX does not exist."; exit 1; fi

SRCDIR=${srcdir:-"."}
test=${test:-*}

pass=0
fail=0
unsupported=0
xfail=0
xpass=0
untested=0

STRIP_PATH="sed -e s;$SRCDIR/;;"
STRIP_WALL='grep -v -e "cc1plus: warnings being treated as errors"'

for file in $SRCDIR/${test}.cxx; do
    unset TIVO_CALL_GRAPH
    unset TIVO_CALL_GRAPH_DEMANGLE

    filename=$(basename $file)
    if [[ -e $file.graph.gold ]]; then
        export TIVO_CALL_GRAPH=$filename.graph
        export TIVO_CALL_GRAPH_DEMANGLE=yes
    fi

    echo "Running $CXX $CXXFLAGS -c $filename"
    $CXX $CXXFLAGS -c $file 2>&1 | eval $STRIP_WALL | $STRIP_PATH > $filename.out 2>&1

    diff $file.out.gold $filename.out
    if [[ $? != 0 ]]; then
        echo "FAIL: $filename -- compare $filename.out with its goldens"
	fail=$(($fail + 1))
    else
        echo "PASS: $filename -- compare $filename.out with its goldens"
	pass=$(($pass + 1))
    fi

    if [[ -e $file.graph.gold ]]; then
        diff $file.graph.gold $filename.graph
        if [[ $? != 0 ]]; then
            echo "FAIL: $filename -- compare $filename.graph with its goldens"
	    fail=$(($fail + 1))
        else
            echo "PASS: $filename -- compare $filename.graph with its goldens"
	    pass=$(($pass + 1))
        fi
    fi
done

echo
echo "                === tivoRefCountSuite Summary ==="
echo
[[ $pass -gt 0 ]] && echo "# of expected passes            $pass"
[[ $fail -gt 0 ]] && echo "# of unexpected failures        $fail"
[[ $xfail -gt 0 ]] && echo "# of expected failures          $xfail"
[[ $xpass -gt 0 ]] && echo "# of expected passes            $xfail"
[[ $unsupported -gt 0 ]] && echo "# of unsupported tests          $unsupported"
[[ $untested -gt 0 ]] && echo "# of untested tests             $untested"
echo $CXX version $($CXX --version | sed -n -e '1s/^[^0-9]*//p')

#if [[ $fail != 0 ]]; then
#    exit 1
#else
#    exit 0
#fi
exit 0
