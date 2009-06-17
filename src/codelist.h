#ifndef _CODELIST_H_
#define _CODELIST_H_

#include "mytypes.h"
#include "dllist.h"

/* Max game title length */
#define CL_TITLE_MAX	127
/* Max cheat description length */
#define CL_DESC_MAX	127

/**
 * code_t - a code object
 * @next: next code in list
 * @prev: previous code in list
 * @addr: code address
 * @val: code value
 * @tag: arbitrary information
 */
typedef struct _code {
	struct _code *next;
	struct _code *prev;
	u32 addr;
	u32 val;
	u32 tag;
} code_t;

/**
 * codelist_t - list of codes
 * @head: head of code list
 * @tail: tail of code list
 */
typedef struct _codelist {
	code_t *head;
	code_t *tail;
} codelist_t;

/**
 * cheat_t - a cheat object
 * @next: next cheat in list
 * @prev: previous cheat in list
 * @desc: cheat description
 * @codes: cheat codes
 * @tag: arbitrary information
 */
typedef struct _cheat {
	struct _cheat *next;
	struct _cheat *prev;
	char desc[CL_DESC_MAX + 1];
	codelist_t codes;
	u32 tag;
} cheat_t;

/**
 * cheatlist_t - list of cheats
 * @head: head of cheat list
 * @tail: tail of cheat list
 */
typedef struct _cheatlist {
	cheat_t *head;
	cheat_t *tail;
} cheatlist_t;

/**
 * game_t - a game object
 * @next: next game in list
 * @prev: previous game in list
 * @title: game title
 * @cheats: game cheats
 * @tag: arbitrary information
 */
typedef struct _game {
	struct _game *next;
	struct _game *prev;
	char title[CL_TITLE_MAX + 1];
	cheatlist_t cheats;
	u32 tag;
} game_t;

/**
 * gamelist_t - list of games
 * @head: head of game list
 * @tail: tail of game list
 */
typedef struct _gamelist {
	game_t *head;
	game_t *tail;
} gamelist_t;


game_t *build_game(const char *title, const cheatlist_t *cheats);
cheat_t *build_cheat(const char *desc, const codelist_t *codes);
code_t *build_code(u32 addr, u32 val);

void free_codes(codelist_t *list);
void free_cheats(cheatlist_t *list);
void free_games(gamelist_t *list);

void sort_games(gamelist_t *list);

game_t *find_game_by_title(const char *title, const gamelist_t *list);

#endif /* _CODELIST_H_ */
