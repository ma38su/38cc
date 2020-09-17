#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "38cc.h"

char *read_file(char *path) {
// oepn file
  FILE *fp = fopen(path, "r"); 
  if (!fp) {
    error("cannot open %s: %s",
        path, strerror(errno));
  }

  // check length of file
  if (fseek(fp, 0, SEEK_END) == -1) {
    error("%s: fseek: %s", path, strerror(errno));
  }
  long size = ftell(fp);
  if (fseek(fp, 0, SEEK_SET) == -1) {
    error("%s: fseek: %s", path, strerror(errno));
  }

  char *buf = calloc(1, size + 2);
  fread(buf, size, 1, fp);

  // add '\n\0' to end of buf
  if (size == 0 || buf[size - 1] != '\n') {
    buf[size++] = '\n';
  }
  buf[size] = '\0';
  fclose(fp);

  return buf;
}

