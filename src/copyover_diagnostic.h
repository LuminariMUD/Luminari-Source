/* Copyover Diagnostic System Header */

#ifndef _COPYOVER_DIAGNOSTIC_H_
#define _COPYOVER_DIAGNOSTIC_H_

/* Initialize copyover diagnostics */
void init_copyover_diagnostics(void);

/* Log copyover phase transition */
void log_copyover_phase(const char *phase, const char *details);

/* Log file operations during copyover */
void log_copyover_file_op(const char *operation, const char *filename, int result, int error_code);

/* Log descriptor state during copyover */
void log_copyover_descriptors(int total, int playing, int saved);

/* Check system state before execl */
void check_pre_execl_state(void);

/* Log execl failure details */
void log_execl_failure(int error_code);

/* Close diagnostics */
void close_copyover_diagnostics(int success);

/* Function to analyze copyover failure after the fact */
void analyze_copyover_failure(void);

#endif /* _COPYOVER_DIAGNOSTIC_H_ */