#include <stdlib.h>
#include <string.h>

#include "map.h"

u_int32_t _hash(char *key) {
  u_int32_t r = 2166136261;
  while (*key) {
    r ^= *key;
    r *= 16777619;
    key++;
  }
  return r;
}

Map *new_map() {
  Map *map = calloc(1, sizeof(Map));
  map->_table = NULL;
  map->_len = 0;
  map->size = 0;
  return map;
}

void _roundup(Map* map) {
  int new_len;
  if (map->_len == 0) {
    new_len = 8;
  } else {
    new_len = map->_len << 1;
  }
  int mask = new_len - 1; // 1000.. -> 111..
  MapEntry **new_table = calloc(new_len, sizeof(MapEntry*));
  if (map->_table) {
    for (int i = 0; i < map->_len; ++i) {
      MapEntry *entry = map->_table[i];
      while (entry) {
        u_int32_t h = _hash(entry->key) & mask;
        MapEntry *e = new_table[h];
        while (e->next) {
          e = e->next;
        }
        e->next = entry;

        entry = entry->next;
      }
    }
  }
  MapEntry **prev_table = map->_table;
  map->_table = new_table;
  free(prev_table);
  map->_len = new_len;
}

void *map_get(Map *map, char *key) {
  int mask = map->_len - 1; // 1000.. -> 111..
  u_int32_t h = _hash(key) & mask;

  MapEntry *e;
  e = map->_table[h];
  while (e) {
    if (strcmp(e->key, key) == 0) {
      return e->val;
    }
    e = e->next;
  }
  return NULL;
}

void map_put(Map *map, char *key, void *val) {
  if (map->_len == 0 || map->_len < (map->size << 1)) {
    _roundup(map);
  }

  int mask = map->_len - 1; // 1000.. -> 111..
  u_int32_t h = _hash(key) & mask;

  if (!map->_table[h]) {
    MapEntry *entry = calloc(1, sizeof(MapEntry));
    entry->key = key;
    entry->val = val;

    map->_table[h] = entry;
    map->size++;
    return;
  } else {
    MapEntry *e = map->_table[h];
    for (;;) {
      if (strcmp(e->key, key) == 0) {
        e->val = val;
        return;
      }
      if (!e->next) {
        break;
      }
      e = e->next;
    }
    MapEntry *entry = calloc(1, sizeof(MapEntry));
    entry->key = key;
    entry->val = val;

    e->next = entry;
    map->size++;
  }
}

