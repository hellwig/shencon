#ifndef MT5PARSER_H
#define MT5PARSER_H

#include <iostream>
#include <string>

#include "buffer.h"

struct texn {
public:
    char name[8];
};

class node;
class textureData;

class mt5 {
public:
    mt5();
    ~mt5();

    void readData(buffer *nbuf);

public:                     //!!MH!! change to private with friend classes
    node *m;
    textureData *t;
};

class colladaExport {
public:
    colladaExport();
    ~colladaExport();

    void exportModel(const char * nfname, mt5 *model);
    void expNode(std::ofstream &expFile, node *sm, textureData *td, int level);
    std::string getModelName(unsigned long ofs, unsigned long id);
    std::string getMeshName(unsigned long ofs);
    std::string getTextureName(texn *name);
    std::string getTextureFileName(texn *name);

private:
    char *fname;
};

#endif // MT5PARSER_H
