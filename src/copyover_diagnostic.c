/* Copyover Diagnostic System
 * This file adds comprehensive logging and diagnostics to help identify
 * intermittent copyover failures in production environments.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "act.h"

#include <sys/resource.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

/* Diagnostic log file for copyover issues */
#define COPYOVER_DIAG_FILE "../log/copyover_diagnostic.log"
#define COPYOVER_LAST_STATE_FILE "../lib/misc/copyover_last_state.txt"

static FILE *diag_file = NULL;
static time_t copyover_start_time = 0;

/* Function to get current resource usage */
static void log_resource_usage(const char *phase)
{
  struct rusage usage;
  struct rlimit rlim;
  int open_fds = 0;
  int i;
  
  if (!diag_file)
    return;
    
  /* Get resource usage */
  if (getrusage(RUSAGE_SELF, &usage) == 0)
  {
    fprintf(diag_file, "[%s] Resource Usage:\n", phase);
    fprintf(diag_file, "  User CPU time: %ld.%06ld seconds\n", 
            usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
    fprintf(diag_file, "  System CPU time: %ld.%06ld seconds\n",
            usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
    fprintf(diag_file, "  Max resident set size: %ld KB\n", usage.ru_maxrss);
    fprintf(diag_file, "  Page faults (minor): %ld\n", usage.ru_minflt);
    fprintf(diag_file, "  Page faults (major): %ld\n", usage.ru_majflt);
    fprintf(diag_file, "  Voluntary context switches: %ld\n", usage.ru_nvcsw);
    fprintf(diag_file, "  Involuntary context switches: %ld\n", usage.ru_nivcsw);
  }
  
  /* Count open file descriptors */
  for (i = 0; i < 1024; i++)
  {
    if (fcntl(i, F_GETFD) != -1)
      open_fds++;
  }
  fprintf(diag_file, "  Open file descriptors: %d\n", open_fds);
  
  /* Check file descriptor limit */
  if (getrlimit(RLIMIT_NOFILE, &rlim) == 0)
  {
    fprintf(diag_file, "  FD limit (soft/hard): %ld/%ld\n", 
            (long)rlim.rlim_cur, (long)rlim.rlim_max);
  }
  
  /* Check memory limit */
  if (getrlimit(RLIMIT_AS, &rlim) == 0)
  {
    fprintf(diag_file, "  Memory limit (soft/hard): ");
    if (rlim.rlim_cur == RLIM_INFINITY)
      fprintf(diag_file, "unlimited/");
    else
      fprintf(diag_file, "%ld MB/", (long)(rlim.rlim_cur / (1024*1024)));
      
    if (rlim.rlim_max == RLIM_INFINITY)
      fprintf(diag_file, "unlimited\n");
    else
      fprintf(diag_file, "%ld MB\n", (long)(rlim.rlim_max / (1024*1024)));
  }
  
  /* Check core dump limit */
  if (getrlimit(RLIMIT_CORE, &rlim) == 0)
  {
    fprintf(diag_file, "  Core dump limit: ");
    if (rlim.rlim_cur == 0)
      fprintf(diag_file, "disabled\n");
    else if (rlim.rlim_cur == RLIM_INFINITY)
      fprintf(diag_file, "unlimited\n");
    else
      fprintf(diag_file, "%ld MB\n", (long)(rlim.rlim_cur / (1024*1024)));
  }
  
  fflush(diag_file);
}

/* Initialize copyover diagnostics */
void init_copyover_diagnostics(void)
{
  char timestamp[100];
  time_t now;
  struct tm *tm_info;
  
  /* Open diagnostic log file in append mode */
  diag_file = fopen(COPYOVER_DIAG_FILE, "a");
  if (!diag_file)
  {
    log("SYSERR: Could not open copyover diagnostic file: %s", strerror(errno));
    return;
  }
  
  /* Log start of copyover */
  now = time(NULL);
  copyover_start_time = now;
  tm_info = localtime(&now);
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
  
  fprintf(diag_file, "\n========================================\n");
  fprintf(diag_file, "COPYOVER DIAGNOSTIC START: %s\n", timestamp);
  fprintf(diag_file, "========================================\n");
  
  /* Log initial state */
  log_resource_usage("INIT");
  
  /* Log environment variables that might affect copyover */
  fprintf(diag_file, "\nEnvironment:\n");
  fprintf(diag_file, "  PATH=%s\n", getenv("PATH") ? getenv("PATH") : "(not set)");
  fprintf(diag_file, "  PWD=%s\n", getenv("PWD") ? getenv("PWD") : "(not set)");
  fprintf(diag_file, "  USER=%s\n", getenv("USER") ? getenv("USER") : "(not set)");
  fprintf(diag_file, "  HOME=%s\n", getenv("HOME") ? getenv("HOME") : "(not set)");
  
  fflush(diag_file);
}

/* Log copyover phase transition */
void log_copyover_phase(const char *phase, const char *details)
{
  char timestamp[100];
  time_t now;
  struct tm *tm_info;
  FILE *state_file;
  
  if (!diag_file)
    return;
    
  now = time(NULL);
  tm_info = localtime(&now);
  strftime(timestamp, sizeof(timestamp), "%H:%M:%S", tm_info);
  
  fprintf(diag_file, "\n[%s] PHASE: %s\n", timestamp, phase);
  if (details)
    fprintf(diag_file, "  Details: %s\n", details);
  
  fprintf(diag_file, "  Elapsed time: %ld seconds\n", (long)(now - copyover_start_time));
  
  /* Log resource usage for this phase */
  log_resource_usage(phase);
  
  fflush(diag_file);
  
  /* Also write current state to a separate file for post-mortem analysis */
  state_file = fopen(COPYOVER_LAST_STATE_FILE, "w");
  if (state_file)
  {
    fprintf(state_file, "Last Phase: %s\n", phase);
    fprintf(state_file, "Timestamp: %ld\n", (long)now);
    if (details)
      fprintf(state_file, "Details: %s\n", details);
    fclose(state_file);
  }
}

/* Log file operations during copyover */
void log_copyover_file_op(const char *operation, const char *filename, int result, int error_code)
{
  if (!diag_file)
    return;
    
  fprintf(diag_file, "  File Op: %s on '%s' - %s", 
          operation, filename, result == 0 ? "SUCCESS" : "FAILED");
  
  if (result != 0)
    fprintf(diag_file, " (errno=%d: %s)", error_code, strerror(error_code));
    
  fprintf(diag_file, "\n");
  fflush(diag_file);
}

/* Log descriptor state during copyover */
void log_copyover_descriptors(int total, int playing, int saved)
{
  if (!diag_file)
    return;
    
  fprintf(diag_file, "  Descriptors: Total=%d, Playing=%d, Saved=%d\n",
          total, playing, saved);
  fflush(diag_file);
}

/* Check system state before execl */
void check_pre_execl_state(void)
{
  struct stat st;
  char cwd[1024];
  sigset_t sigset;
  int i;
  
  if (!diag_file)
    return;
    
  fprintf(diag_file, "\nPre-execl() System Check:\n");
  
  /* Check current working directory */
  if (getcwd(cwd, sizeof(cwd)))
    fprintf(diag_file, "  Current directory: %s\n", cwd);
  else
    fprintf(diag_file, "  Current directory: UNKNOWN (errno=%d)\n", errno);
  
  /* Check if binary exists and permissions */
  if (stat(EXE_FILE, &st) == 0)
  {
    fprintf(diag_file, "  Binary exists: YES\n");
    fprintf(diag_file, "  Binary size: %ld bytes\n", (long)st.st_size);
    fprintf(diag_file, "  Binary permissions: %o\n", st.st_mode & 0777);
    fprintf(diag_file, "  Binary executable: %s\n", 
            (st.st_mode & S_IXUSR) ? "YES" : "NO");
  }
  else
  {
    fprintf(diag_file, "  Binary exists: NO (errno=%d: %s)\n", 
            errno, strerror(errno));
  }
  
  /* Check signal mask */
  if (sigprocmask(SIG_BLOCK, NULL, &sigset) == 0)
  {
    fprintf(diag_file, "  Blocked signals:");
    for (i = 1; i < 32; i++)
    {
      if (sigismember(&sigset, i))
        fprintf(diag_file, " %d", i);
    }
    fprintf(diag_file, "\n");
  }
  
  /* Check copyover file */
  if (stat(COPYOVER_FILE, &st) == 0)
  {
    fprintf(diag_file, "  Copyover file exists: YES\n");
    fprintf(diag_file, "  Copyover file size: %ld bytes\n", (long)st.st_size);
  }
  else
  {
    fprintf(diag_file, "  Copyover file exists: NO\n");
  }
  
  fflush(diag_file);
}

/* Log execl failure details */
void log_execl_failure(int error_code)
{
  if (!diag_file)
    return;
    
  fprintf(diag_file, "\nEXECL FAILED!\n");
  fprintf(diag_file, "  Error code: %d\n", error_code);
  fprintf(diag_file, "  Error message: %s\n", strerror(error_code));
  
  /* Provide specific guidance based on error */
  switch(error_code)
  {
    case EACCES:
      fprintf(diag_file, "  Diagnosis: Permission denied - check file permissions\n");
      break;
    case ENOENT:
      fprintf(diag_file, "  Diagnosis: File not found - check path and working directory\n");
      break;
    case ENOEXEC:
      fprintf(diag_file, "  Diagnosis: Not executable - check binary format\n");
      break;
    case E2BIG:
      fprintf(diag_file, "  Diagnosis: Argument list too long\n");
      break;
    case ENOMEM:
      fprintf(diag_file, "  Diagnosis: Out of memory\n");
      break;
    case ETXTBSY:
      fprintf(diag_file, "  Diagnosis: Text file busy - binary may be in use\n");
      break;
    default:
      fprintf(diag_file, "  Diagnosis: Unknown error\n");
  }
  
  fflush(diag_file);
}

/* Close diagnostics */
void close_copyover_diagnostics(int success)
{
  char timestamp[100];
  time_t now;
  struct tm *tm_info;
  
  if (!diag_file)
    return;
    
  now = time(NULL);
  tm_info = localtime(&now);
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
  
  fprintf(diag_file, "\n========================================\n");
  fprintf(diag_file, "COPYOVER DIAGNOSTIC END: %s\n", timestamp);
  fprintf(diag_file, "Status: %s\n", success ? "SUCCESS" : "FAILED");
  fprintf(diag_file, "Total time: %ld seconds\n", (long)(now - copyover_start_time));
  fprintf(diag_file, "========================================\n\n");
  
  fclose(diag_file);
  diag_file = NULL;
}

/* Function to analyze copyover failure after the fact */
void analyze_copyover_failure(void)
{
  FILE *state_file;
  char line[256];
  
  log("Analyzing copyover failure...");
  
  /* Check if state file exists */
  state_file = fopen(COPYOVER_LAST_STATE_FILE, "r");
  if (state_file)
  {
    log("Last copyover state:");
    while (fgets(line, sizeof(line), state_file))
    {
      /* Remove newline */
      line[strcspn(line, "\n")] = 0;
      log("  %s", line);
    }
    fclose(state_file);
  }
  else
  {
    log("No copyover state file found");
  }
  
  /* Check if copyover file still exists (indicates failure during execl) */
  if (access(COPYOVER_FILE, F_OK) == 0)
  {
    struct stat st;
    if (stat(COPYOVER_FILE, &st) == 0)
    {
      log("Copyover file still exists (size: %ld bytes)", (long)st.st_size);
      log("This suggests execl() failed or new process crashed immediately");
    }
  }
}