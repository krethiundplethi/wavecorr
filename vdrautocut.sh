#!/bin/bash

#/usr/bin/ffmpeg -i /mnt/vdr/data/vdrdir/Radiokolleg/_/2007-11-26.09.03.02.99.rec/001.vdr -acodec pcm_s16le -ar 8000 -ac 1 -f s16le - 2>/dev/null | debug/src/wavecorr -y match3.wav -r -l 10000000 -t 200 -s

PATH="/mnt/vdr/data/vdrdir/Radiokolleg/_"
CORR="/home/ducky/wavecorr/debug/src/wavecorr"
MATCH="match3.wav"
NAMING="Dimensionen"

rm blacklist.txt

for inname in $PATH/*
do 
  outname=`echo ${inname##*/} | /usr/bin/awk -F '.' '{ printf "$NAMING_"; printf $1; print ".mp3" }'`
  title=
  splitsec=`/usr/bin/cat $inname/0??.vdr | /usr/bin/ffmpeg -i - -acodec pcm_s16le -ar 8000 -ac 1 -f s16le - 2>/dev/null | debug/src/wavecorr -y $MATCH -r -l 10000000 -t 15 -s`
 
  sec=`echo $splitsec | /usr/bin/awk -F '.' '{ print $1 }'`
  if [ $sec -lt 0 ]; then
    echo $inname >> blacklist.txt 
  else
      /usr/bin/cat $inname/0??.vdr \
    | /usr/bin/ffmpeg -i $inname -ss $splitsec -acodec pcm_s24le -f wav - \
    | lame -m m -q2 --abr 64 -b 32 -B 160 \
      --tt $title --ty ${outname%%-} \
    - $outname
  fi
  echo $splitsec
done

#rm list.txt
#echo "" >> list.txt

#for FILE in $PATH/*
#do
#  echo "Finding cutting position for $FILE -> ${FILE##*/}"
#  echo -n "/usr/bin/mp3splt $FILE " >> list.txt
#  /usr/bin/ffmpeg -i $FILE -acodec pcm_s16le -ar 44100 -f s16le - | $CORR -y match.wav -r -l 7000000 2>log.txt >> list.txt
#  echo "999.00 -o ${FILE##*/}" >> list.txt
#done

