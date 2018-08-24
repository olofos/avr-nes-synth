TRACK_COUNT=$(./nsf_play -c "$1")
BANK_COUNT=$(./nsf_play -b "$1")

echo Analysing file: \"$1\"
echo Number of tracks: $TRACK_COUNT
./nsf_play -f "$1"

for track in `seq 1 $TRACK_COUNT`; do
    NUMBER_OF_USED_BANKS=$(./nsf_play -t$track "$1" -s120 | grep '^0xb' | sed -e 's/^0xb.,.*0x\(..\),/\1/g;t;d'|sort|uniq|wc -l)
    echo Track $track uses $NUMBER_OF_USED_BANKS of $BANK_COUNT banks
done
