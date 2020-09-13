#include <stdio.h>
#include "test.h"

void call_with_struct(CharLongIntShort *obj, long p) {
  assertLong("test30-1", p, (long) obj);
  assertChar("test30-2", 'M', obj->val1);
  assertLong("test30-3", 1024, obj->val2);
  assertInt("test30-4", -131, obj->val3);
  assertInt("test30-5", 253, obj->val4);

}
