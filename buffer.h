#ifndef BUFFER_H
#define BUFFER_H

class buffer {
public:
    buffer(const char *fname);
    buffer(const buffer *orig, const unsigned long ofs, unsigned long bsize);
    ~buffer();

    bool seek(unsigned long pos);
    unsigned long tell();
    bool eof() const;
    unsigned char rb();
    unsigned short rw();
    unsigned long rl();
    float rf();
    const unsigned char *ptr();
    const unsigned long blen() const;

    void dump(const char *fname, unsigned long ofs, unsigned long writeSize);
private:
    unsigned char *buf;
    unsigned long cursor;
    unsigned long buflen;
};

extern buffer *b;

#endif // BUFFER_H
