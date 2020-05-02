#ifndef SAS_LOG_H
#define SAS_LOG_H

#define LOG_DEBUG "[DEBUG] "
#define LOG_INFO  "[INFO] "
#define LOG_WARN  "[WARN] "
#define LOG_ERR   "[ERROR] "

/**
 * Log the specified message. See sprintf(3) for formatting options.
 *
 * @param[in] msg The message to log.
 */
void sas_log(const char *restrict msg, ...);

#endif /* SAS_LOG_H */
