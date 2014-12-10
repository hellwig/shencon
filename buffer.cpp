#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <strings.h>
#include <zlib.h>

#include "buffer.h"

using namespace std;

buffer::buffer(const char *fname) : buf(NULL), cursor(0), buflen(0) {
    FILE *f = fopen(fname, "rb");
    if (!f) {
        cerr << "error opening file " << fname << endl;
        return;
    }
    unsigned short magic;
    fread(&magic, sizeof(short), 1, f);
    if (magic == 0x8b1f) {
        fseek(f, -4L, SEEK_END);
        fread(&buflen, sizeof(unsigned long), 1, f);
        cout << "gzip compressed file with size " << buflen << endl;
    } else {
        fseek(f, 0L, SEEK_END);
        buflen = ftell(f);
        cout << "uncompressed file with size" << buflen << endl;
    }
    fclose(f);
    buf = new unsigned char[buflen];
    gzFile gf = gzopen(fname, "rb");
    if (gzread(gf, buf, buflen) != buflen) {
        gzclose(gf);
        delete [] buf;
        buf = NULL;
        buflen = 0;
        cerr << "error reading file " << fname << endl;
        return;
    }
    gzclose(gf);
    cout << "successful read " << buflen << " bytes from file " << fname << endl;

}

buffer::buffer(const buffer *orig, const unsigned long ofs, unsigned long bsize) : cursor(0), buflen(bsize) {
    buf = new unsigned char[bsize];
    memcpy(buf, orig->buf + ofs, bsize);
}

buffer::~buffer() {
    if (buf) delete [] buf;
}

const unsigned long buffer::blen() const
{
    return buflen;
}

bool buffer::seek(unsigned long pos) {
    if (pos<buflen) {
        cursor = pos;
        return true;
    } else {
        return false;
    }
}

unsigned long buffer::tell() {return cursor;}

bool buffer::eof() const {return cursor >= buflen;}

unsigned char buffer::rb() {return buf[cursor++];}

unsigned short buffer::rw() {unsigned short val=*((unsigned short *)(buf + cursor)); cursor+=2; return val;}

unsigned long buffer::rl() {unsigned long val=*((unsigned long *)(buf + cursor)); cursor+=4; return val;}

float buffer::rf() {float val=*((float *)(buf + cursor)); cursor+=4; return val;}

const unsigned char *buffer::ptr() {return buf + cursor;}

void buffer::dump(const char *fname, unsigned long ofs, unsigned long writeSize) {
    FILE *f = fopen(fname, "wb");
    if (!f) {
        cerr << "error opening file " << fname << endl;
        return;
    }
    if (fwrite(buf+ofs, sizeof(char), writeSize, f) != writeSize) {
        fclose(f);
        cerr << "error writing file " << fname << endl;
        return;
    }
    fclose(f);
    cout << "successful wrote " << writeSize << " bytes to file " << fname << endl;
}

buffer *b;
