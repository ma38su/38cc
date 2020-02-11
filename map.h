typedef struct Map Map;
typedef struct MapEntry MapEntry;

struct Map {
  MapEntry **_table;
  int _len;
  int size;
};

struct MapEntry {
  char *key;
  void *val;
  MapEntry *next;
};

Map *new_map();
void map_put(Map *map, char *key, void *val);
void *map_get(Map *map, char *key);

