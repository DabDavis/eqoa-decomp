/* minigzip.c -- SETUP 0x00582320 .. 0x005824D0: zlib 1.1.4's minigzip.c, cut
   down for the PS2 -- no exits, no program name, no unlink, and the two
   uncompress halves emptied.  C, so the names are unmangled. */
#include <stdio.h>
#include <string.h>
#include "zlib.h"

#define GZ_SUFFIX ".gz"
#define BUFLEN      16384
#define MAX_NAME_LEN 1024

void error(const char *msg);
void gz_compress(FILE *in, gzFile out);
void gz_uncompress(gzFile in, FILE *out);
void file_compress(char *file, char *mode);
void file_uncompress(char *file);

/* 0x00582320. */
void error(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
}

/* 0x00582350. */
void gz_compress(FILE *in, gzFile out)
{
    char buf[BUFLEN];
    int len;
    int err;

    for (;;) {
        len = fread(buf, 1, sizeof(buf), in);
        if (ferror(in)) {
            perror("fread");
        }
        if (len == 0) break;

        if (gzwrite(out, buf, (unsigned)len) != len) error(gzerror(out, &err));
    }
    fclose(in);
    if (gzclose(out) != Z_OK) error("failed gzclose");
}

/* 0x00582418. */
void gz_uncompress(gzFile in, FILE *out)
{
}

/* 0x00582420. */
void file_compress(char *file, char *mode)
{
    char outfile[MAX_NAME_LEN];
    FILE *in;
    gzFile out;

    strcpy(outfile, file);
    strcat(outfile, GZ_SUFFIX);

    in = fopen(file, "rb");
    out = gzopen(outfile, mode);
    if (out == NULL) {
        fprintf(stderr, "Can't gzopen %s\n", outfile);
    }
    gz_compress(in, out);
}

/* 0x005824C8. */
void file_uncompress(char *file)
{
}
