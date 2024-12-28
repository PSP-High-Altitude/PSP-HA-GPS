// Convert a libpcap/tcpdump file to files of 8-bit samples
//
// GNSS Firehose
// Copyright (c) 2012 Peter Monta <pmonta@gmail.com>

#include <stdio.h>
#include <stdlib.h>

unsigned char buf[1478];
long long t0,timestamp,delta;

void get_sample(unsigned char buf[],int s,unsigned char* x)
{ int byte_offset = s/4;
  int samp_offset = 3-(s%4);
  *x = (buf[byte_offset]>>(2*samp_offset))&3; }

void convert(unsigned char x[], int chan)
{ int i;
  unsigned char buf[1920];
  unsigned char xi,xq;
  for (i=0; i<960; i++) {
    get_sample(x,6*i+2*(chan-1),&xi);
    get_sample(x,6*i+2*(chan-1)+1,&xq);
    xi = 2*xi - 3;
    xq = 2*xq - 3;
    buf[2*i] = xi;
    buf[2*i+1] = xq; }
  fwrite(buf,1920,1,stdout); }

void convert_zeros(int n)
{ unsigned char buf[1920];
  int i;
  for (i=0; i<960; i++) {
    buf[2*i] = 0;
    buf[2*i+1] = 0; }
  fwrite(buf,1920,1,stdout); }

int read_next_valid_packet()
{ int plen;
  int n;
  for (;;) {
    n = fread(buf,16,1,stdin);
    if (n!=1)
      return 0;
    plen = buf[8] + (buf[9]<<8) + (buf[10]<<16) + (buf[11]<<22);
    fread(buf+16,plen,1,stdin);
    if (plen==1462)
      return 1; } }

int main(int argc, char* argv[])
{ int i,n;
  int chan;
  chan = 1;
  if (argc==2)
    chan = atoi(argv[1]);
  fread(buf,4,1,stdin);
  fread(buf,4,1,stdin);
  fread(buf,4,1,stdin);
  fread(buf,4,1,stdin);
  fread(buf,4,1,stdin);
  fread(buf,4,1,stdin);
  t0 = -1;
  i = 0;
  for (;;) {
    n = read_next_valid_packet();
    if (n!=1)
      break;
    timestamp = (((long long)buf[30])<<56) |
                (((long long)buf[31])<<48) |
                (((long long)buf[32])<<40) |
                (((long long)buf[33])<<32) |
                (((long long)buf[34])<<24) |
                (((long long)buf[35])<<16) |
                (((long long)buf[36])<<8) |
                (((long long)buf[37])<<0);
    if (t0>0) {
      delta = timestamp - t0;
      if (delta!=960)
        fprintf(stderr,"packet %d: timestamp delta %d (pkt %d rem %d) (ts1 %08llx ts2 %08llx)\n",
                  i,(int)delta,((int)delta)/480,((int)delta)%480,t0,timestamp);
      if (delta<0) {
        fprintf(stderr,"delta<0, bailing out");
        break; }
      if ((delta%960)!=0) {
        fprintf(stderr,"delta not a multiple of 960 samples, bailing out");
        break; }
      while (delta>960) {
        convert_zeros(960);
        delta -= 960; } }
    convert(&buf[38],chan);
    i = i + 1;
    t0 = timestamp; }
  return 0; }
