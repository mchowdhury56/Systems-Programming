###############################################################################
# Author: Marjan Chowdhury
# Description: Provides the basic functionality of a recycle bin
###############################################################################
#!/bin/bash

readonly JUNK=~/.junk

display_flag=0
list_flag=0
purge_flag=0
file_flag=0

name=$(basename "$0")

display_usage() {
cat << Here
Usage: $name [-hlp] [list of files]
    -h: Display help.
    -l: List junked files.
    -p: Purge all files.
    [list of files] with no other arguments to junk those files.   
Here
}


while getopts ":hlp" option; do
    case "$option" in
        h) display_flag=1
            ;;
        l) list_flag=1
            ;;
        p) purge_flag=1
            ;;
        ?) printf "Error: Unknown option '-%s'.\n" $OPTARG >&2
            display_usage
            exit 1
            ;;
    esac
done


if [ $# -eq 0 ]; then
    display_usage
    exit 0
fi

if [ ! -d ~/.junk ]; then
    mkdir "$JUNK"
fi


declare -a file_array

shift "$((OPTIND-1))"
for f in $@; do
    file_array[$index]=$f
    file_flag=1
    ((++index))
done


if [ $(($display_flag + $list_flag + $purge_flag + $file_flag)) -gt 1 ]; then
    printf "Error: Too many options enabled.\n" $OPTARG >&2
            display_usage
            exit 1
fi


if [ $display_flag -eq 1 ]; then
    display_usage
    exit 0

elif [ $list_flag -eq 1 ]; then
    ls -lAF $JUNK
    exit 0

elif [ $purge_flag -eq 1 ]; then
    rm -r $JUNK
    exit 0

elif [ $file_flag -eq 1 ]; then
    for f in $file_array; do
        mv $f $JUNK
    done
    mv ${file_array[*]} "$JUNK"
    exit 0
fi

exit 0