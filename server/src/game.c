static volatile bool intCaught = false;

void intHandler(int _dummy) {
    _dummy += 0; // avoid unused parameter warning
    intCaught = true;
}

void *print_message_function( void *ptr )
{
  state *st = (state *) ptr;

  while(intCaught == false) {
    st->count += 1;
    printf("count: %d\n", st->count);
    sleep(1);
  }

  return NULL;
}

int main(int argc, char *argv[]) {

  signal(SIGINT, intHandler);   // should use sigaction

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <state_file>\n", argv[0]);
    return EXIT_FAILURE;
  }

  config *cfg = NULL;
  config_result result = config_load(&cfg);
  if(result != CONFIG_SUCCESS) {
    config_print_error(result, __FILE__, __LINE__);
    return EXIT_FAILURE;
  }

  state *st = NULL;
  state_result st_result = state_load(argv[1], &st);
  if(st_result != STATE_SUCCESS) {
    state_print_error(st_result, __FILE__, __LINE__);
    config_free(&cfg);
    return EXIT_FAILURE;
  }

  puts(cfg->path);

  int  iret1;
  pthread_t thread1;
  iret1 = pthread_create( &thread1, NULL, print_message_function, (void*) st);

  /* network code */

  pthread_join( thread1, NULL);

  st_result = state_write(argv[2], &st);
  state_free(&st);
  config_free(&cfg);
}

