#include <string>
#include <iostream>

#include "taparser.h"
#include "buffer.h"

using namespace std;

taparser::taparser(buffer *buf, const unsigned long ofs)  {
    buf->seek(ofs);
    for (int i=0;i<7;i++) globalParam[i] = buf->rl();

    paraType = bits(PARAM_CTRL, 29, 31);
    endOfStrip = bits(PARAM_CTRL, 28, 28);
    listType = bits(PARAM_CTRL, 24, 26);
    groupEn = bits(PARAM_CTRL, 23, 23);
    stripLen = bits(PARAM_CTRL, 18, 19);
    userClip = bits(PARAM_CTRL, 16, 17);
    shadow = bits(PARAM_CTRL, 7, 7);
    volume = bits(PARAM_CTRL, 6, 6);
    colType = bits(PARAM_CTRL, 4, 5);
    texture = bits(PARAM_CTRL, 3, 3);
    offset = bits(PARAM_CTRL, 2, 2);
    gouraud = bits(PARAM_CTRL, 1, 1);
    uv16Bit = bits(PARAM_CTRL, 0, 0);

    depthCompare = bits(ISP_TSP_INSTR, 29, 31);
    cullingMode = bits(ISP_TSP_INSTR, 27,28);
    zWriteDisable = bits(ISP_TSP_INSTR, 26, 26);
    cacheBypass = bits(ISP_TSP_INSTR, 21, 21);
    dCalcCtrl = bits(ISP_TSP_INSTR, 20, 20);

    srcAlphaInstr = bits(TSP_INSTR, 29, 31);
    dstAlphaInstr = bits(TSP_INSTR, 26, 28);
    srcSelect = bits(TSP_INSTR, 25, 25);
    dstSelect = bits(TSP_INSTR, 24, 24);
    fogControl = bits(TSP_INSTR, 22, 23);
    colorClamp = bits(TSP_INSTR, 21, 21);
    useAlpha = bits(TSP_INSTR, 20, 20);
    ignoreTexAlpha = bits(TSP_INSTR, 19, 19);
    flipUV = bits(TSP_INSTR, 17, 18);
    clampUV = bits(TSP_INSTR, 15, 16);
    filterMode = bits(TSP_INSTR, 13, 14);
    superSampleTex = bits(TSP_INSTR, 12, 12);
    mipMapDAdj = bits(TSP_INSTR, 8, 11);
    texShadInstr = bits(TSP_INSTR, 6, 7);
    texUSize = bits(TSP_INSTR, 3, 5);
    texVSize = bits(TSP_INSTR, 0, 2);

    mipMapped = bits(TEXTURE_CTRL, 31, 31);
    vqCompressed = bits(TEXTURE_CTRL, 30, 30);
    pixelFormat = bits(TEXTURE_CTRL, 27, 29);
    scanOrder = bits(TEXTURE_CTRL, 26, 26);
    strideSelect = bits(TEXTURE_CTRL, 25, 25);
    paletteSelector = bits(TEXTURE_CTRL, 21, 26);
}

taparser::~taparser() {
}

