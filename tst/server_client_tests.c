#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>

#include "vendor/unity.c"

// Test configuration
#define DEFAULT_NUM_CLIENTS 50
#define DEFAULT_RUNTIME_SECONDS 30
#define SERVER_STARTUP_DELAY 2  // seconds to wait for server to start
#define CLIENT_SPAWN_DELAY_MS 20  // milliseconds between client spawns

// Process tracking
typedef struct {
  pid_t pid;
  int pipe_fd;  // For capturing output
  char *name;
} process_info_t;

static process_info_t server_proc = {0};
static process_info_t *client_procs = NULL;
static int num_clients = DEFAULT_NUM_CLIENTS;
static int runtime_seconds = DEFAULT_RUNTIME_SECONDS;

// Helper function to check for error messages in output
static bool check_for_errors(int pipe_fd, const char *process_name) {
  char buffer[4096];
  ssize_t bytes_read;
  bool found_error = false;
  
  // Set non-blocking mode
  int flags = fcntl(pipe_fd, F_GETFL, 0);
  fcntl(pipe_fd, F_SETFL, flags | O_NONBLOCK);
  
  while ((bytes_read = read(pipe_fd, buffer, sizeof(buffer) - 1)) > 0) {
    buffer[bytes_read] = '\0';
    
    // Check for error indicators
    if (strstr(buffer, "ERROR") || strstr(buffer, "Error") || 
        strstr(buffer, "error") || strstr(buffer, "FAILED") ||
        strstr(buffer, "Failed") || strstr(buffer, "failed")) {
      printf("[%s] Error detected in output: %s\n", process_name, buffer);
      found_error = true;
    }
  }
  
  return found_error;
}

// Helper function to check if process is still running
static bool is_process_running(pid_t pid) {
  return kill(pid, 0) == 0;
}

// Helper function to terminate a process gracefully
static void terminate_process(process_info_t *proc) {
  if (proc->pid > 0) {
    // First try SIGTERM
    kill(proc->pid, SIGTERM);
    
    // Give it a moment to exit gracefully
    usleep(100000);  // 100ms
    
    // If still running, use SIGKILL
    if (is_process_running(proc->pid)) {
      kill(proc->pid, SIGKILL);
    }
    
    // Wait for process to exit
    int status;
    waitpid(proc->pid, &status, 0);
    
    // Close pipe
    if (proc->pipe_fd > 0) {
      close(proc->pipe_fd);
    }
  }
}

// Start the server process
static bool start_server(void) {
  int pipefd[2];
  if (pipe(pipefd) == -1) {
    return false;
  }
  
  pid_t pid = fork();
  if (pid == -1) {
    close(pipefd[0]);
    close(pipefd[1]);
    return false;
  }
  
  if (pid == 0) {
    // Child process
    close(pipefd[0]);  // Close read end
    
    // Redirect output to pipe
    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    close(pipefd[1]);
    
    // Execute server
    execl("bin/server", "server", NULL);
    
    // If we get here, exec failed
    exit(1);
  }
  
  // Parent process
  close(pipefd[1]);  // Close write end
  
  server_proc.pid = pid;
  server_proc.pipe_fd = pipefd[0];
  server_proc.name = "server";
  
  // Wait for server to start
  sleep(SERVER_STARTUP_DELAY);
  
  // Check if server is still running
  if (!is_process_running(server_proc.pid)) {
    close(server_proc.pipe_fd);
    return false;
  }
  
  return true;
}

// Start client processes
static bool start_clients(void) {
  client_procs = calloc(num_clients, sizeof(process_info_t));
  if (!client_procs) {
    return false;
  }
  
  for (int i = 0; i < num_clients; i++) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
      return false;
    }
    
    pid_t pid = fork();
    if (pid == -1) {
      close(pipefd[0]);
      close(pipefd[1]);
      return false;
    }
    
    if (pid == 0) {
      // Child process
      close(pipefd[0]);  // Close read end
      
      // Redirect output to pipe
      dup2(pipefd[1], STDOUT_FILENO);
      dup2(pipefd[1], STDERR_FILENO);
      close(pipefd[1]);
      
      // Execute client
      execl("bin/client", "client", NULL);
      
      // If we get here, exec failed
      exit(1);
    }
    
    // Parent process
    close(pipefd[1]);  // Close write end
    
    client_procs[i].pid = pid;
    client_procs[i].pipe_fd = pipefd[0];
    
    // Allocate name for this client
    char name[32];
    snprintf(name, sizeof(name), "client_%d", i + 1);
    client_procs[i].name = strdup(name);
    
    // Small delay between client starts to avoid thundering herd
    usleep(CLIENT_SPAWN_DELAY_MS * 1000);
  }
  
  return true;
}

