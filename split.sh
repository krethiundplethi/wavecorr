#!/bin/bash

PATH=/media/horch/Radiokolleg
CORR=/home/andi/wavecorr/debug/src/wavecorr

#rm list.txt
echo "" >> list.txt

for FILE in $PATH/*
do
  echo "Finding cutting position for $FILE -> ${FILE##*/}"
  echo -n "/usr/bin/mp3splt $FILE " >> list.txt
  /usr/bin/ffmpeg -i $FILE -acodec pcm_s16le -ar 44100 -f s16le - | $CORR -y match.wav -r -l 7000000 2>log.txt >> list.txt
  echo "999.00 -o ${FILE##*/}" >> list.txt
done

