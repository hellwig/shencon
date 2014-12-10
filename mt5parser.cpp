#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <vector>
#include <strings.h>

#include "buffer.h"
#include "pngwriter.h"
#include "pvrspi.h"
#include "mt5parser.h"
#include "taparser.h"
#include "database.h"

using namespace std;

buffer *buf;

struct vector3d { float x, y, z; };
struct vector2d { float u, v;};
struct vertUV {
    unsigned long vertex;
    unsigned long uv;
    unsigned long color;
};

class vertexNormalUVData {
public:
    vertexNormalUVData(unsigned long ofs, unsigned long ncount) : count(ncount) {
        vector3d p;
        buf->seek(ofs);
        cout << "  VertexNormal " << count << endl;
        for (unsigned long i=0; i<count; i++) {
            p.x = buf->rf();
            p.y = buf->rf();
            p.z = buf->rf();
            vertexVector.push_back(p);
//            cout << "  " << i << " (" << p.x << "," << p.y << "," << p.z << ")";
            p.x = buf->rf();
            p.y = buf->rf();
            p.z = buf->rf();
            normalVector.push_back(p);
 //           cout << " (" << p.x << "," << p.y << "," << p.z << ")" << endl;
        }
    }
    ~vertexNormalUVData() {
        vertexVector.clear();
        normalVector.clear();
        uvVector.clear();
        colorVector.clear();
    }

public:         //!!MH!! better private with friends
    unsigned long count;
    vector<vector3d> vertexVector;
    vector<vector3d> normalVector;
    vector<vector2d> uvVector;
    vector<vector2d> colorVector;
};

class meshStrip {
public:
    meshStrip(unsigned long ofs,unsigned short nformat, vertexNormalUVData *pVertexNormalUV) : format(nformat){
        buf->seek(ofs);
        nVertUV = (unsigned long) (0xffff - buf->rw() + 1);
 //       cout << "      MeshStrip " << nVertUV << " ";
        vertUV v;
        vector2d uvCoord;
        vector2d color;
        for (unsigned long i=0; i<nVertUV; i++) {
            switch (format) {
            case 0x10:
                v.vertex = (unsigned long)buf->rw();
                v.uv = 0xffffffff;
                v.color = 0xffffffff;
//                cout << i << "(" << v.vertex << ") ";
                break;
            case 0x11:
                v.vertex = (unsigned long)buf->rw();
                v.uv = pVertexNormalUV->uvVector.size();
                v.color = 0xffffffff;
                uvCoord.u = (float)((signed short)(buf->rw()))/(float)(0x3ff);
                uvCoord.v = (float)((signed short)(buf->rw()))/(float)(0x3ff);
                pVertexNormalUV->uvVector.push_back(uvCoord);
//                cout << i << "(" << v.vertex << "," << v.u << "," << v.v << ") ";
                break;
           case 0x1c:
                v.vertex = (unsigned long)buf->rw();
                v.uv = pVertexNormalUV->uvVector.size();
                uvCoord.u = (float)((signed short)(buf->rw()))/(float)(0x3ff);
                uvCoord.v = (float)((signed short)(buf->rw()))/(float)(0x3ff);
                pVertexNormalUV->uvVector.push_back(uvCoord);
                v.color = pVertexNormalUV->colorVector.size();
                color.u = buf->rw();
                color.v = buf->rw();
                pVertexNormalUV->colorVector.push_back(color);
                //cout << i << "(" << hex << color.u << "," << color.v << ") " << dec;
                break;
            default:
                cerr << "unknown mesh strip format" << hex << format << endl;
                exit(1);
            }
            vertUVVector.push_back(v);
        }
//        cout << endl;
    }
    ~meshStrip() {
        vertUVVector.clear();
    }
    unsigned long getSize() {
        switch (format) {
        case 0x10:                              // only vertex information
            return nVertUV * 2+2;
            break;
        case 0x11:                              // vertex and uv information
            return nVertUV * 6+2;
            break;
        case 0x1c:                              // vertex and uv and additional (color?) information
            return nVertUV * 10+2;
            break;
        default:
            cerr << "unknown mesh strip format" << hex << format << endl;
            exit(1);
        }
    } // +2 (number of vertices in strip)

public:                     //!!MH!! better make it public with friend classes
    unsigned long nVertUV;
    unsigned short format;
    vector<vertUV> vertUVVector;
};

