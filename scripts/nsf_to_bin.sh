#/bin/bash

if [ $# -lt 3 ]; then
    echo -n 'Usage: '
    echo -n $0
    echo ' nsf_file bin_name number_of_tracks'
else
    NSF_FILE=$1
    BIN_BASE=bin/$2
    TRACKS=$3

    for i in $(seq 1 $TRACKS); do
        printf -v j "%02d" $i
        echo Playing track $i of '"'$(basename "$NSF_FILE")'"'
        ./nsf_play "$NSF_FILE" -t $i -s 300 -o out.dat
        echo Checking for loops
        ./detect_loops out.dat out-loop.dat 
        echo Writing "$BIN_BASE$j.BIN"
        ./dat_to_bin out-loop.dat "$BIN_BASE$j.BIN" > /dev/null
        echo
    done
    echo Done
fi
