#!/bin/bash

GetInstruction ()
{
    [ -z "$1" ] && exit
    functionName="$1 "

    headerFile=`grep --include=\*intrin.h -Rl "$functionName" /usr/lib/gcc | head -n1`
    [ -z "$headerFile" ] && exit
    >&2 echo "find in: $headerFile"

    targetDirective=`grep "#pragma GCC target(\|$functionName" $headerFile | grep -B 1 "$functionName" | head -n1`
    echo $targetDirective | grep -o '"[^,]*[,"]' | sed 's/"//g' | sed 's/,//g'
}

instruction=`GetInstruction $1`
if [ -z "$instruction" ]; then
    echo "Error: function not found: $1"
else
    echo "GCC SIMD Instruction Lib: -m$instruction"
fi
