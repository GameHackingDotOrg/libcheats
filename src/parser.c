/*
 * parser.c - Parse cheats in text format
 *
 * Copyright (C) 2009-2012 Mathias Lafeldt <mathias.lafeldt@gmail.com>
 *
 * This file is part of libcheats.
 *
 * libcheats is licensed under the terms of the MIT License. See LICENSE file.
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cheatlist.h"
#include "mystring.h"

#ifdef _DEBUG
	#define D_PRINTF(args...)	printf(args)
#else
	#define D_PRINTF(args...)	do {} while (0)
#endif

/* Max line length to parse */
#define LINE_MAX	511

/* Tokens used for parsing */
#define TOK_NO				0
#define TOK_GAME_TITLE		1
#define TOK_CHEAT_DESC		2
#define TOK_CHEAT_CODE		4

/**
 * parser_ctx_t - parser context
 * @next: next expected token(s), see TOK_* flags
 * @game: ptr to current game object
 * @cheat: ptr to current cheat object
 * @code: ptr to current code object
 */
typedef struct _parser_ctx {
	int	top;
	int	next;
	game_t	*game;
	cheat_t	*cheat;
	code_t	*code;
} parser_ctx_t;

/* Information about last error */
char parse_error_text[256];
int parse_error_line;


/*
 * tok2str - Convert a token value to a string.
 */
static const char *tok2str(int tok)
{
	static const char *str[] = {
		"game title",
		"cheat description",
		"cheat code"
	};

	switch (tok) {
		case TOK_GAME_TITLE:
			return str[0];
		case TOK_CHEAT_DESC:
			return str[1];
		case TOK_CHEAT_CODE:
			return str[2];
		default:
			return NULL;
	}
}

/*
 * hexstr2bytes - Convert a hex string (eg: "DEAD1337C0DE") to a byte array (0xDEAD1337C0DE)
 */
static const char *hexstr2bytes(const char *s)
{
    size_t i;
    size_t length = strlen(s) / 2;
	char * buffer = (char*)malloc(length);
	memset(buffer, 0, length);
	
    for (i = 0; i < length; i++) {
        sscanf(s, "%2hhx", &buffer[i]);
        s += 2;
    }

	return buffer;
}

/*
 * is_cmt_str - Return non-zero if @s indicates a comment.
 */
static inline int is_cmt_str(const char *s)
{
	return (s[0] == '/' && s[1] == '/');
}

/*
 * is_game_title - Return non-zero if @s indicates a game title.
 *
 * Example: "TimeSplitters PAL"
 */
static inline int is_game_title(const char *s)
{
	size_t len = strlen(s);

	return ((len > 2) && (*s == '"') && (s[len - 1] == '"'));
}

/*
 * is_cheat_code - Return non-zero if @s indicates a cheat code.
 *
 * Example: 10B8DAFA 00003F00
 * Example: 0 00B8DAFA 3F00
 * Example: F10 00B80000 00B8DA00
 */
static inline int is_cheat_code(const char *s)
{
	int w = 0, i = 0;

	// Counts the number of words, returning 0 if it hits a non-hexadecimal digit
	while (*s) {
		if (isspace(*s)) {
			if (i > 0) {
				w++;
				i = 0;
			}
		}
		else if (!isxdigit(*s)) {
			return 0;
		}
		else {
			i++;
		}
		s++;
	}

	// Return true if we have either 2 or 3 words
	return w < 3 && w > 0;
}

/*
 * get_token - Get token value TOK_* for string @s.
 */
static int get_token(const char *s, int top)
{
	if (is_game_title(s))
		return TOK_GAME_TITLE;
	else if (is_cheat_code(s))
		return TOK_CHEAT_CODE;
	else
		return TOK_CHEAT_DESC;
}

/*
 * next_token - Return next expected token(s).
 */
static int next_token(int tok)
{
	switch (tok) {
	case TOK_GAME_TITLE:
		return TOK_GAME_TITLE | TOK_CHEAT_DESC;
	case TOK_CHEAT_DESC:
	case TOK_CHEAT_CODE:
		return TOK_GAME_TITLE | TOK_CHEAT_DESC | TOK_CHEAT_CODE;
	default:
		return TOK_NO;
	}
}

/*
 * __make_game - Create a game object from a game title.
 */
static game_t *__make_game(const char *title)
{
	char buf[GAME_TITLE_MAX + 1];

	/* Remove leading and trailing quotes from game title */
	strncpy(buf, title + 1, strlen(title) - 2);
	buf[strlen(title) - 2] = NUL;

	return make_game(buf, NULL, 0);
}

/*
 * __make_cheat - Create a cheat object from a cheat description.
 */
static cheat_t *__make_cheat(const char *desc)
{
	return make_cheat(desc, NULL, 0);
}

/*
 * __make_code - Create a code object from string @s.
 */
