static volatile bool intCaught = false;

void
intHandler(int _dummy)
{
  _dummy += 0;                  // avoid unused parameter warning
  intCaught = true;
}

void *
game_loop(void *ptr)
{
  state *st = (state *) ptr;

  while (intCaught == false) {
    st->count += 1;
    printf("count: %d\n", st->count);
    sleep(1);
  }

  return NULL;
}