class meshObject {
public:
    meshObject(unsigned long nofs, vertexNormalUVData *pVertexNormalUV) : ofs(nofs){
        taparser ta(buf, ofs);
        ta.dump();
        buf->seek(ofs);
        cout << "  MeshObject ofs=" << ofs << " ";
        for (int i=0; i<3; i++) {
            header[i] = buf->rw();
            cout << hex << header[i] << " ";
        }
        switch (getType()) {
        case 0x2e:
        case 0x0a:                  // Type 0x2e is a regular textured Mesh, Type 0x0a also contains interpolated color
            headerSize = 16;
            break;
        case 0x26:                  // no texture coordinates (flat?)
            headerSize = 12;
            break;
        default:
            cerr << "unknown mesh object format" << hex << header[2] << endl;
            exit(1);
        }
        for (unsigned long i=3; i<headerSize; i++) {
            header[i] = buf->rw();
            cout << hex << header[i] << " ";
        }
        cout << endl;
        nStrips = buf->rw();
        cout << "    Number of strips " << nStrips << endl;
        unsigned long pos = ofs + 2 + headerSize*2;   // header size + number of strips
        for (unsigned long i=0; i<nStrips; i++) {
            meshStrip *strip = new meshStrip(pos, getPrimitiveType(), pVertexNormalUV);          // format depends on header 14
            meshStripVector.push_back(strip);
            pos += strip->getSize();
        }
    }
    ~meshObject() {
        for (vector<meshStrip *>::iterator it = meshStripVector.begin(); it != meshStripVector.end(); it++) {
            delete *it;
        }
        meshStripVector.clear();
    }
    unsigned long getID() {return ofs;}
    unsigned long getSize() {return (unsigned long)header[headerSize-1]+headerSize*2;}// add size of strip header
    unsigned short getTrans() {return header[0];}
    unsigned short getType() {return header[2];}
    unsigned short getTextureNumber() {return header[11];}
    unsigned short getPrimitiveType() {return header[headerSize-2];}
public:         //!!MH!! better make it private with friend classes
    unsigned long ofs;
    unsigned short header[16];

    unsigned long headerSize;
    unsigned long nStrips;
    vector<meshStrip *> meshStripVector;
};

class meshData {
public:
    meshData(unsigned long ofs, unsigned long nObjStart) : objStart(nObjStart) {
        buf->seek(ofs);
        for (int i=0; i<4; i++) header[i] = buf->rl();
        cout << "  MeshData " << hex << header[0] << " " << header[1] << " " << header[2] <<  " " << header[3] << endl;
    }
    ~meshData() {
    }
private:
    unsigned long header[4];
    unsigned long objStart;         // First object that is described by this data
};

class modelData {
public:
    modelData(unsigned long ofs) : pVertexNormalUV(NULL) {
        buf->seek(ofs);
        polyType = buf->rl();
        oVertexNormal = buf->rl();
        vertexCount = buf->rl();
        oMesh = buf->rl();
        f1 = buf->rl();
        f2 = buf->rl();
        f3 = buf->rl();
        f4 = buf->rl();
        cout << "polyType=" << polyType << " oVertexNormal=" << hex << oVertexNormal << " vertexCount=" << vertexCount << " oMesh=" << hex << oMesh << endl;
        cout << "f1=" << hex << f1 << " f2=" << f2 << " f3=" << f3 << " f4=" << f4 << dec << endl;

        pVertexNormalUV = new vertexNormalUVData(oVertexNormal, vertexCount);
        meshData *pMeshData;
        meshObject *pMeshObject;
        unsigned long pos = oMesh;
        unsigned long magic;
        do {
            buf->seek(pos);
            magic = buf->rl();
            switch (magic) {
            case 0x00100002:
            case 0x00100003:
            case 0x00100004:
                pMeshObject = new meshObject(pos, pVertexNormalUV);
                meshObjectVector.push_back(pMeshObject);
                pos += pMeshObject->getSize();
                break;
            case 0x0008000e:
                pMeshData = new meshData(pos, meshObjectVector.size());
                meshDataVector.push_back(pMeshData);
                pos += 16;
                break;
            case 0xffff8000:
                // end Model data
                break;
            default:
                cerr << "unknown magic " << hex << magic << endl;
                exit(1);
                break;
            }
        } while (magic != 0xffff8000);
    }
    ~modelData() {
        if (pVertexNormalUV) delete pVertexNormalUV;
        for (vector<meshData *>::iterator it = meshDataVector.begin(); it != meshDataVector.end(); it++) {
            delete *it;
        }
        meshDataVector.clear();
        for (vector<meshObject *>::iterator it = meshObjectVector.begin(); it != meshObjectVector.end(); it++) {
            delete *it;
        }
        meshObjectVector.clear();
    }

public:         // !!MH!! better private with friends?
    unsigned long polyType;
    unsigned long oVertexNormal;
    unsigned long vertexCount;
    unsigned long oMesh;
    unsigned long f1, f2, f3, f4;

    vertexNormalUVData *pVertexNormalUV;
    vector <meshData *> meshDataVector;
    vector<meshObject *> meshObjectVector;
};

class node;
map <unsigned long, node *> modelMap;

