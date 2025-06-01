config_result config_load(config **cfg) {
  (*cfg)= malloc(sizeof(config));
  (*cfg)->path = getenv("PATH");
  return CONFIG_SUCCESS;
}

config_result config_free(config **cfg) {
  free(*cfg);
  return CONFIG_SUCCESS;
}
