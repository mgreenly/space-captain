int main() {
  config *cfg = NULL;

  config_result result = config_load(&cfg);
  if(result != CONFIG_SUCCESS) {
    fprintf(stderr, "Failed to load configuration\n");
    return EXIT_FAILURE;
  }

  cfg->path = getenv("PATH");

  puts(cfg->path);

  config_free(&cfg);
}
