//
// state_load
//
state_result state_load(char *filename, state **st) {

  (*st)= malloc(sizeof(state));
  if (*st == NULL) {
    puts("----------------Error, failed to allocate memory for state.");
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
  free(*st);
  return STATE_SUCCESS;
}

state_result state_free(state **st) {
  free(*st);
  return STATE_SUCCESS;
}

void state_print_error(state_result result, const char *filename, int line) {
  fprintf(stderr, "%s (%s:%d)\n", STATE_RESULT_STRINGS[result], filename, line);
}
