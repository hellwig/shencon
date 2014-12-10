#ifndef TAPARSER_H
#define TAPARSER_H

#include "buffer.h"

class taparser {
public:
    taparser(buffer *buf, const unsigned long ofs);
    ~taparser();

    void dump();

enum {ISP_TSP_INSTR = 0, PARAM_CTRL = 1, TEXTURE_CTRL = 2, TSP_INSTR = 3};
private:
    unsigned long globalParam[8];

    unsigned long paraType;
    unsigned long endOfStrip;
    unsigned long listType;

    unsigned long groupEn;
    unsigned long stripLen;
    unsigned long userClip;

    unsigned long shadow;
    unsigned long volume;
    unsigned long colType;
    unsigned long texture;
    unsigned long offset;
    unsigned long gouraud;
    unsigned long uv16Bit;

    unsigned long depthCompare;         // or volumeInstruction
    unsigned long cullingMode;
    unsigned long zWriteDisable;
    unsigned long cacheBypass;
    unsigned long dCalcCtrl;

    unsigned long srcAlphaInstr;
    unsigned long dstAlphaInstr;
    unsigned long srcSelect;
    unsigned long dstSelect;
    unsigned long fogControl;
    unsigned long colorClamp;
    unsigned long useAlpha;
    unsigned long ignoreTexAlpha;
    unsigned long flipUV;
    unsigned long clampUV;
    unsigned long filterMode;
    unsigned long superSampleTex;
    unsigned long mipMapDAdj;
    unsigned long texShadInstr;
    unsigned long texUSize;
    unsigned long texVSize;

    unsigned long mipMapped;
    unsigned long vqCompressed;
    unsigned long pixelFormat;
    unsigned long scanOrder;
    unsigned long strideSelect;
    unsigned long paletteSelector;

    unsigned long bits(const unsigned long param, const unsigned long startbit, const unsigned long stopbit);
};

#endif // TAPARSER_H
