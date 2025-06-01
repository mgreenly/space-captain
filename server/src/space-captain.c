int main() {
  config *cfg = NULL;

  config_result result = config_load(&cfg);
  if(result != CONFIG_SUCCESS) {
    config_print_error(result, __FILE__, __LINE__);
    return EXIT_FAILURE;
  }

  cfg->path = getenv("PATH");

  puts(cfg->path);

  config_free(&cfg);
}

