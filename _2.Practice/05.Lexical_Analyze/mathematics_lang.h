#ifndef MATHEMATICS_LANG_H
#define MATHEMATICS_LANG_H

#include <stdlib.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

/*

 exp = '(' exp ')' | term
 term = factor | exp '+' exp
 factor = func | exp '*' exp
 func = id '(' exp ')' | num
 digit = '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9'
 num = digit '.'? digit*
 id  = 'A' | ... 'Z'  | 'a' | ... 'z'
 ws = ' ' | '\t' | '\r' | '\n'

 */

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