static code_t *__make_code(const char *s)
{
	int w = 0;
	int i[] = { 0, 0, 0 };
	char a1[LINE_MAX + 1], a2[LINE_MAX + 1];
	const char *line = s;
	uint32_t tag = 0;

	while (*s && w < 3) {
		if (isxdigit(*s)) {
			i[w]++;
		}
		else if (isspace(*s)) {
			if (i > 0) {
				w++;
			}
		}
		s++;
	}

	if (w == 1) {
		sscanf(line, "%s %s", (char*)a1, (char*)a2);
	}
	else if (w > 1) {
		sscanf(line, "%08x %s %s", &tag, a1, a2);
	}

	return make_code(hexstr2bytes(a1), strlen(a1)/2, hexstr2bytes(a2), strlen(a2)/2, tag);
}

/*
 * init_parser - Initialize the parser's context.  Must be called each time
 * before a file is parsed.
 */
static void init_parser(parser_ctx_t *ctx)
{
	if (ctx != NULL) {
		/* first token must be a game title */
		ctx->next = TOK_GAME_TITLE;
		ctx->game = NULL;
		ctx->cheat = NULL;
		ctx->code = NULL;
	}
}

/*
 * parse_err - Store information about a parse error.
 */
static void parse_err(int nl, const char *msg, ...)
{
	va_list ap;

	parse_error_line = nl;

	if (msg != NULL) {
		va_start(ap, msg);
		vsprintf(parse_error_text, msg, ap);
		va_end(ap);
	} else {
		strcpy(parse_error_text, "-");
	}

	D_PRINTF("line %i: %s\n", nl, parse_error_text);
}

/*
 * parse_line - Parse the current line and process the found token.
 */
static int parse_line(const char *line, int nl, parser_ctx_t *ctx, gamelist_t *list)
{
	int tok = get_token(line, ctx->top);
	D_PRINTF("%4i  %i  %s\n", nl, tok, line);

	/*
	 * Check if current token is expected - makes sure that the list
	 * operations succeed.
	 */
	if (!(ctx->next & tok)) {
		parse_err(nl, "parse error: %s invalid here", tok2str(tok));
		return -1;
	}

	/* Process actual token and add it to the list it belongs to. */
	switch (tok) {
	case TOK_GAME_TITLE:
		ctx->game = __make_game(line);
		if (ctx->game == NULL) {
			parse_err(nl, "make_game() failed");
			return -1;
		}
		GAMES_INSERT_TAIL(list, ctx->game);
		break;

	case TOK_CHEAT_DESC:
		ctx->cheat = __make_cheat(line);
		if (ctx->cheat == NULL) {
			parse_err(nl, "make_cheat() failed");
			return -1;
		}
		CHEATS_INSERT_TAIL(&ctx->game->cheats, ctx->cheat);
		break;

	case TOK_CHEAT_CODE:
		ctx->code = __make_code(line);
		if (ctx->code == NULL) {
			parse_err(nl, "make_code() failed");
			return -1;
		}
		CODES_INSERT_TAIL(&ctx->cheat->codes, ctx->code);
		break;
	}

	ctx->next = next_token(tok);

	return 0;
}

/**
 * parse_stream - Parse a text stream for cheats.
 * @list: list to add cheats to
 * @stream: stream to read cheats from
 * @return: 0: success, -1: error
 */
int parse_stream(gamelist_t *list, FILE *stream)
{
	parser_ctx_t ctx;
	char line[LINE_MAX + 1];
	int nl = 1;

	if (list == NULL || stream == NULL)
		return -1;

	init_parser(&ctx);

	while (fgets(line, sizeof(line), stream) != NULL) { /* Scanner */
		if (!is_empty_str(line)) {
			/* Screener */
			term_str(line, is_cmt_str);
			trim_str(line);

			/* Parser */
			if (strlen(line) > 0 && parse_line(line, nl, &ctx, list) < 0)
				return -1;
		}
		nl++;
	}

	return 0;
}

/**
 * parse_buf - Parse a text buffer for cheats.
 * @list: list to add cheats to
 * @buf: buffer holding text (must be NUL-terminated!)
 * @return: 0: success, -1: error
 */
int parse_buf(gamelist_t *list, const char *buf)
{
	parser_ctx_t ctx;
	char line[LINE_MAX + 1];
	int nl = 1;

	if (list == NULL || buf == NULL)
		return -1;

	init_parser(&ctx);

	while (*buf) {
		/* Scanner */
		int len = chr_idx(buf, LF);
		if (len < 0)
			len = strlen(line);
		else if (len > LINE_MAX)
			len = LINE_MAX;

		if (!is_empty_substr(buf, len)) {
			strncpy(line, buf, len);
			line[len] = NUL;

			/* Screener */
			term_str(line, is_cmt_str);
			trim_str(line);

			/* Parser */
			if (strlen(line) > 0 && parse_line(line, nl, &ctx, list) < 0)
				return -1;
		}
		nl++;
		buf += len + 1;
	}

	return 0;
}
