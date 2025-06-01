typedef enum {
  CONFIG_SUCCESS = 0
} config_result;

typedef struct {
  char *path;
} config;

config_result config_load(config **cfg);
config_result config_free(config **cfg);
