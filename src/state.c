#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>

state_result state_write(char *filename, state ** st)
{
  FILE *file = fopen(filename, "w");
  if (file == NULL) {
    return STATE_UNABLE_TO_OPEN_FILE;
  }

  size_t records_written = fwrite(*st, sizeof(state), 1, file);
  if (records_written != 1) {
    fclose(file);
    return STATE_UNABLE_TO_WRITE_FILE;
  }

  fclose(file);
  return STATE_SUCCESS;
}

//
// state_load
//
state_result state_load(char *filename, state ** st)
{

  (*st) = malloc(sizeof(state));
  if (*st == NULL) {
    return STATE_MALLOC_ERROR;
  }

  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    free(*st);
    return STATE_UNABLE_TO_OPEN_FILE;
  }

  size_t records_read = fread(*st, sizeof(state), 1, file);
  if (records_read != 1) {
    fclose(file);
    free(*st);
    return STATE_UNABLE_TO_READ_FILE;
  }

  fclose(file);
  return STATE_SUCCESS;
}

//
// state_free
//
state_result state_free(state ** st)
{
  assert(*st != NULL);

  free(*st);

  return STATE_SUCCESS;
}

//
// state_print_error
//
void state_print_error(state_result result, const char *filename, int32_t line)
{
  fprintf(stderr, "%s (%s:%" PRId32 ")\n", STATE_RESULT_STRINGS[result], filename, line);
}
