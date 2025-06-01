

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




SERVER
======

start
  read environment variables
    read state
      -read wal
        enter main loop
          -accepting connections
            recv message
              wal
                update-state
           stop accepting connections
    write state
exit






start
read env-vars, read config, cli args override
enter game loop
   send commands
   updating local state view
shutting down
