#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <ctype.h>   /* isblank() */
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL, strtod() */

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_true(lept_context* c, lept_value* v) {
    EXPECT(c, 't');
    if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_TRUE;
    return LEPT_PARSE_OK;
}

static int lept_parse_false(lept_context* c, lept_value* v) {
    EXPECT(c, 'f');
    if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 4;
    v->type = LEPT_FALSE;
    return LEPT_PARSE_OK;
}

static int lept_parse_null(lept_context* c, lept_value* v) {
    EXPECT(c, 'n');
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_NULL;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    const char* end;
    const char *origin = c->json;

    if (*c->json == '-')
        c->json++;

    /**
     * integer part
     */

    // at least one digit is required in the integer part
    if (!isdigit(*c->json))
        return LEPT_PARSE_INVALID_VALUE;

    if (*c->json == '0') {
        // leading 0s are not allowed
        if (isdigit(*(c->json + 1)))
            return LEPT_PARSE_INVALID_VALUE;
    }

    c->json++;
    // go through integer part
    while (isdigit(*c->json))
        c->json++;

    /**
     * frag part
     */
    if (*c->json == '.') {
        c->json++;
        // at least one digit is required in the frag part
        if (!isdigit(*c->json))
            return LEPT_PARSE_INVALID_VALUE;

        // go through frag part
        c->json++;
        while (isdigit(*c->json))
            c->json++;
    }

    /**
     * exponent part
     */
    if (*c->json == 'e' || *c->json == 'E') {
        c->json++;
        if (*c->json == '+' || *c->json == '-')
            c->json++;

        // NOTE!: leading 0s **is allowed** in the exponent part

        // at least one digit is required in the exponent part
        if (!isdigit(*c->json))
            return LEPT_PARSE_INVALID_VALUE;

        c->json++;
        while (isdigit(*c->json))
            c->json++;
    }

    // followed with unexpected chracters
    if (*c->json != '\0' && !isblank(*c->json))
        return LEPT_PARSE_INVALID_VALUE;

    end = c->json;
    c->json = origin;

    v->n = strtod(c->json, NULL);
    if (v->n == HUGE_VAL || v->n == -HUGE_VAL)
        return LEPT_PARSE_NUMBER_TOO_BIG;

    c->json = end;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_true(c, v);
        case 'f':  return lept_parse_false(c, v);
        case 'n':  return lept_parse_null(c, v);
        default:   return lept_parse_number(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}
