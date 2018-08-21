/*
Copyright 2007 Andreas Fellnhofer

Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in 
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <cstdlib>


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include<stdio.h>
#include<vector>
#include<math.h>
#include<sndfile.h>
#include<stdint.h>

#include<unistd.h>

#include <itpp/signal/sigfun.h>
#include <itpp/signal/transforms.h>
#include <itpp/signal/window.h>
#include <itpp/base/converters.h>
#include <itpp/base/math/elem_math.h>
#include <itpp/base/matfunc.h>
#include <itpp/base/specmat.h>
#include <itpp/stat/misc_stat.h>




using namespace std;


void usage () {
    printf ( "\
\n    usage: wavecorr -x <input.wav> -y <pattern.wav> -l <len> [-t <threshold>] [-r] [-s] \n\
\n           -r             ... assume raw input 44100Hz 16le 1 channel \
\n           -s             ... print sec.hsec instead of min.sec.hsec \
\n           -t <threshold> ... stop if a correlation value of <threshold> or greater is found \
\n\n");
}



int main(int argc, char **argv)
{
  SNDFILE *xf, *yf;
  SF_INFO xfi;
  SF_INFO yfi;
  char *fnamex=NULL, *fnamey=NULL;
  double *x, *y;
  int resample=1;
  xfi.format = yfi.format = 0;
  int len=0, threshold=0;
  bool print_seconds=false;

  const char *options = "x:y:l:t:sr";
  int c = getopt ( argc, argv, options );

  do
  {
    switch ( c )
    {
      case 'x':
        fnamex = strdup ( optarg );
        break;
      case 'y':
        fnamey = strdup ( optarg );
        break;
      case 'l':
        sscanf ( optarg, "%d", &len );
        break;
      case 's':
        print_seconds = true;
        break;
      case 'r':
        xfi.format = SF_FORMAT_RAW | SF_FORMAT_PCM_16 | SF_ENDIAN_LITTLE;
        xfi.samplerate = 8000;
        xfi.channels = 1;
        fprintf(stderr, "!! RAW expecting: %dHz  %d channel(s) pcm16le\n", xfi.samplerate, xfi.channels);
        break;
      case 't':
        sscanf ( optarg, "%d", &threshold );
        break;
      default:
        usage();
        exit(1);
    }
    c = getopt ( argc, argv, options );
  }
  while ( c != -1 );

  int fd;
  if(fnamex) fd = open(fnamex, O_RDONLY);
  else fd = 0;
  xf = sf_open_fd(fd, SFM_READ, &xfi, fnamex!=NULL);
  yf = sf_open(fnamey, SFM_READ, &yfi);
  if(!xf || !yf) { fprintf(stderr, "files not found %s, %s\r\n", fnamex, fnamey); exit(2); }
  if(len<=0) len = max(xfi.frames, yfi.frames);
  fprintf(stderr, "file %s opened %d\n", fnamex, len);

  x = new double[5*yfi.frames];
  y = new double[yfi.frames];

  memset(x, 0, 5*yfi.frames*sizeof(double));
  memset(y, 0, yfi.frames*sizeof(double));
  int count = sf_readf_double(yf, y, yfi.frames);
  fprintf(stderr, "reference %s (%d): %d samples read\r\n", fnamey, yfi.frames, count);

  itpp::Factory f;
  const itpp::vec vy(y, yfi.frames, f);
  int max=0, maxframe=0, corr;
  bool stop=false;

  count = sf_readf_double(xf, x, 5*yfi.frames);          // read 4 frames and 1 for overlapping
  for(int i=0; (i<len/yfi.frames+1) && count; i+=4) {
    fprintf(stderr, "read %d samples ", count);

    const itpp::vec vx(x, 5*yfi.frames, f);
    itpp::vec vz;

    vz = itpp::xcorr(vy, vx);
    int temp = (int) itpp::max(vz, corr);
    fprintf(stderr, "corr:%d ", temp);
    fprintf(stderr, "@frame:%d\r\n", (i+5)*yfi.frames-corr+yfi.frames);

    for(int j=0; j<yfi.frames; j++)                     // overlap frames to seek in
      x[j] = x[4*yfi.frames+j];
    count = sf_readf_double(xf, x+yfi.frames, 4*yfi.frames);

    if(stop) break;
    if(temp>max) {
      if(threshold && temp>=threshold) { stop=true; }  // break after one additional loop
      max=temp;
      maxframe=(i+5)*yfi.frames-corr+yfi.frames;
    }
  }

  float seconds = (double)maxframe/(double)yfi.samplerate;
  if(!(!threshold || stop)) printf("-");
  if(print_seconds) printf("%02d.%02d", (int)seconds, ((int)(seconds*100))%100);
  else printf("%02d:%02d+%03d", ((int)seconds)/60, ((int)seconds)%60, ((int)(seconds*1000))%1000);

  sf_close(xf);
  sf_close(yf);
  return !(!threshold || stop);   // return 0 if no threshold specified or threshold was found
}

