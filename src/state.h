#ifndef STATE_H
#define STATE_H

typedef struct {
  int32_t count;
} state;

typedef enum {
  STATE_SUCCESS = 0,              // Operation completed successfully
  STATE_UNKNOWN_ERROR = 1,        // Unknown error occurred
  STATE_MALLOC_ERROR = 2,         // Memory allocation failed
  STATE_UNABLE_TO_OPEN_FILE = 3,  // Failed to open state file
  STATE_UNABLE_TO_READ_FILE = 4,  // Failed to read state file
  STATE_UNABLE_TO_WRITE_FILE = 5, // Failed to write state file
} state_result;

const char *STATE_RESULT_STRINGS[] = {
  "Success, no error occured.",
  "Error, unknown error occurred.",
  "Error, failed to allocate memory for state.",
  "Error, failed to open state file.",
  "Error, failed to read state file.",
  "Error, failed to write state file.",
};

state_result sc_state_write(char *filename, state **st);
state_result sc_state_load(char *filename, state **st);
state_result sc_state_nuke(state **st);
void sc_state_print_error(state_result result, const char *filename, int32_t line);

#endif // STATE_H
