#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>

config_result config_load(config **cfg) {
  (*cfg) = malloc(sizeof(config));
  if (*cfg == NULL) {
    return CONFIG_MALLOC_ERROR;
  }

  (*cfg)->path = getenv("PATH");

  return CONFIG_SUCCESS;
}

config_result config_free(config **cfg) {
  assert(*cfg != NULL);

  free(*cfg);

  return CONFIG_SUCCESS;
}

void config_print_error(config_result result, const char *filename, int32_t line) {
  fprintf(stderr, "%s (%s:%" PRId32 ")\n", CONFIG_RESULT_STRINGS[result], filename, line);
}