class node {
public:
    node(unsigned long ofs) :  myOfs(ofs), pDown(NULL), pNext(NULL), pUp(NULL) {
        modelMap[ofs] = this;
        buf->seek(ofs);
        nodeID = buf->rl();
        oMeshHeader = buf->rl();
        rotX = 360. * ((float)buf->rl())/((float)(0xffff));
        rotY = 360. * ((float)buf->rl())/((float)(0xffff));
        rotZ = 360. * ((float)buf->rl())/((float)(0xffff));
        scaleX = buf->rf();
        scaleY = buf->rf();
        scaleZ = buf->rf();
        x = buf->rf();
        y = buf->rf();
        z = buf->rf();
        oDown = buf->rl();
        oNext = buf->rl();
        oUp = buf->rl();
        unsigned long unknown0 = buf->rl();
        unsigned long unknown1 = buf->rl();
        if (unknown0 != 0 || unknown1 != 0) {
            cerr << "node definition ofs=" << ofs << " has unknown bytes not equal to zero." << endl;
            exit(1);
        }
        map <unsigned long, node *>::iterator found;
        cout << "nodeID=" << hex << nodeID << " oMeshHeader=" << hex << oMeshHeader
            << " rotX=" << rotX << " rotY=" << rotY << " rotZ=" << rotZ
            << " scaleX=" << scaleX << " scaleY=" << scaleY << " scaleZ=" << scaleZ
            << " x=" << x << " y=" << y << " z=" << z
            << " oDown=" << hex << oDown << " oNext=" << hex << oNext << " oUp=" << hex << oUp << endl;
        if (oMeshHeader) pData = new modelData(oMeshHeader); else pData = NULL;
        if (oUp) {
            if ((found = modelMap.find(oUp)) == modelMap.end()) {
                pUp = new node(oUp);
            } else pUp = found->second;
        }
        if (oNext) {
            if ((found = modelMap.find(oNext)) == modelMap.end()) {
                pNext = new node(oNext);
            } else pNext = found->second;
        }
        if (oDown) {
            if ((found = modelMap.find(oDown)) == modelMap.end()) {
                pDown = new node(oDown);
            } else pDown = found->second;
        }
    }
public:             //!!MH!! better private with friends ?
    unsigned long nodeID;
    unsigned long oMeshHeader;
    float rotX, rotY, rotZ;
    float scaleX, scaleY, scaleZ;
    float x,y,z;
    unsigned long oDown, oNext, oUp;

    unsigned long myOfs;

    node *pDown;
    node *pNext;
    node *pUp;

    modelData *pData;
};

map<unsigned long, texn *> nameMap;

struct texl {
public:
    unsigned long ofs;
    unsigned long u1, u2;
    texn *name;
};

class textureData {
public:
    textureData(unsigned long ofs) {
        unsigned long i;
        unsigned long start;
        texn *name;
        texl load;
        do {
            buf->seek(ofs);
            if (buf->eof()) break;
            unsigned long magic = buf->rl();
            unsigned long length = buf->rl();
            switch (magic) {
            case ('DXET'):
                nTextures = buf->rl();
                break;
            case ('NXET'):
                readTexture(ofs);
                break;
            case ('EMAN'):
                for (i=0; i<((length-8)/8);i++) {
                    name = new texn;
                    *((unsigned long *)name->name)=buf->rl();
                    *((unsigned long *)((name->name)+4))=buf->rl();
                    textureNameVector.push_back(name);
                    cout << hex << *(unsigned long *)name->name << *(unsigned long *)((name->name)+4) << endl;
                    nameMap[ofs] = name;
                }
                break;
            case ('LXET'):
                start = buf->rl();
                nLoads = buf->rl();
                for (i=0; i<((length-16)/12);i++) {
                    load.ofs=buf->rl();
                    load.u1=buf->rl();
                    load.u2=buf->rl();
                    map<unsigned long, texn *>::iterator found;
                    if ((found = nameMap.find(load.ofs))==nameMap.end()) load.name = NULL; else load.name = found->second;
                    textureLoadVector.push_back(load);
                }
                if (start != ofs+16) {
                    cerr << "TEXL start is strange (ofs=" << hex << ofs << ",start=" << start << ")" << endl;
                    exit(1);
                }
                if (nLoads != (length-16)/12) {
                    cerr << "TEXL number of loads is strange (nLoads=" << hex << nLoads << ",(length-16)/12=" << (length-16)/12 << ")" << endl;
                    exit(1);
                }
                break;
            case ('LRTP'):
                nPtr = buf->rl();
                for (i=0; i<((length-12)/4); i++) {
                    texturePtrVector.push_back(buf->rl());
                }
                if (nPtr != (length-12)/4) {
                    cerr << "PTRL number of pointers is strange (nPtr=" << hex << nPtr << ",(length-12)/4=" << (length-12)/4 << ")" << endl;
                    exit(1);
                }
                break;
            default:
                cerr << "unknown texture magic word " << hex << magic << endl;
                exit(1);
            }
            ofs += length;
        } while (true);
    }

    ~textureData() {
        for (vector<texn *>::iterator it = textureNameVector.begin(); it != textureNameVector.end(); it++) {
            delete *it;
        }
        textureNameVector.clear();
        textureLoadVector.clear();
        texturePtrVector.clear();
   }

