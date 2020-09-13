#ifndef CC38_TEST_H
#define CC38_TEST_H

#include "38cc.h"
#include "vector.h"

typedef enum {
  MAME_A,
  MAME_B,
  MAME_C = 5,
  MAME_D,
} Mame;

struct Char {
  char val;
};
typedef struct Char Char;

typedef struct {
  short val;
} Short;

typedef struct {
  int val;
} Int;

typedef struct {
  short val1;
  char val2;
} ShortChar;

typedef struct {
  int val1;
  char val2;
} IntChar;

typedef struct {
  long val1;
  char val2;
} LongChar;

typedef struct {
  char val1;
  long val2;
  char val3;
} CharLongChar;

typedef struct {
  char val1;
  char val2;
  long val3;
} CharCharLong;

typedef struct {
  short val1;
  int val2;
  short val3;
} ShortIntShort;

typedef struct {
  int val1;
  short val2;
  short val3;
} IntShortShort;

typedef struct {
  long val1;
  long val2;
  long val3;
} LongLongLong;

typedef struct {
  int val1;
  int val2;
  int val3;
} IntIntInt;

typedef struct {
  char val1;
  short val2;
  int val3;
  long val4;
} CharShortIntLong;

typedef struct {
  long val1;
  int val2;
  short val3;
  char val4;
} LongIntShortChar;

typedef struct {
  char val1;
  long val2;
  int val3;
  short val4;
} CharLongIntShort;

typedef struct Nest {
  enum NestType {
    A = 1,
    B = 3,
  } type;
  char *name;
} Nest;

typedef union {
  char v1;
  int v2;
  short v3;
} IntUnion;

union LongUnion {
  char v1;
  int v2;
  long v3;
};
typedef union LongUnion LongUnion;

void assert(char* name, int ret);
void assertInt(char *, int, int);
void assertLong(char *, long, long);
void assertChar(char *name, char expect, char actual);

void hello();

void call_with_struct(CharLongIntShort *obj, long p);

#endif
