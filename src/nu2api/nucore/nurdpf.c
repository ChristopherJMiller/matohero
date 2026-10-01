#include "nurdp.h"
#include "nustring.h"

#include <stddef.h>

/* Float expression evaluator behind NuFParGetFloatRDP: numbers, names (through a getvar
 * callback), + - * / and brackets. Based on opensagadev/saga
 * src/nu2api/nucore/nurdpf.cpp (GPL-3.0); names and locals from the PS2 prototype. */

typedef struct rdpfctxt_s {
    int curr_tok;
    float curr_num;
    char *txtbuff;
    int idx;
    nurdpgetvarfn *GetVar;
} RDPFCTXT;

enum {
    TOK_ADD = 0,
    TOK_SUB = 1,
    TOK_MUL = 2,
    TOK_DIV = 3,
    TOK_LBK = 4,
    TOK_RBK = 5,
    TOK_END = 6,
    TOK_NUM = 7,
    TOK_NAME = 8,
    TOK_CNT = 9
};

static __inline int isnumordot(char c) {
    if ((c >= '0' && c <= '9') || c == '.') {
        return 1;
    }
    return 0;
}

// FUNCTION: BIONICLE 0x56ca30
static void get_tok(RDPFCTXT *c) {
    float adp;
    char varname[256];
    int varidx;

    /* Meant to skip whitespace, but it increments the character rather than the index:
     * a space becomes '!' and a tab '\n', which ends the loop and is then ignored. */
    for (;;) {
        if (c->txtbuff[c->idx] == '\0') {
            c->curr_tok = TOK_END;
            return;
        }
        if (c->txtbuff[c->idx] == ' ') {
            c->txtbuff[c->idx]++;
            continue;
        }
        if (c->txtbuff[c->idx] == '\t') {
            c->txtbuff[c->idx]++;
            continue;
        }
        break;
    }

    switch (c->txtbuff[c->idx]) {
    case ';':
        c->curr_tok = TOK_END;
        break;
    case '+':
        c->curr_tok = TOK_ADD;
        c->idx++;
        break;
    case '-':
        c->curr_tok = TOK_SUB;
        c->idx++;
        break;
    case '*':
        c->curr_tok = TOK_MUL;
        c->idx++;
        break;
    case '/':
        c->curr_tok = TOK_DIV;
        c->idx++;
        break;
    case '(':
        c->curr_tok = TOK_LBK;
        c->idx++;
        break;
    case ')':
        c->curr_tok = TOK_RBK;
        c->idx++;
        break;
    default:
        break;
    case '.':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        c->curr_tok = TOK_NUM;
        c->curr_num = 0.0f;
        adp = 0.0f;
        while (isnumordot(c->txtbuff[c->idx])) {
            if (c->txtbuff[c->idx] == '.') {
                adp = 0.1f;
            } else if (adp != 0.0f) {
                c->curr_num += (float)(c->txtbuff[c->idx] - '0') * adp;
                adp *= 0.1f;
            } else {
                c->curr_num *= 10.0f;
                c->curr_num += (float)(c->txtbuff[c->idx] - '0');
            }
            c->idx++;
        }
        break;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P':
    case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':
    case '_':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
    case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z':
        varidx = 0;
        while (NuIsAlNum(c->txtbuff[c->idx])) {
            varname[varidx] = c->txtbuff[c->idx];
            c->idx++;
            varidx++;
        }
        varname[varidx] = '\0';
        c->curr_tok = TOK_NUM;
        c->curr_num = 0.0f;
        (*c->GetVar)(varname, &c->curr_num, NULL);
        return;
    }
}

static float expr(RDPFCTXT *c);

// FUNCTION: BIONICLE 0x56cd30
static float prim(RDPFCTXT *c) {
    float e;

    switch (c->curr_tok) {
    case TOK_NUM:
    case TOK_NAME:
        get_tok(c);
        return c->curr_num;
    case TOK_SUB:
        get_tok(c);
        return -prim(c);
    case TOK_LBK:
        get_tok(c);
        e = expr(c);
        get_tok(c);
        return e;
    case TOK_END:
        return 1.0f;
    default:
        return 0.0f;
    }
}

// FUNCTION: BIONICLE 0x56cdb0
static float term(RDPFCTXT *c) {
    float left;

    left = prim(c);
    for (;;) {
        switch (c->curr_tok) {
        case TOK_MUL:
            get_tok(c);
            left *= prim(c);
            break;
        case TOK_DIV:
            get_tok(c);
            left /= prim(c);
            break;
        default:
            return left;
        }
    }
}

// FUNCTION: BIONICLE 0x56ce00
static float expr(RDPFCTXT *c) {
    float left;

    left = term(c);
    for (;;) {
        switch (c->curr_tok) {
        case TOK_ADD:
            get_tok(c);
            left += term(c);
            break;
        case TOK_SUB:
            get_tok(c);
            left -= term(c);
            break;
        default:
            return left;
        }
    }
}

// FUNCTION: BIONICLE 0x56ce50
float NuRDPFVar(char *txt, nurdpgetvarfn *getvar) {
    RDPFCTXT c;

    c.txtbuff = txt;
    c.idx = 0;
    c.GetVar = getvar;
    get_tok(&c);
    return expr(&c);
}

// FUNCTION: BIONICLE 0x56ce90
float NuRDPF(char *txt) {
    RDPFCTXT c;

    c.txtbuff = txt;
    c.idx = 0;
    c.GetVar = NULL;
    get_tok(&c);
    return expr(&c);
}