   void readTexture(unsigned long ofs) {
        buf->seek(ofs);
        unsigned long magic1 = buf->rl();
        if (magic1 != 'NXET') {
            cerr << "Wrong Texture Magic Word" << endl;
            exit(1);
        }
        unsigned long texSize = buf->rl();
        texn *name = new texn;
        *((unsigned long *)name->name)=buf->rl();
        *((unsigned long *)((name->name)+4))=buf->rl();
        textureNameVector.push_back(name);
        nameMap[ofs] = name;
        unsigned long magic2 = buf->rl();
        if (magic2 != 'XIBG') {
            cerr << "Wrong GBIX Magic Word" << endl;
            exit(1);
        }
        unsigned long unknown = buf->rl();
        unsigned long texnum = buf->rl();
        unsigned long magic3 = buf->rl();
        if (magic3 != 'TRVP') {
            cerr << "Wrong PVRT Magic Word" << endl;
            exit(1);
        }
        cout << "Texture " << texnum << ": size=" << texSize << ", name=" << hex << *((unsigned long *)(name->name)) << "_" << *((unsigned long *)(name->name+4)) << ", unknown=" << unknown << endl;

        int width, height;
        int image_size;
        int bits;
        unsigned char *outptr;
        int ret;
        stringstream fname;

        fname.clear();
        fname.str("");
        fname << "tex_" << hex << *((unsigned long *)(name->name)) << "_" << *((unsigned long *)(name->name+4)) << ".pvr" << dec;
        b->dump(fname.str().c_str(), ofs+28, texSize-28);
        // Convert Dreamcast Texture to PNG file for blender export
        b->seek(ofs+28);
        ret = pvr2bin(b->ptr(),outptr,width, height, image_size, bits);
        cout << "res:" << ret << " width:" << width << " height:" << height << endl;
        fname.clear();
        fname.str("");
        texn t;
        memcpy(t.name, name->name, 8);
        fname << "t" << hex << texDB.getID(&t) << ".png";
        writePng(fname.str().c_str(), outptr, width, height, bits);
        free(outptr);
   }

public:                 //!!MH!! change to private with friends classes
    unsigned long nTextures;
    unsigned long nLoads;
    unsigned long nPtr;
    vector <texn *> textureNameVector;
    vector <texl> textureLoadVector;
    vector <unsigned long> texturePtrVector;
};

mt5::mt5() : m(NULL){}

mt5::~mt5() {
    if (m) delete m;
    modelMap.clear();
    nameMap.clear();
}

void mt5::readData(buffer *nbuf) {
    buf = nbuf;
    buf->seek(0);
    modelMap.clear();
    nameMap.clear();
    unsigned long magic = buf->rl();
    unsigned long oTex = buf->rl();
    unsigned long oModel = buf->rl();

    // Read Header
    if (magic != 'MCRH') {
        cerr << "unknown format (only HRCM supported)" << endl;
        return;
    }
    cout << "Texture Pointer:" << std::hex << oTex << " Model Pointer:" << std::hex << oModel << endl;

    m = new node(oModel);
    t = new textureData(oTex);
}

colladaExport::colladaExport() :fname(NULL){}
colladaExport::~colladaExport() {
    if (fname) delete [] fname;
}

