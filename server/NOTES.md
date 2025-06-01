

/server
    /c

/client
    /cli-client
    /tui-client
    /gfx-client
    /web-client
    /win-client
    /mac-clinet



The server is only using environment variables for config
  .envrc

start
read environment variables to setup config
read state
read wall
enter main loop
    accepting/process connections



start
read env-vars, read config, cli args override
enter game loop
   send commands
   updating local state view
shutting down
