/**
 * @file dotenv.h
 * @author LuminariMUD Dev Team
 * @brief Simple .env file parser header
 * 
 * Part of the LuminariMUD distribution.
 */

#ifndef DOTENV_H
#define DOTENV_H

/* Function prototypes */
char *get_env_value(const char *key);
int get_env_int(const char *key, int default_value);
bool get_env_bool(const char *key, bool default_value);

#endif /* DOTENV_H */