void taparser::dump() {
    string paraTypeStr;
    switch (paraType) {
    case (0):
        paraTypeStr = "End_of_List";
        break;
    case (1):
        paraTypeStr = "User_Tile_Clip";
        break;
    case (2):
        paraTypeStr = "Object_List_Set";
        break;
    case (3):
        paraTypeStr = "Reserved_(Control_Parameter)";
        break;
    case (4):
        paraTypeStr = "Polygon_or_Modifier_Volume";
        break;
    case (5):
        paraTypeStr = "Sprite";
        break;
    case (6):
        paraTypeStr = "Reserved_(Global_Parameter)";
        break;
    case (7):
        paraTypeStr = "Vertex_Parameter";
        break;
    }
    cout << paraTypeStr << " ";

    if (paraType == 1 || paraType == 2 || paraType == 4 || paraType == 5) {
        string listTypeStr;
        switch (listType) {
        case (0):
            listTypeStr = "Opaque";
            break;
        case (1):
            listTypeStr = "Opaque_Modifier_Volume";
            break;
        case (2):
            listTypeStr = "Translucent";
            break;
        case (3):
            listTypeStr = "Translucent_Modifier_Volume";
            break;
        case (4):
            listTypeStr = "Punch_Through";
            break;
        default:
            listTypeStr = "Reserved_(prohibited)";
            break;
        }
        cout << listTypeStr << " ";
    }

    if (paraType == 4 || paraType == 5) {
        string stripLenStr;
        switch (stripLen) {
        case (0):
            stripLenStr = "1strip";
            break;
        case (1):
            stripLenStr = "2strip";
            break;
        case (2):
            stripLenStr = "4strip";
            break;
        case (3):
            stripLenStr = "6strip";
            break;
        }

        string userClipStr;
        switch (userClip) {
        case(0):
            userClipStr = "Disable";
            break;
        case(1):
            userClipStr = "Reserved_(userClip)";
            break;
        case (2):
            userClipStr = "Inside_enable";
            break;
        case (3):
            userClipStr = "Outside_enable";
            break;
        }
        cout << "stripLen:" << stripLenStr << " userClip:" << userClipStr << " ";

        string srcAlphaInstrStr;
        switch (srcAlphaInstr) {
        case(0):
            srcAlphaInstrStr = "Zero";
            break;
        case(1):
            srcAlphaInstrStr = "One";
            break;
        case(2):
            srcAlphaInstrStr = "Other_Color";
            break;
        case(3):
            srcAlphaInstrStr = "Inverse_Other_Color";
            break;
        case(4):
            srcAlphaInstrStr = "Src_Alpha";
            break;
        case(5):
            srcAlphaInstrStr = "Inverse_Src_Alpha";
            break;
        case(6):
            srcAlphaInstrStr = "Dst_Alpha";
            break;
        case(7):
            srcAlphaInstrStr = "Inverse_Dst_Alpha";
            break;
        }
        string dstAlphaInstrStr;
        switch (dstAlphaInstr) {
        case(0):
            dstAlphaInstrStr = "Zero";
            break;
        case(1):
            dstAlphaInstrStr = "One";
            break;
        case(2):
            dstAlphaInstrStr = "Other_Color";
            break;
        case(3):
            dstAlphaInstrStr = "Inverse_Other_Color";
            break;
        case(4):
            dstAlphaInstrStr = "Src_Alpha";
            break;
        case(5):
            dstAlphaInstrStr = "Inverse_Src_Alpha";
            break;
        case(6):
            dstAlphaInstrStr = "Dst_Alpha";
            break;
        case(7):
            dstAlphaInstrStr = "Inverse_Dst_Alpha";
            break;
        }
        string fogControlStr;
        switch (fogControl) {
        case(0):
            fogControlStr="Fog:Look_up_table";
            break;
        case(1):
            fogControlStr="Fog:PerVertex";
            break;
        case(2):
            fogControlStr="No_Fog";
            break;
        case(3):
            fogControlStr="Fog:Look_up_table_Mode_2";
            break;
        }
        string filterModeStr;
        switch (filterMode) {
        case(0):
            filterModeStr = "Point_sampled";
            break;
        case(1):
            filterModeStr = "Bilinear";
            break;
        case(2):
            filterModeStr = "Trilinear_Pass_A";
            break;
        case(3):
            filterModeStr = "Trilinear_Pass_B";
            break;
        }
        string texShadInstrStr;
        switch (texShadInstr) {
        case(0):
            texShadInstrStr ="Decal";
            break;
        case(1):
            texShadInstrStr ="Modulate";
            break;
        case(2):
            texShadInstrStr ="Decal_Alpha";
            break;
        case(3):
            texShadInstrStr ="Modulate_Alpha";
            break;
        }
        cout << "srcAlphaInstrStr:" << srcAlphaInstrStr << " dstAlphaInstrStr:" << dstAlphaInstrStr << " srcSelect:" << srcSelect << " dstSelect:" << dstSelect << " " << fogControlStr
            << " colorClamp:" << colorClamp << " useAlpha(Color):" << useAlpha << " ignoreTexAlpha:" << ignoreTexAlpha << " flipUV:" << flipUV
            << " clampUV:" << clampUV << " filterMode:" << filterModeStr << " superSampleTex:" << superSampleTex << " mipMapDAdj:" << 0.25 * (float)mipMapDAdj
            << " texShadInstr:" << texShadInstrStr << " ";

        string cullingModeStr;
        switch (cullingMode) {
        case (0):
            cullingModeStr = "no_culling";
            break;
        case (1):
            cullingModeStr = "cull_if_small";
            break;
        case (2):
            cullingModeStr = "cull_if_negative";
            break;
        case (3):
            cullingModeStr = "cull_if_positive";
            break;
        }
        if (listType == 1 || listType == 3) {
            string volumeInstrStr;
            switch (depthCompare) {
            case(0):
                volumeInstrStr = "normal_polygon";
                break;
            case(1):
                volumeInstrStr = "inside_last_polygon";
                break;
            case(2):
                volumeInstrStr = "outside_last_polygon";
                break;
            default:
                volumeInstrStr = "reserved";
                break;
            }
            cout << "volumeInstr:" << volumeInstrStr << " cullingMode:" << cullingModeStr << " ";
        } else {
            string depthCompareStr;
            switch (depthCompare) {
            case(0):
                depthCompareStr = "never";
                break;
            case(1):
                depthCompareStr = "less";
                break;
            case(2):
                depthCompareStr = "equal";
                break;
            case(3):
                depthCompareStr = "less_or_equal";
                break;
            case(4):
                depthCompareStr = "greater";
                break;
            case(5):
                depthCompareStr = "not_equal";
                break;
            case(6):
                depthCompareStr = "greater_or_equal";
                break;
            case(7):
                depthCompareStr = "always";
                break;
            }
            cout << "depthCompareMode:" << depthCompareStr << " cullingMode:" << cullingModeStr << " zWriteDisable:" << zWriteDisable << " cacheBypass:" << cacheBypass << " dCalcCtrl:" << dCalcCtrl << " ";

            string pixelFormatStr;
            switch (pixelFormat) {
            case (0):
                pixelFormatStr = "1555";
                break;
            case (1):
                pixelFormatStr = "565";
                break;
            case (2):
                pixelFormatStr = "4444";
                break;
            case (3):
                pixelFormatStr = "YUV422";
                break;
            case (4):
                pixelFormatStr = "BumpMap";
                break;
            case (5):
                pixelFormatStr = "4BPP_Palette";
                break;
            case (6):
                pixelFormatStr = "8BPP_Palette";
                break;
            case (7):
                pixelFormatStr = "Reserved";
                break;
            }
            cout << "mipMapped:" << mipMapped << " vqCompressed:" << vqCompressed << " pixelFormat:" << pixelFormatStr << " ";
            if (pixelFormat == 5 || pixelFormat == 6) {
                cout << "paletteSelector:" << paletteSelector << " ";
            } else {
                cout << "scanOrder:" << (scanOrder == 0 ? "twiddled " : "non twiddled") << " strideSelect:" << strideSelect << " ";
            }
        }
    }
    cout << endl;
}

unsigned long taparser::bits(const unsigned long param, const unsigned long startbit, const unsigned long stopbit) {
    return (globalParam[param]>>startbit) & ((1 << (stopbit - startbit + 1))-1);
}