void colladaExport::exportModel(const char * nfname, mt5 *model) {
    string mname;
    string texName;
    string matName;
    ofstream expFile;

    mDB.clear();
    nDB.clear();

    fname = new char[strlen(nfname)+1];
    strcpy(fname, nfname);

    expFile.open((string(fname)+".dae").c_str());

    // Export Header
    expFile << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << endl;
    expFile << "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">" << endl;
    expFile << "<asset>" << endl;
    expFile << "\t<contributor>" << endl;
    expFile << "\t\t<authoring_tool>MT5Converter (Marc.Hellwig@gmail.com)</authoring_tool>" << endl;
    expFile << "\t</contributor>" << endl;
    expFile << "\t<unit name=\"meter\" meter=\"1\"/>" << endl;
    expFile << "\t<up_axis>Y_UP</up_axis>" << endl;
    expFile << "</asset>" << endl;

    // Export the texture names
    expFile << "<library_images>" << endl;
    for (vector <texn *>::iterator tit=model->t->textureNameVector.begin(); tit != model->t->textureNameVector.end(); tit++) {
        texName = getTextureName(*tit);
        expFile << "\t<image id=\"" << texName << "_png\" name=\"" << texName << "_png\">" << endl;
        expFile << "\t\t<init_from>" << getTextureFileName(*tit) << "</init_from>" << endl;
        expFile << "\t</image>" << endl;
    }
    expFile << "</library_images>" << endl;

    // Export the material
    expFile << "<library_effects>" << endl;
    // go through all the models and figure out, which textures are in use
    for (map <unsigned long, node *>::iterator it=modelMap.begin(); it != modelMap.end(); it++) {
        if (it->second->pData) {
            // go through ALL mesh objects
            mname = getModelName(it->first, it->second->nodeID);
            for (vector<meshObject *>::iterator mObIt = it->second->pData->meshObjectVector.begin(); mObIt != it->second->pData->meshObjectVector.end(); mObIt++) {
                unsigned short mtype = (*mObIt)->getType();
                if (mtype == 0x2e || mtype == 0x0a) texName = getTextureName(model->t->textureNameVector[(*mObIt)->getTextureNumber()]);
                matName = mname + getMeshName((*mObIt)->getID());
                expFile << "\t<effect id=\"" << matName << "-effect\">" << endl;
                expFile << "\t\t<profile_COMMON>" << endl;
                if (mtype == 0x2e || mtype == 0x0a) {
                    expFile << "\t\t\t<newparam sid=\"" << texName << "_png-surface\">" << endl;
                    expFile << "\t\t\t\t<surface type = \"2D\">" << endl;
                    expFile << "\t\t\t\t\t<init_from>" << texName << "_png</init_from>" << endl;
                    expFile << "\t\t\t\t</surface>" << endl;
                    expFile << "\t\t\t</newparam>" << endl;
                    expFile << "\t\t\t<newparam sid=\"" << texName << "_png-sampler\">" << endl;
                    expFile << "\t\t\t\t<sampler2D>" << endl;
                    expFile << "\t\t\t\t\t<source>" << texName << "_png-surface</source>" << endl;
                    expFile << "\t\t\t\t</sampler2D>" << endl;
                    expFile << "\t\t\t</newparam>" << endl;
                }
                expFile << "\t\t\t<technique sid=\"common\">" << endl;
                expFile << "\t\t\t\t<phong>" << endl;
                expFile << "\t\t\t\t\t<emission>" << endl;
                expFile << "\t\t\t\t\t\t<color sid=\"emission\">0 0 0 1</color>" << endl;
                expFile << "\t\t\t\t\t</emission>" << endl;
                expFile << "\t\t\t\t\t<ambient>" << endl;
                expFile << "\t\t\t\t\t\t<color sid=\"ambient\">0.5 0.5 0.5 1</color>" << endl;
                expFile << "\t\t\t\t\t</ambient>" << endl;
                if (mtype == 0x2e || mtype == 0x0a) {
                    expFile << "\t\t\t\t\t<diffuse>" << endl;
                    expFile << "\t\t\t\t\t\t<texture texture=\"" << texName << "_png-sampler\" texcoord=\"UVMap\"/>" << endl;
                    expFile << "\t\t\t\t\t</diffuse>" << endl;
                }
                expFile << "\t\t\t\t\t<specular>" << endl;
                expFile << "\t\t\t\t\t\t<color sid=\"specular\">0.1 0.1 0.1 1</color>" << endl;
                expFile << "\t\t\t\t\t</specular>" << endl;
                expFile << "\t\t\t\t\t<shininess>" << endl;
                expFile << "\t\t\t\t\t\t<float sid=\"shininess\">50</float>" << endl;
                expFile << "\t\t\t\t\t</shininess>" << endl;
                if ((*mObIt)->getTrans() == 3) {
                    if (mtype == 0x2e || mtype == 0x0a) {
                        expFile << "\t\t\t\t\t<transparent>" << endl;
                        expFile << "\t\t\t\t\t\t<texture texture=\"" << texName << "_png-sampler\" texcoord=\"UVMap\"/>" << endl;
                        expFile << "\t\t\t\t\t</transparent>" << endl;
                    }
                    expFile << "\t\t\t\t\t<transparency>" << endl;
                    expFile << "\t\t\t\t\t\t<float sid=\"transparency\">0</float>" << endl;
                    expFile << "\t\t\t\t\t</transparency>" << endl;
                }
                expFile << "\t\t\t\t\t<index_of_refraction>" << endl;
                expFile << "\t\t\t\t\t\t<float sid=\"index_of_refraction\">1</float>" << endl;
                expFile << "\t\t\t\t\t</index_of_refraction>" << endl;
                expFile << "\t\t\t\t</phong>" << endl;
                expFile << "\t\t\t</technique>" << endl;
                expFile << "\t\t</profile_COMMON>" << endl;
                expFile << "\t</effect>" << endl;
            }
        }
    }
    expFile << "</library_effects>" << endl;
    expFile << "<library_materials>" << endl;
    // go through all the models and figure out, which textures are in use
    for (map <unsigned long, node *>::iterator it=modelMap.begin(); it != modelMap.end(); it++) {
        if (it->second->pData) {
            mname = getModelName(it->first, it->second->nodeID);
            for (vector<meshObject *>::iterator mObIt = it->second->pData->meshObjectVector.begin(); mObIt != it->second->pData->meshObjectVector.end(); mObIt++) {
                matName = mname + getMeshName((*mObIt)->getID());
                expFile << "\t<material id=\"" << matName << "-material\">" << endl;
                expFile << "\t\t<instance_effect url=\"#" << matName << "-effect\"/>" << endl;
                expFile << "\t</material>" << endl;
            }
        }
    }
    expFile << "</library_materials>" << endl;

    // Export the geometry of the models
    expFile << "<library_geometries>" << endl;
    for (map <unsigned long, node *>::iterator it=modelMap.begin(); it != modelMap.end(); it++) {
        mname = getModelName(it->first, it->second->nodeID);
        if (it->second->pData) {
            expFile << "\t<geometry id=\"" << mname << "-mesh\" name=\"" << mname << "\">" << endl;
            expFile << "\t\t<mesh>" << endl;

            // Vertex Data
            expFile << "\t\t\t<source id=\"" << mname << "-mesh-positions\">" << endl;
            expFile << "\t\t\t\t<float_array id=\"" << mname << "-mesh-positions-array\" count=\"" << it->second->pData->vertexCount*3 << "\">";
            for (vector<vector3d>::iterator vit=it->second->pData->pVertexNormalUV->vertexVector.begin(); vit != it->second->pData->pVertexNormalUV->vertexVector.end(); vit++) {
                expFile << vit->x << " " << vit->y << " " << vit->z << " ";
            }
            expFile << "</float_array>" << endl;
            expFile << "\t\t\t\t<technique_common>" << endl;
            expFile << "\t\t\t\t\t<accessor source=\"#" << mname << "-mesh-positions-array\" count=\"" << it->second->pData->vertexCount << "\" stride=\"3\">" << endl;
            expFile << "\t\t\t\t\t\t<param name=\"X\" type=\"float\"/>" << endl;
            expFile << "\t\t\t\t\t\t<param name=\"Y\" type=\"float\"/>" << endl;
            expFile << "\t\t\t\t\t\t<param name=\"Z\" type=\"float\"/>" << endl;
            expFile << "\t\t\t\t\t</accessor>" << endl;
            expFile << "\t\t\t\t</technique_common>" << endl;
            expFile << "\t\t\t</source>" << endl;

            // Normal Data
            expFile << "\t\t\t<source id=\"" << mname << "-mesh-normals\">" << endl;
            expFile << "\t\t\t\t<float_array id=\"" << mname << "-mesh-normals-array\" count=\"" << it->second->pData->vertexCount*3 << "\">";
            for (vector<vector3d>::iterator vit=it->second->pData->pVertexNormalUV->normalVector.begin(); vit != it->second->pData->pVertexNormalUV->normalVector.end(); vit++) {
                expFile << vit->x << " " << vit->y << " " << vit->z << " ";
            }
            expFile << "</float_array>" << endl;
            expFile << "\t\t\t\t<technique_common>" << endl;
            expFile << "\t\t\t\t\t<accessor source=\"#" << mname << "-mesh-normals-array\" count=\"" << it->second->pData->vertexCount << "\" stride=\"3\">" << endl;
            expFile << "\t\t\t\t\t\t<param name=\"X\" type=\"float\"/>" << endl;
            expFile << "\t\t\t\t\t\t<param name=\"Y\" type=\"float\"/>" << endl;
            expFile << "\t\t\t\t\t\t<param name=\"Z\" type=\"float\"/>" << endl;
            expFile << "\t\t\t\t\t</accessor>" << endl;
            expFile << "\t\t\t\t</technique_common>" << endl;
            expFile << "\t\t\t</source>" << endl;

            // UV Data
            if (it->second->pData->pVertexNormalUV->uvVector.size()) {
                expFile << "\t\t\t<source id=\"" << mname << "-mesh-map\">" << endl;
                expFile << "\t\t\t\t<float_array id=\"" << mname << "-mesh-map-array\" count=\"" << it->second->pData->pVertexNormalUV->uvVector.size() * 2 << "\">";
                for (vector<vector2d>::iterator vit=it->second->pData->pVertexNormalUV->uvVector.begin(); vit != it->second->pData->pVertexNormalUV->uvVector.end(); vit++) {
                    expFile << vit->u << " " << vit->v << " ";
                }
                expFile << "</float_array>" << endl;
                expFile << "\t\t\t\t<technique_common>" << endl;
                expFile << "\t\t\t\t\t<accessor source=\"#" << mname << "-mesh-map-array\" count=\"" << it->second->pData->pVertexNormalUV->uvVector.size() << "\" stride=\"2\">" << endl;
                expFile << "\t\t\t\t\t\t<param name=\"S\" type=\"float\"/>" << endl;
                expFile << "\t\t\t\t\t\t<param name=\"T\" type=\"float\"/>" << endl;
                expFile << "\t\t\t\t\t</accessor>" << endl;
                expFile << "\t\t\t\t</technique_common>" << endl;
                expFile << "\t\t\t</source>" << endl;
            }

            // Color Data
/*                if (it->second->pData->pVertexNormalUV->colorVector.size()) {
                expFile << "\t\t\t<source id=\"" << mname << "-mesh-color\">" << endl;
                expFile << "\t\t\t\t<float_array id=\"" << mname << "-mesh-color-array\" count=\"" << it->second->pData->pVertexNormalUV->colorVector.size() * 2 << "\">";
                for (vector<vector2d>::iterator vit=it->second->pData->pVertexNormalUV->colorVector.begin(); vit != it->second->pData->pVertexNormalUV->colorVector.end(); vit++) {
                    expFile << vit->u << " " << vit->v << " ";
                }
                expFile << "</float_array>" << endl;
                expFile << "\t\t\t\t<technique_common>" << endl;
                expFile << "\t\t\t\t\t<accessor source=\"#" << mname << "-mesh-color-array\" count=\"" << it->second->pData->pVertexNormalUV->uvVector.size() << "\" stride=\"2\">" << endl;
                expFile << "\t\t\t\t\t\t<param name=\"BASE\" type=\"float\"/>" << endl;
                expFile << "\t\t\t\t\t\t<param name=\"OFFSET\" type=\"float\"/>" << endl;
                expFile << "\t\t\t\t\t</accessor>" << endl;
                expFile << "\t\t\t\t</technique_common>" << endl;
                expFile << "\t\t\t</source>" << endl;
            }*/

            // Vertices
            expFile << "\t\t\t<vertices id=\"" << mname << "-mesh-vertices\">" << endl;
            expFile << "\t\t\t\t<input semantic=\"POSITION\" source=\"#" << mname << "-mesh-positions\"/>" << endl;
            expFile << "\t\t\t</vertices>" << endl;

#ifdef USE_TRIANGLE_STRIPS
            // Now the triangles, quads and trianglestrips
            for (vector<meshObject *>::iterator mObIt = it->second->pData->meshObjectVector.begin(); mObIt != it->second->pData->meshObjectVector.end(); mObIt++) {
                expFile << "\t\t\t<tristrips count=\"" << (*mObIt)->nStrips << "\">" << endl;
                expFile << "\t\t\t\t<input semantic=\"VERTEX\" source=\"#" << mname << "-mesh-vertices\" offset=\"0\"/>" << endl;
                expFile << "\t\t\t\t<input semantic=\"NORMAL\" source=\"#" << mname << "-mesh-normals\" offset=\"1\"/>" << endl;
                expFile << "\t\t\t\t<input semantic=\"TEXCOORD\" source=\"#" << mname << "-mesh-map\" offset=\"2\"/>" << endl;
                for (vector<meshStrip *>::iterator mStIt = (*mObIt)->meshStripVector.begin(); mStIt != (*mObIt)->meshStripVector.end(); mStIt++) {
                    expFile << "\t\t\t\t<p>";
                    for (vector<vertUV>::iterator tVIt = (*mStIt)->vertUVVector.begin(); tVIt != (*mStIt)->vertUVVector.end(); tVIt++) {
                        expFile << tVIt->vertex << " " << tVIt->vertex << " " << tVIt->uv << " ";
                    }
                    expFile << "</p>" << endl;
                }
                expFile << "\t\t\t</tristrips>" << endl;
            }
#else
            for (vector<meshObject *>::iterator mObIt = it->second->pData->meshObjectVector.begin(); mObIt != it->second->pData->meshObjectVector.end(); mObIt++) {
                matName = " material=\"" + getModelName(it->first, it->second->nodeID) + getMeshName((*mObIt)->getID()) + "-material\"";
                for (vector<meshStrip *>::iterator mStIt = (*mObIt)->meshStripVector.begin(); mStIt != (*mObIt)->meshStripVector.end(); mStIt++) {
                    expFile << "\t\t\t<triangles";
                    if ((*mStIt)->format == 0x11 || (*mStIt)->format == 0x1c) expFile << matName;
                    expFile << " count=\"" << (*mStIt)->vertUVVector.size()-2 << "\">" << endl;
                    expFile << "\t\t\t\t<input semantic=\"VERTEX\" source=\"#" << mname << "-mesh-vertices\" offset=\"0\"/>" << endl;
                    expFile << "\t\t\t\t<input semantic=\"NORMAL\" source=\"#" << mname << "-mesh-normals\" offset=\"1\"/>" << endl;
                    if ((*mStIt)->format == 0x11 || (*mStIt)->format == 0x1c) expFile << "\t\t\t\t<input semantic=\"TEXCOORD\" source=\"#" << mname << "-mesh-map\" offset=\"2\"/>" << endl;
//                        if ((*mStIt)->format == 0x1c) expFile << "\t\t\t\t<input semantic=\"COLOR\" source=\"#" << mname << "-mesh-map\" offset=\"3\"/>" << endl;
                    expFile << "\t\t\t\t<p>" << endl;
                    for (unsigned long triangleLoop = 0; triangleLoop < (*mStIt)->vertUVVector.size()-2; triangleLoop++) {
                        expFile << string(5,'\t')
                                << ((triangleLoop%2) ? ((*mStIt)->vertUVVector[triangleLoop+1].vertex) : ((*mStIt)->vertUVVector[triangleLoop].vertex)) << " "
                                << ((triangleLoop%2) ? ((*mStIt)->vertUVVector[triangleLoop+1].vertex) : ((*mStIt)->vertUVVector[triangleLoop].vertex)) << " ";
                        if ((*mStIt)->format == 0x11 || (*mStIt)->format == 0x1c) expFile << ((triangleLoop%2) ? ((*mStIt)->vertUVVector[triangleLoop+1].uv) : ((*mStIt)->vertUVVector[triangleLoop].uv)) << " ";
//                            if ((*mStIt)->format == 0x1c) expFile << ((triangleLoop%2) ? ((*mStIt)->vertUVVector[triangleLoop+1].color) : ((*mStIt)->vertUVVector[triangleLoop].color)) << " ";
                        expFile << " ";
                        expFile << ((triangleLoop%2) ? ((*mStIt)->vertUVVector[triangleLoop].vertex) : ((*mStIt)->vertUVVector[triangleLoop+1].vertex)) << " "
                                << ((triangleLoop%2) ? ((*mStIt)->vertUVVector[triangleLoop].vertex) : ((*mStIt)->vertUVVector[triangleLoop+1].vertex)) << " ";
                        if ((*mStIt)->format == 0x11 || (*mStIt)->format == 0x1c) expFile << ((triangleLoop%2) ? ((*mStIt)->vertUVVector[triangleLoop].uv) : ((*mStIt)->vertUVVector[triangleLoop+1].uv)) << " ";
//                            if ((*mStIt)->format == 0x1c) expFile << ((triangleLoop%2) ? ((*mStIt)->vertUVVector[triangleLoop].color) : ((*mStIt)->vertUVVector[triangleLoop+1].color)) << " ";
                        expFile << " ";
                        expFile << (*mStIt)->vertUVVector[triangleLoop+2].vertex << " "
                                << (*mStIt)->vertUVVector[triangleLoop+2].vertex << " ";
                        if ((*mStIt)->format == 0x11 || (*mStIt)->format == 0x1c) expFile << (*mStIt)->vertUVVector[triangleLoop+2].uv << " ";
//                            if ((*mStIt)->format == 0x1c) expFile <<  (*mStIt)->vertUVVector[triangleLoop+2].color << " ";
                        expFile << endl;
                    }
                    expFile << "\t\t\t\t</p>" << endl;
                    expFile << "\t\t\t</triangles>" << endl;
                }
            }
#endif

            // End of Geometry
            expFile << "\t\t</mesh>" << endl;
            expFile << "\t</geometry>" << endl;
        }
    }
    expFile << "</library_geometries>" << endl;

    // Export the scene
    expFile << "<library_visual_scenes>" << endl;
    expFile << "\t<visual_scene id=\"Scene\" name=\"Scene\">" << endl;
    expNode(expFile, model->m, model->t, 0);
    expFile << "\t</visual_scene>" << endl;
    expFile << "</library_visual_scenes>" << endl;

    // End of File
    expFile << "<scene>" << endl;
    expFile << "\t<instance_visual_scene url=\"#Scene\"/>" << endl;
    expFile << "</scene>" << endl;
    expFile << "</COLLADA>" << endl;

    expFile.close();
}

