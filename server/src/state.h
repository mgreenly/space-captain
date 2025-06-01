typedef struct {
  int count[1];
} state;

typedef enum {
  STATE_SUCCESS,
  STATE_UNKNOWN_ERROR,
  STATE_MALLOC_ERROR,
  STATE_UNABLE_TO_OPEN_FILE,
  STATE_UNABLE_TO_READ_FILE,
} state_result;

const char *STATE_RESULT_STRINGS[] = {
  "Success, no error occured.",
  "Error, unknown error occurred.",
  "Error, failed to allocate memory for state.",
  "Error, failed to open state file.",
  "Error, failed to read state file.",
};

state_result state_load(char *filename, state **cfg);
state_result state_free(state **cfg);
void state_print_error(state_result result, const char *filename, int line);
