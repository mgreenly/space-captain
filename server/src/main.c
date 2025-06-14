#define  _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "config.h"
#include "game.h"
#include "network.h"
#include "state.h"

#include "config.c"
#include "game.c"
#include "network.c"
#include "state.c"

//
//
// main
//
//
int32_t main(int32_t argc, char *argv[]) {

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

  int32_t  iret1;
  pthread_t thread1;
  iret1 = pthread_create( &thread1, NULL, game_loop, (void*) st);

  network_loop();

  pthread_join( thread1, NULL);

  printf("iret1: %d\n", iret1);
  st_result = state_write(argv[2], &st);
  state_free(&st);
  config_free(&cfg);
}

