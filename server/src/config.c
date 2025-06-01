config_result config_load(config **cfg) {
  (*cfg)= malloc(sizeof(config));
    return CONFIG_MALLOC_ERROR;
  if (*cfg == NULL) {
    return CONFIG_MALLOC_ERROR;
  }

  (*cfg)->path = getenv("PATH");

  return CONFIG_SUCCESS;
}

config_result config_free(config **cfg) {
  free(*cfg);
  return CONFIG_SUCCESS;
}

void config_print_error(config_result result, const char *filename, int line) {
  fprintf(stderr, "%s (%s:%d)\n", CONFIG_RESULT_STRINGS[result], filename, line);
}
