#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <strings.h>

#include "database.h"

using namespace std;

textureDB::textureDB() {
    char line[256];
    FILE *f = fopen("tex.db", "rb");
    if (f) {
        tval = 0;
        entry e;
        while (!feof(f)) {
            if (fgets(line, 255, f) == 0) continue;
            int c = sscanf(line, "%x %x %d", (unsigned long *)(e.name.name), (unsigned long *)(e.name.name + 4), &(e.id));
            if (c==3) {
                tlist.push_back(e);
                if (e.id >= tval) tval = e.id + 1;
            }
        }
        fclose(f);
    }

}

textureDB::~textureDB() {
    FILE *f = fopen("tex.db", "wb");
    if (f) {
        for (vector <entry>::iterator tit=tlist.begin(); tit != tlist.end(); tit++) {
            fprintf(f, "%x %x %d\n", *((unsigned long *)(tit->name.name)),*((unsigned long *)(tit->name.name + 4)),tit->id);
        }
        fclose(f);
    } else {
        cerr << "error writing updated texture database" << endl;
    }
}

unsigned long textureDB::getID(texn *t) {
    for (vector <entry>::iterator tit=tlist.begin(); tit != tlist.end(); tit++) {
        if (!memcmp(tit->name.name, t->name, 8)) {
            return tit->id;
        }
    }
    entry e;
    memcpy(e.name.name, t->name, 8);
    e.id = tval;
    tlist.push_back(e);
    return tval++;
}

fileDB::fileDB() {
    char line[256];
    char name[256];
    FILE *f = fopen("file.db", "rb");
    if (f) {
        fval = 0;
        entry e;
        while (!feof(f)) {
            if (fgets(line, 255, f) == 0) continue;
            int c = sscanf(line, "%s %d", &(name), &(e.id));
            e.name = name;
            if (c==2) {
                flist.push_back(e);
                if (e.id >= fval) fval = e.id + 1;
            }
        }
        fclose(f);
    }

}

fileDB::~fileDB() {
    FILE *f = fopen("file.db", "wb");
    if (f) {
        for (vector <entry>::iterator fit=flist.begin(); fit != flist.end(); fit++) {
            fprintf(f, "%s %d\n", fit->name.c_str(), fit->id);
        }
        fclose(f);
    } else {
        cerr << "error writing file database" << endl;
    }
}

unsigned long fileDB::getID(string name) {
    for (vector <entry>::iterator fit=flist.begin(); fit != flist.end(); fit++) {
        if (name == fit->name) {
            return fit->id;
        }
    }
    entry e;
    e.name = name;
    e.id = fval;
    flist.push_back(e);
    return fval++;
}

nodeDB::nodeDB() {
}

nodeDB::~nodeDB() {
}

unsigned long nodeDB::getID(unsigned long nid, unsigned long sid) {
    for (vector <entry>::iterator nit=nlist.begin(); nit != nlist.end(); nit++) {
        if (nid == nit->nid) {
            return nit->id;
        }
    }
    entry e;
    e.nid = nid;
    e.sid = sid;
    e.id = nval;
    nlist.push_back(e);
    return nval++;
}

void nodeDB::clear() {
    nlist.clear();
    nval = 0;
}

meshDB::meshDB() {
}

meshDB::~meshDB() {
}

unsigned long meshDB::getID(unsigned long mid) {
    for (vector <entry>::iterator mit=mlist.begin(); mit != mlist.end(); mit++) {
        if (mid == mit->mid) {
            return mit->id;
        }
    }
    entry e;
    e.mid = mid;
    e.id = mval;
    mlist.push_back(e);
    return mval++;
}

void meshDB::clear() {
    mlist.clear();
    mval = 0;
}

fileDB fDB;
textureDB texDB;
nodeDB nDB;
meshDB mDB;
