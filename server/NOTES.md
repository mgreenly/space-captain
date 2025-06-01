

/server
    /c

/client
    /cli-client
    /tui-client
    /gfx-client
    /web-client
    /win-client
    /mac-client


The server is only using environment variables for config
  .envrc

The server is almost certainly going to be distrubuted via a docker image



start
read environment variables to setup config
read state
read wal
enter main loop
    accepting/process connections
asked to shutdown
    close connections
    flush wal
    write state

start
read env-vars, read config, cli args override
enter game loop
   send commands
   updating local state view
shutting down