void colladaExport::expNode(ofstream &expFile, node *sm, textureData *td, int level) {
    string mname = getModelName(sm->myOfs, sm->nodeID);
    string matName;

    expFile << string(level, '\t') << "\t\t<node id=\"" << mname << "\" name=\"" << mname << "\" type=\"NODE\">" << endl;
    expFile << string(level, '\t') << "\t\t\t<translate sid=\"location\">" << sm->x << " " << sm->y << " " << sm->z << "</translate>" << endl;
    expFile << string(level, '\t') << "\t\t\t<rotate sid=\"rotationX\">1 0 0 " << sm->rotX << "</rotate>" << endl;
    expFile << string(level, '\t') << "\t\t\t<rotate sid=\"rotationY\">0 1 0 " << sm->rotY << "</rotate>" << endl;
    expFile << string(level, '\t') << "\t\t\t<rotate sid=\"rotationZ\">0 0 1 " << sm->rotZ << "</rotate>" << endl;
    expFile << string(level, '\t') << "\t\t\t<scale sid=\"scale\">" << sm->scaleX << " " << sm->scaleY << " " << sm->scaleZ << "</scale>" << endl;
    if (sm->pData) {
        expFile << string(level, '\t') << "\t\t\t<instance_geometry url=\"#" << mname << "-mesh\">" << endl;

        // go through ALL mesh objects
        for (vector<meshObject *>::iterator mObIt = sm->pData->meshObjectVector.begin(); mObIt != sm->pData->meshObjectVector.end(); mObIt++) {
            matName = mname + getMeshName((*mObIt)->getID());
            expFile << string(level, '\t') << "\t\t\t\t<bind_material>" << endl;
            expFile << string(level, '\t') << "\t\t\t\t\t<technique_common>" << endl;
            expFile << string(level, '\t') << "\t\t\t\t\t\t<instance_material symbol=\"" << matName<< "-material\" target=\"#" << matName << "-material\">" << endl;
            if ((*mObIt)->getType() == 0x2e || (*mObIt)->getType() == 0x0a) {
                expFile << string(level, '\t') << "\t\t\t\t\t\t\t<bind_vertex_input semantic=\"UVMap\" input_semantic=\"TEXCOORD\" input_set=\"0\"/>" << endl;
            }
            expFile << string(level, '\t') << "\t\t\t\t\t\t</instance_material>" << endl;
            expFile << string(level, '\t') << "\t\t\t\t\t</technique_common>" << endl;
            expFile << string(level, '\t') << "\t\t\t\t</bind_material>" << endl;
        }
        expFile << string(level, '\t') << "\t\t\t</instance_geometry>" << endl;
    }
    if (sm->pDown) expNode(expFile, sm->pDown, td, level+1);
    expFile << string(level, '\t') << "\t\t</node>" << endl;
    if (sm->pNext) expNode(expFile, sm->pNext, td, level);
}

string colladaExport::getModelName(unsigned long ofs, unsigned long id) {
    stringstream res;
    res << "f" << hex << fDB.getID(fname) << "n" << hex << nDB.getID(ofs, id);
    return res.str();
}

string colladaExport::getMeshName(unsigned long ofs) {
    stringstream res;
    res << "m" << hex << mDB.getID(ofs);
    return res.str();
}

string colladaExport::getTextureName(texn *name) {
    stringstream res;
    res << "f" << hex << fDB.getID(fname) << "t" << hex << texDB.getID(name);
    return res.str();
}

string colladaExport::getTextureFileName(texn *name) {
    stringstream res;
    res << "t" << hex << texDB.getID(name) << ".png";
    return res.str();
}

/*int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Shenmue MT5 Converter\nusage:%s <infile.mt5> <outfile.dae>\n", argv[0]);
        exit(-1);
    }
    b = new buffer(argv[1]);
    mt5 *mydata = new mt5();
    mydata->readData();
    cout << "Export Model" << endl;
    colladaExport *cExp = new colladaExport();
    cExp->exportModel(argv[2], mydata);

    delete cExp;
    delete mydata;

    return 0;
}*/
