#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mathematics_lang.h"

struct LexerContext {
    const char *content;
    size_t content_size;
    size_t it;
    size_t line_no;
    size_t line_it;
};

bool lexer_init_string(struct LexerContext **ctx, const char *text) {
    if (ctx == 0)
        return false;
    if (text == 0)
        return false;
    (*ctx) = calloc(1, sizeof(struct LexerContext));
    (*ctx)->content = strdup(text);
    (*ctx)->content_size = strlen(text);
    (*ctx)->line_no = 1;
    (*ctx)->line_it = 0;
    return true;
}

bool lexer_init_file(struct LexerContext **ctx, const char *filename) {
    FILE *fd;
    if (ctx == 0)
        return false;
    if (filename == 0)
        return false;

    fd = fopen(filename, "rt");
    if (fd == 0)
        return false;
    (*ctx) = calloc(1, sizeof(struct LexerContext));
    fseek(fd, 0, SEEK_END);
    (*ctx)->content_size = (size_t) ftell(fd);
    fseek(fd, 0, SEEK_SET);
    (*ctx)->content = calloc((*ctx)->content_size, 1);
    fread((void *) (*ctx)->content, (*ctx)->content_size, 1, fd);
    fclose(fd);
    (*ctx)->line_no = 1;
    return true;
}

void lexer_destroy(struct LexerContext **ctx) {
    if (ctx != 0 && (*ctx) != 0) {
        free((void *) (*ctx)->content);
        free((*ctx));
        (*ctx) = 0;
    }
}

static bool lexer_symbol_next(struct LexerContext *ctx) {
    if (ctx != 0 && ctx->it < ctx->content_size) {
        ctx->it++;
        ctx->line_it++;
        return true;
    }
    return false;
}

inline static char lexer_symbol(struct LexerContext *ctx) {
    assert(ctx->it < ctx->content_size);
    return ctx->content[ctx->it];
}

inline static bool lexer_symbol_is(struct LexerContext *ctx, char original) {
    return lexer_symbol(ctx) == original;
}

inline static bool lexer_symbol_is_digit(struct LexerContext *ctx) {
    char symbol = lexer_symbol(ctx);
    return symbol >= '0' && symbol <= '9';
}

inline static bool lexer_symbol_is_dot(struct LexerContext *ctx) {
    return lexer_symbol_is(ctx, '.');
}

inline static bool lexer_symbol_is_lpar(struct LexerContext *ctx) {
    return lexer_symbol_is(ctx, '(');
}

inline static bool lexer_symbol_is_rpar(struct LexerContext *ctx) {
    return lexer_symbol_is(ctx, ')');
}

inline static bool lexer_symbol_is_mul(struct LexerContext *ctx) {
    return lexer_symbol_is(ctx, '*');
}

inline static bool lexer_symbol_is_plus(struct LexerContext *ctx) {
    return lexer_symbol_is(ctx, '+');
}

inline static bool lexer_symbol_is_minus(struct LexerContext *ctx) {
    return lexer_symbol_is(ctx, '-');
}

inline static bool lexer_symbol_is_div(struct LexerContext *ctx) {
    return lexer_symbol_is(ctx, '/');
}

inline static bool lexer_symbol_is_alpha(struct LexerContext *ctx) {
    char symbol = lexer_symbol(ctx);
    return (symbol >= 'A' && symbol <= 'Z') ||
           (symbol >= 'a' && symbol <= 'z');
}

inline static bool lexer_symbol_is_space(struct LexerContext *ctx) {
    char symbol = lexer_symbol(ctx);
    return symbol == ' ' || symbol == '\t';
}

inline static bool lexer_symbol_is_nl(struct LexerContext *ctx) {
    char symbol = lexer_symbol(ctx);
    return symbol == '\r' || symbol == '\n';
}

inline static bool lexer_symbol_parse_numeric(struct LexerContext *ctx, struct LexerToken *tok) {
    bool ret = true;
    tok->p = ctx->content + ctx->it;
    tok->it_start = ctx->it;
    tok->type = TokenInt;
    while (lexer_symbol_next(ctx)) {
        if (lexer_symbol_is_digit(ctx))
            continue;
        if (lexer_symbol_is_dot(ctx)) {
            if (tok->type == TokenReal) {
                tok->error = "Duplicate dot present on input sequence";
                ret = false;
                break;
            }
            tok->type = TokenReal;
            continue;
        }
        break;
    }
    tok->it_end = ctx->it;
    return ret;
}

inline static bool lexer_symbol_parse_id(struct LexerContext *ctx, struct LexerToken *tok) {
    bool ret = true;
    tok->type = TokenId;
    tok->p = ctx->content + ctx->it;
    tok->it_start = ctx->it;
    while (lexer_symbol_next(ctx)) {
        if (lexer_symbol_is_alpha(ctx))
            continue;
        if (lexer_symbol_is_digit(ctx))
            continue;
        if (lexer_symbol_is(ctx, '_'))
            continue;
        break;
    }
    tok->it_end = ctx->it;
    return ret;
}

inline static bool lexer_symbol_skip(struct LexerContext *ctx) {
    while (!lexer_eof(ctx) && (lexer_symbol_is_space(ctx) || lexer_symbol_is_nl(ctx))) {
        if (lexer_symbol_is_nl(ctx)) {
            ctx->line_no++;
            ctx->line_it = 0;
        }
        lexer_symbol_next(ctx);
    }
    return true;
}

bool lexer_eof(struct LexerContext *ctx) {
    if (ctx != 0 && ctx->it < ctx->content_size) {
        return false;
    }
    return true;
}

bool lexer_next(struct LexerContext *ctx, struct LexerToken *token) {
    if (token == 0)
        return false;
    token->type = End;
    if (lexer_eof(ctx))
        return false;
    lexer_symbol_skip(ctx);
    token->p = ctx->content + ctx->it;
    token->it_start = ctx->it;
    token->it_end = ctx->it;
    token->line_no = ctx->line_no;
    token->error = 0;
    if (lexer_symbol_is_plus(ctx)) {
        token->type = TokenPlus;
        lexer_symbol_next(ctx);
        return true;
    } else if (lexer_symbol_is_minus(ctx)) {
        token->type = TokenMinus;
        lexer_symbol_next(ctx);
        return true;
    } else if (lexer_symbol_is_div(ctx)) {
        token->type = TokenDiv;
        lexer_symbol_next(ctx);
        return true;
    } else if (lexer_symbol_is_mul(ctx)) {
        token->type = TokenMul;
        lexer_symbol_next(ctx);
        return true;
    } else if (lexer_symbol_is_lpar(ctx)) {
        token->type = TokenLPar;
        lexer_symbol_next(ctx);
        return true;
    } else if (lexer_symbol_is_rpar(ctx)) {
        token->type = TokenRPar;
        lexer_symbol_next(ctx);
        return true;
    } else if (lexer_symbol_is_rpar(ctx)) {
        token->type = TokenMul;
        lexer_symbol_next(ctx);
        return true;
    } else if (lexer_symbol_is_digit(ctx)) {
        return lexer_symbol_parse_numeric(ctx, token);
    } else if (lexer_symbol_is_alpha(ctx)) {
        return lexer_symbol_parse_id(ctx, token);
    }
    token->error = "Unknown input token";
    return false;
}
