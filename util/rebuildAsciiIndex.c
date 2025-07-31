/* ************************************************************************
*  file:  rebuildAsciiIndex.c                         Part of LuminariMUD *
*  Copyright (C) 1990, 2010 - see 'license.doc' for complete information. *
*  All Rights Reserved                                                    *
*                                                                         *
*  This utility rebuilds the ASCII player index file by scanning all      *
*  .plr files in the player directory and extracting key information      *
*  such as player ID, name, level, admin level, and last login time.      *
*                                                                         *
*  Updated: 2025 - Enhanced for LuminariMUD compatibility, fixed C90      *
*  compliance issues, improved error handling and documentation           *
************************************************************************* */

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#define READ_SIZE 256

/* Function prototypes */
int atoi(const char *str);
long atol(const char *str);
void walkdir(FILE *index_file, char *dir);
int get_line(FILE *fl, char *buf);
char *parsename(char *filename);
char *findLine(FILE *plr_file, char *tag);
long parseid(FILE *plr_file);
int parselevel(FILE *plr_file);
int parseadminlevel(FILE *plr_file, int level);
long parselast(FILE *plr_file);

/**
 * Main function for the rebuildAsciiIndex utility
 *
 * Rebuilds the ASCII player index file by scanning all .plr files
 * in the current directory and subdirectories, extracting player
 * information and writing it to the specified index file.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments:
 *             [1] output index file name
 * @return 0 on success, 1 on error
 */
int main(int argc, char **argv)
{
	FILE *index_file;

	if (argc == 1)
	{
		printf("Usage: %s <indexfile>\n", argv[0]);
		printf("\n");
		printf("Rebuilds ASCII player index by scanning .plr files.\n");
		printf("The index file will contain player ID, name, level,\n");
		printf("admin level, and last login information.\n");
		printf("\n");
		printf("Example: %s plr_index\n", argv[0]);
		return 0;
	}

	if (!(index_file = fopen(argv[1], "w")))
	{
		perror("error opening index file");
		return 1;
	}

	printf("Scanning player files and rebuilding index...\n");
	walkdir(index_file, ".");

	fprintf(index_file, "~\n");
	fclose(index_file);
	printf("Index rebuild complete.\n");
	return 0;
}

/**
 * Parse player name from filename
 *
 * Extracts the player name from a .plr filename by removing the extension.
 * Only processes files with .plr extension.
 *
 * @param filename The filename to parse
 * @return Player name without extension, or NULL if not a .plr file
 */
char *parsename(char *filename)
{
	static char copy[1024];
	char *extension;

	strcpy(copy, filename);
	extension = strchr(copy, '.');
	if (extension == NULL)
	{
		return NULL;
	}
	if (strcmp(".plr", extension))
	{
		return NULL;
	}
	*extension = '\0';
	return copy;
}

/**
 * Find a line in player file starting with specified tag
 *
 * @param plr_file Player file to search
 * @param tag Tag to search for (e.g., "Id  :", "Levl:")
 * @return Pointer to content after tag, or NULL if not found
 */
char *findLine(FILE *plr_file, char *tag)
{
	static char line[5000];
	rewind(plr_file);

	while (get_line(plr_file, line))
	{
		if (!strncmp(tag, line, strlen(tag)))
		{
			return line + strlen(tag);
		}
	}
	return NULL;
}

/**
 * Parse player ID from player file
 *
 * @param plr_file Player file to read from
 * @return Player ID number
 */
long parseid(FILE *plr_file)
{
	char *id_str = findLine(plr_file, "Id  :");
	return id_str ? atol(id_str) : 0;
}

/**
 * Parse player level from player file
 *
 * @param plr_file Player file to read from
 * @return Player level
 */
int parselevel(FILE *plr_file)
{
	char *level_str = findLine(plr_file, "Levl:");
	return level_str ? atoi(level_str) : 0;
}

/**
 * Parse admin level from player file
 *
 * @param plr_file Player file to read from
 * @param level Player's regular level (for fallback calculation)
 * @return Admin level
 */
int parseadminlevel(FILE *plr_file, int level)
{
	char *fromFile = findLine(plr_file, "Admn:");
	if (fromFile != NULL)
		return atoi(fromFile);

	/* Fallback: calculate from regular level for older files */
	if (level >= 30)
		return level - 30;
	else
		return 0;
}

/**
 * Parse last login time from player file
 *
 * @param plr_file Player file to read from
 * @return Last login timestamp
 */
long parselast(FILE *plr_file)
{
	char *last_str = findLine(plr_file, "Last:");
	return last_str ? atol(last_str) : 0;
}

/**
 * Recursively walk directory tree and process .plr files
 *
 * Scans the specified directory and all subdirectories for .plr files,
 * extracts player information from each file, and writes it to the index.
 *
 * @param index_file Output file for the index
 * @param dir Directory to scan
 */
void walkdir(FILE *index_file, char *dir)
{
	char filename_qfd[1000];
	struct dirent *dp;
	DIR *dfd;
	struct stat stbuf;
	char *name;
	FILE *plr_file;
	long id, last;
	int level, adminlevel;

	if ((dfd = opendir(dir)) == NULL)
	{
		fprintf(stderr, "Can't open %s\n", dir);
		return;
	}

	while ((dp = readdir(dfd)) != NULL)
	{
		sprintf(filename_qfd, "%s/%s", dir, dp->d_name);
		if (stat(filename_qfd, &stbuf) == -1)
		{
			fprintf(stdout, "Unable to stat file: %s\n", filename_qfd);
			continue;
		}

		if ((stbuf.st_mode & S_IFMT) == S_IFDIR)
		{
			if (!strcmp(".", dp->d_name) || !strcmp("..", dp->d_name))
				continue;

			walkdir(index_file, filename_qfd);
		}
		else
		{
			name = parsename(dp->d_name);

			if (name != NULL)
			{
				plr_file = fopen(filename_qfd, "r");
				if (!plr_file) {
					fprintf(stderr, "Warning: Could not open player file %s\n", filename_qfd);
					continue;
				}

				id = parseid(plr_file);
				level = parselevel(plr_file);
				adminlevel = parseadminlevel(plr_file, level);
				if (level > 30)
					level = 30;
				last = parselast(plr_file);

				fprintf(index_file, "%ld %s %d %d 0 %ld\n", id, name, level, adminlevel, last);

				fclose(plr_file);
			}
		}
	}
	closedir(dfd);
}

/**
 * Read a line from file, skipping comments and blank lines
 *
 * Reads lines from the file, skipping those that start with '*' (comments)
 * or are blank lines. Removes trailing newlines and carriage returns.
 *
 * @param fl File to read from
 * @param buf Buffer to store the line (must be at least READ_SIZE)
 * @return Number of lines read, or 0 on EOF
 */
int get_line(FILE *fl, char *buf)
{
	char temp[READ_SIZE];
	int lines = 0;
	int sl;

	do
	{
		if (!fgets(temp, READ_SIZE, fl))
			return (0);
		lines++;
	} while (*temp == '*' || *temp == '\n' || *temp == '\r');

	/* Remove trailing newlines and carriage returns */
	sl = strlen(temp);
	while (sl > 0 && (temp[sl - 1] == '\n' || temp[sl - 1] == '\r'))
		temp[--sl] = '\0';

	strcpy(buf, temp); /* strcpy: OK, if buf >= READ_SIZE (256) */
	return (lines);
}
