#ifndef DATABASE_H
#define DATABASE_H

#include <vector>
#include <string>

#include "mt5parser.h"

class textureDB {
public:
    textureDB();
    ~textureDB();
    unsigned long getID(texn *t);
private:
    struct entry {
        unsigned long id;
        texn name;
    };
    std::vector<entry> tlist;
    unsigned long tval;
};


class nodeDB {
public:
    nodeDB();
    ~nodeDB();
    unsigned long getID(unsigned long nid, unsigned long sid);
    void clear();
private:
    struct entry{
        unsigned long id;
        unsigned long nid;          // Offset in file
        unsigned long sid;          // Sega ID for node
    };
    std::vector<entry> nlist;
    unsigned long nval;
};

class meshDB {
public:
    meshDB();
    ~meshDB();
    unsigned long getID(unsigned long mid);
    void clear();
private:
    struct entry {
        unsigned long id;
        unsigned long mid;          // Offset in file
    };
    std::vector<entry> mlist;
    unsigned long mval;
};

/*class materialDB {
public:
    materialDB();
    ~materialDB();
private:
    struct entry{
        unsigned long id;
        taparser *tadata;
        unsigned long texid;
    };
    std::vector<entry> matlist;
    unsigned long matval;
};
*/
class fileDB {
public:
    fileDB();
    ~fileDB();
    unsigned long getID(std::string name);
private:
    struct entry {
        unsigned long id;
        std::string name;
    };
    std::vector<entry> flist;
    unsigned long fval;
};

extern textureDB texDB;
extern fileDB fDB;
extern nodeDB nDB;
extern meshDB mDB;
//extern materialDB matDB;

#endif // DATABASE_H
