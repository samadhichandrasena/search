#!/bin/sh
#
# Output the WIDTH and HEIGHT -D arguments
# given a tiles board size 8, 15, 24, etc.
#
sz=$(echo "scale=0; sqrt($1+1)" | bc)
if [[ $((sz*sz)) -eq $(($1+1)) ]]
then
    echo -DWIDTH=$sz -DHEIGHT=$sz
else
    echo -DWIDTH=$sz -DHEIGHT=$((sz+1))
fi
