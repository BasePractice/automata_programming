#ifndef MATHEMATICS_LANG_H
#define MATHEMATICS_LANG_H

#include <stdlib.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

/*
 Добавить знак возведения в степень, ^
 Добавить разбор числа со знаком (+10, -4,  5 * 7 - tg(0) - -30   )
 * */

enum LexerTokenType {
    TokenLPar, TokenRPar,
    TokenInt, TokenReal,
    TokenId,
    TokenDiv, TokenMul, TokenMinus, TokenPlus,
    End
};

struct LexerToken {
    const char *p;
    size_t it_start;
    size_t it_end;
    size_t line_no;
    enum LexerTokenType type;
    char *error;
};

struct LexerContext;

bool lexer_init_string(struct LexerContext **ctx, const char *text);

bool lexer_init_file(struct LexerContext **ctx, const char *filename);

void lexer_destroy(struct LexerContext **ctx);

bool lexer_next(struct LexerContext *ctx, struct LexerToken *token);

bool lexer_eof(struct LexerContext *ctx);

#if defined(__cplusplus)
}
#endif

#endif

