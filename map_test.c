#include <stdio.h>
#include "map.h"

int main() {
  Map *map = new_map();
  char *array[] = {"a1", "a2", "1a", "2a"};

  for (int i = 0; i < 4; ++i) {
    map_put(map, array[i], array[(i + 1)%4]);
  }

  map_put(map, "a2", "1fdsa");
  for (int i = 0; i < 4; ++i) {
    printf("%d: %s = %s\n", i, array[i], (char*) map_get(map, array[i]));
  }
}
