typedef enum {
  CONFIG_SUCCESS,
  CONFIG_UNKNOWN_ERROR,
  CONFIG_MALLOC_ERROR,
} config_result;

const char *CONFIG_RESULT_STRINGS[] = {
  "Success, no error occured.",
  "Error, unknown error occurred.",
  "Error, failed to allocate memory for configuration.",
};

typedef struct {
  char *path;
} config;

config_result config_load(config **cfg);
config_result config_free(config **cfg);
void config_print_error(config_result result, const char *filename, int32_t line);
