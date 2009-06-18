/*
 * libcheats.c - Library for reading, manipulating, and writing cheat codes in
 * text format
 *
 * Copyright (C) 2009 misfire <misfire@xploderfreax.de>
 *
 * This file is part of libcheats.
 *
 * libcheats is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libcheats is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libcheats.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include "cheatlist.h"
#include "dbgprintf.h"
#include "libcheats.h"
#include "readcheats.h"

/**
 * cheats_init - Initialize a cheats object.
 * @cheats: cheats
 */
void cheats_init(cheats_t *cheats)
{
	if (cheats != NULL) {
		memset(cheats, 0, sizeof(cheats_t));
		list_init(&cheats->games);
	}
}

/**
 * cheats_destroy - Destroy a cheats object, deallocate all memory associated
 * with it, but not including the cheats_t structure itself.
 * @cheats: cheats
 */
void cheats_destroy(cheats_t *cheats)
{
	if (cheats != NULL) {
		free_games(&cheats->games);
	}
}

/**
 * cheats_read - Read cheats from a stream.
 * @cheats: cheats
 * @stream: stream to read cheats from
 * @return: CHEATS_TRUE: success, CHEATS_FALSE: error
 */
int cheats_read(cheats_t *cheats, FILE *stream)
{
	if (cheats == NULL || stream == NULL)
		return CHEATS_FALSE;

	setbuf(stream, NULL);

	if (parse_stream(&cheats->games, stream) < 0) {
		strcpy(cheats->error_text, parse_error_text);
		cheats->error_line = parse_error_line;
		return CHEATS_FALSE;
	} else {
		cheats->error_text[0] = '\0';
		cheats->error_line = 0;
		return CHEATS_TRUE;
	}
}

/**
 * cheats_read_file - Read cheats from a text file.
 * @cheats: cheats
 * @filename: name of file to read cheats from
 * @return: CHEATS_TRUE: success, CHEATS_FALSE: error
 */
int cheats_read_file(cheats_t *cheats, const char *filename)
{
	FILE *fp;
	int ret;

	if (cheats == NULL || filename == NULL)
		return CHEATS_FALSE;

	fp = fopen(filename, "r");
	if (fp == NULL) {
		sprintf(cheats->error_text, "could not open input file %s", filename);
		return CHEATS_FALSE;
	}

	ret = cheats_read(cheats, fp);
	fclose(fp);
	return ret;
}

/**
 * cheats_read_buf - Read cheats from a text buffer.
 * @cheats: cheats
 * @buf: buffer holding text (must be NUL-terminated!)
 * @return: CHEATS_TRUE: success, CHEATS_FALSE: error
 */
int cheats_read_buf(cheats_t *cheats, const char *buf)
{
	if (cheats == NULL || buf == NULL)
		return CHEATS_FALSE;

	if (parse_buf(&cheats->games, buf) < 0) {
		strcpy(cheats->error_text, parse_error_text);
		cheats->error_line = parse_error_line;
		return CHEATS_FALSE;
	} else {
		cheats->error_text[0] = '\0';
		cheats->error_line = 0;
		return CHEATS_TRUE;
	}
}

/**
 * cheats_write - Write cheats to a stream.
 * @cheats: cheats
 * @stream: stream to write cheats to
 * @return: CHEATS_TRUE: success, CHEATS_FALSE: error
 */
int cheats_write(cheats_t *cheats, FILE *stream)
{
	game_t *game;
	cheat_t *cheat;
	code_t *code;

	if (cheats == NULL || stream == NULL)
		return CHEATS_FALSE;

	for (game = cheats->games.head; game != NULL; game = game->next) {
		fprintf(stream, "\"%s\"\n", game->title);
		for (cheat = game->cheats.head; cheat != NULL; cheat = cheat->next) {
			fprintf(stream, "%s\n", cheat->desc);
			for (code = cheat->codes.head; code != NULL; code = code->next) {
				fprintf(stream, "%08X %08X\n", code->addr, code->val);
			}
		}
		fprintf(stream, "\n//--------\n\n");
	}

	return CHEATS_FALSE;
}

/**
 * cheats_write_file - Write cheats to a text file.
 * @cheats: cheats
 * @filename: name of file to write cheats to
 * @return: CHEATS_TRUE: success, CHEATS_FALSE: error
 */
int cheats_write_file(cheats_t *cheats, const char *filename)
{
	FILE *fp;
	int ret;

	if (cheats == NULL || filename == NULL)
		return CHEATS_FALSE;

	fp = fopen(filename, "w");
	if (fp == NULL) {
		sprintf(cheats->error_text, "could not open output file %s", filename);
		return CHEATS_FALSE;
	}

	ret = cheats_write(cheats, fp);
	fclose(fp);
	return CHEATS_TRUE;
}

/**
 * cheats_error_text - Return the text of the last parse error.
 * @cheats: cheats
 * @return: error text
 */
const char *cheats_error_text(const cheats_t *cheats)
{
	if (cheats == NULL)
		return NULL;

	return cheats->error_text;
}

/**
 * cheats_error_line - Return the line number of the last parse error.
 * @cheats: cheats
 * @return: line number
 */
int cheats_error_line(const cheats_t *cheats)
{
	if (cheats == NULL)
		return -1;

	return cheats->error_line;
}