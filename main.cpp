#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <strings.h>

#include "pngwriter.h"
#include "pvrspi.h"
#include "buffer.h"
#include "mt5parser.h"
#include "database.h"

using namespace std;

struct ipacDirEntry {
    char name[9];
    char ext[5];
    unsigned long ipacOfs;
    unsigned long ipacSize;
};

void convertMT5(buffer *nb, const char *basename, unsigned long nofs, unsigned long nsize) {
    buffer *nbuf = new buffer(b, nofs, nsize);
    mt5 *mydata = new mt5();
    mydata->readData(nbuf);

    fDB.getID(basename);

    stringstream fname;
    fname.clear();
    fname.str("");
    fname << basename;
    colladaExport *cExp = new colladaExport();
    cExp->exportModel(fname.str().c_str(), mydata);

    delete cExp;
    delete mydata;
    delete nbuf;
}

class ipac {
public:
    ipac(unsigned long ofs) {
        b->seek(ofs);
        unsigned long magic = b->rl();
        if (magic != 'CAPI') {
            cerr << "wrong magic number for IPAC" << endl;
            exit(1);
        }
        unsigned long ipacSize1 = b->rl();
        unsigned long ipacNum = b->rl();
        unsigned long ipacSize2 = b->rl();
        cout << "IPAC size1=" << ipacSize1 << " numFiles=" << ipacNum << " size2=" << ipacSize2 << endl;
        b->seek(ofs+ipacSize1);
        for (unsigned long i=0; i<ipacNum; i++) {
            ipacDirEntry de;
            *((unsigned long *)(de.name))=b->rl();
            de.name[8] = 0;
            *((unsigned long *)(de.name+4))=b->rl();
            de.ext[4] = 0;
            *((unsigned long *)(de.ext))=b->rl();
            de.ipacOfs = b->rl();
            de.ipacSize = b->rl();
            cout << "file " << i << " name=" << de.name << "." << de.ext << " ofs=" << hex << de.ipacOfs << " size=" << de.ipacSize << dec << endl;
            stringstream fname;
            fname << de.name << "." << de.ext;
            b->dump(fname.str().c_str(), de.ipacOfs+ofs, de.ipacSize);
            unsigned long savepos = b->tell();
            if (!strcmp(de.ext, "CHRM") || !strcmp(de.ext, "MAPM")) {
                cout << "converting map file" << endl;
                convertMT5(b, fname.str().c_str(), de.ipacOfs+ofs, de.ipacSize);
            }
            b->seek(savepos);
            ipacDir.push_back(de);
        }
    }
private:
    vector<ipacDirEntry> ipacDir;
};

class pakf {
public:
    pakf() : nTex(0) {
        b->seek(0);
        unsigned long magic = b->rl();
        if (magic != 'FKAP') {
            cerr << "wrong magic number for PAKF" << endl;
            exit(1);
        }
        unsigned long pakfSize = b->rl();
        unsigned long pakfC1 = b->rl();
        unsigned long pakfNumTex = b->rl();
        cout << "PAKF file size=" << pakfSize << " c1=" << hex << pakfC1 << " numTex=" << pakfNumTex << dec << endl;
        unsigned long ofs = 16;
        do {
            b->seek(ofs);
            magic = b->rl();
            unsigned long sectionSize = b->rl();
            unsigned long texn1, texn2;
            unsigned long gbix;
            unsigned long texn3, texn4;
            int width, height;
            int image_size;
            int bits;
            unsigned char *outptr;
            int ret;
            stringstream fname;
            switch (magic) {
            case (0):
                continue;
            case ('YMUD'):
                cout << "Dummy" << endl;
                break;
            case ('NXET'):
                texn1 = b->rl();
                texn2 = b->rl();
                gbix = b->rl();
                texn3 = b->rl();
                texn4 = b->rl();
                if (gbix != 'XIBG') {
                    cerr << "no GBIX signature" << endl;
                    exit(1);
                }
                cout << "Texture " << hex << nTex << " size=" << sectionSize << " texn1=" << texn1 << " texn2=" << texn2 << dec << endl;
                // Dump texture to PVR file
                fname.clear();
                fname.str("");
                fname << "tex_" << hex << texn1 << "_" << texn2 << ".pvr" << dec;
                b->dump(fname.str().c_str(), b->tell(), sectionSize-28);
                // Convert Dreamcast Texture to PNG file for blender export
                b->seek(ofs+0x10);
                ret = pvr2bin(b->ptr(),outptr,width, height, image_size, bits);
                cout << "res:" << ret << " width:" << width << " height:" << height << endl;
                fname.clear();
                fname.str("");
                texn t;
                memcpy(t.name, &texn1, 4);
                memcpy(t.name+4, &texn2, 4);
                fname << "t" << hex << texDB.getID(&t) << ".png";
                writePng(fname.str().c_str(), outptr, width, height, bits);
                free(outptr);
                nTex++;
                break;
            default:
                cerr << "unknown PAKF section type " << (char)magic << (char)(magic>>8) << (char)(magic>>16) << (char)(magic>>24) << ", please check file format" << endl;
                exit(1);
            }
            ofs += sectionSize;
        } while (magic != 0);
        if (nTex != pakfNumTex) {
            cerr << "number of PAKF textures in file not compatible with header" << endl;
            exit(1);
        }
        if (ofs+16 != pakfSize) {
            cerr << "size of PAKF payload " << hex << ofs+16 << " in file not compatible with header " << pakfSize << endl;
            exit(1);
        }
        ipacData = new ipac(pakfSize);
    }
    ~pakf() {
        if (ipacData) delete ipacData;
    }
private:
    unsigned long nTex;
    ipac *ipacData;
};

class paks {
public:
    paks() : ipacData(NULL){
        b->seek(0);
        unsigned long magic = b->rl();
        unsigned long paksSize = b->rl();
        unsigned long paksC1 = b->rl();
        unsigned long paksC2 = b->rl();
        if (magic != 'SKAP') {
            cerr << "wrong magic number for PAKS" << endl;
            exit(1);
        }
        if (paksSize != 16) {
            cerr << "unusual section size of PAKS, please check file format" << endl;
        }
        cout << "PAKS file c1=" << hex << paksC1 << ", c2=" << paksC2 << dec << endl;
        ipacData = new ipac(paksSize);
    }
    ~paks() {
        if (ipacData) delete ipacData;
    }
private:
    ipac *ipacData;
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf (stderr, "This program unpacks Shenmue *.PKF, *.PFS files and *.MT5 files.\nUsage %s <fname>\n", argv[0]);
        exit(-1);
    }
    paks *myPaks;
    pakf *myPakf;

    b = new buffer(argv[1]);
    b->seek(0);
    unsigned long magic = b->rl();
    switch (magic) {
    case ('SKAP'):
        myPaks = new paks();
        break;
    case ('FKAP'):
        myPakf = new pakf();
        break;
    case ('MCRH'):
        convertMT5(b, argv[1], 0, b->blen());
        break;
    }
    return 0;
}