// Monitor all processes
static bool monitor_processes(void) {
  time_t start_time = time(NULL);
  bool all_running = true;
  bool errors_found = false;
  
  printf("Monitoring %d clients for %d seconds...\n", num_clients, runtime_seconds);
  
  while (time(NULL) - start_time < runtime_seconds) {
    // Check server
    if (!is_process_running(server_proc.pid)) {
      printf("Server process died unexpectedly\n");
      all_running = false;
      break;
    }
    
    // Check for server errors
    if (check_for_errors(server_proc.pipe_fd, server_proc.name)) {
      errors_found = true;
    }
    
    // Check clients
    int alive_count = 0;
    for (int i = 0; i < num_clients; i++) {
      if (is_process_running(client_procs[i].pid)) {
        alive_count++;
        
        // Check for client errors
        if (check_for_errors(client_procs[i].pipe_fd, client_procs[i].name)) {
          errors_found = true;
        }
      }
    }
    
    // Log progress every 5 seconds
    int elapsed = time(NULL) - start_time;
    if (elapsed % 5 == 0) {
      printf("  [%d/%d seconds] %d/%d clients running\n", 
             elapsed, runtime_seconds, alive_count, num_clients);
    }
    
    if (alive_count < num_clients) {
      printf("Only %d/%d clients still running\n", alive_count, num_clients);
      all_running = false;
      break;
    }
    
    // Sleep briefly before next check
    usleep(100000);  // 100ms
  }
  
  return all_running && !errors_found;
}

// Cleanup all processes
static void cleanup_processes(void) {
  // Terminate clients
  if (client_procs) {
    for (int i = 0; i < num_clients; i++) {
      terminate_process(&client_procs[i]);
      free(client_procs[i].name);
    }
    free(client_procs);
    client_procs = NULL;
  }
  
  // Terminate server
  terminate_process(&server_proc);
}

// Test setup
void setUp(void) {
  // Reset process tracking
  memset(&server_proc, 0, sizeof(server_proc));
  client_procs = NULL;
}

// Test teardown
void tearDown(void) {
  cleanup_processes();
}

// Main functional test
void test_server_client_interaction(void) {
  // Check if binaries exist
  TEST_ASSERT_MESSAGE(access("bin/server", X_OK) == 0, 
                      "Server binary not found or not executable");
  TEST_ASSERT_MESSAGE(access("bin/client", X_OK) == 0, 
                      "Client binary not found or not executable");
  
  // Start server
  TEST_ASSERT_TRUE_MESSAGE(start_server(), "Failed to start server");
  
  // Start clients
  TEST_ASSERT_TRUE_MESSAGE(start_clients(), "Failed to start clients");
  
  // Monitor for the specified duration
  bool success = monitor_processes();
  
  // Clean up before assertions (so we don't leave processes running on failure)
  cleanup_processes();
  
  // Assert success
  TEST_ASSERT_TRUE_MESSAGE(success, 
    "Test failed: either processes died or errors were detected");
}

// Test with custom parameters
void test_server_client_custom_params(void) {
  // Test with fewer clients and shorter runtime
  num_clients = 10;
  runtime_seconds = 10;
  
  test_server_client_interaction();
  
  // Reset to defaults
  num_clients = DEFAULT_NUM_CLIENTS;
  runtime_seconds = DEFAULT_RUNTIME_SECONDS;
}

int main(void) {
  // Allow command line arguments to override defaults
  char *env_clients = getenv("TEST_NUM_CLIENTS");
  char *env_runtime = getenv("TEST_RUNTIME_SECONDS");
  
  if (env_clients) {
    num_clients = atoi(env_clients);
    if (num_clients <= 0) num_clients = DEFAULT_NUM_CLIENTS;
  }
  
  if (env_runtime) {
    runtime_seconds = atoi(env_runtime);
    if (runtime_seconds <= 0) runtime_seconds = DEFAULT_RUNTIME_SECONDS;
  }
  
  printf("Running server/client test with %d clients for %d seconds\n", 
         num_clients, runtime_seconds);
  
  UNITY_BEGIN();
  
  RUN_TEST(test_server_client_interaction);
  // RUN_TEST(test_server_client_custom_params);  // Commented out for faster testing
  
  return UNITY_END();
}