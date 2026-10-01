#include "nurdp.h"
#include "nustring.h"

#include <stddef.h>

/* Integer expression evaluator behind NuFParGetIntRDP: decimal and $hex numbers, names
 * (through a getvar callback), arithmetic, bitwise, logical and comparison operators,
 * brackets. Based on opensagadev/saga src/nu2api/nucore/nurdpi.cpp (GPL-3.0); names and
 * locals from the PS2 prototype. Not in the PC exe: NuRDPIVar (nothing calls it). */

typedef struct rdpictxt_s {
    int curr_tok;
    int curr_num;
    char *txtbuff;
    int idx;
    nurdpgetvarfn *GetVar;
} RDPICTXT;

enum {
    TOK_ADD = 0,
    TOK_SUB = 1,
    TOK_MUL = 2,
    TOK_DIV = 3,
    TOK_MOD = 4,
    TOK_LBK = 5,
    TOK_RBK = 6,
    TOK_END = 7,
    TOK_NUM = 8,
    TOK_OR = 9,
    TOK_AND = 10,
    TOK_NOT = 11,
    TOK_XOR = 12,
    TOK_LOR = 13,
    TOK_LAND = 14,
    TOK_LNOT = 15,
    TOK_EQ = 16,
    TOK_NEQ = 17,
    TOK_LT = 18,
    TOK_GT = 19,
    TOK_LE = 20,
    TOK_GE = 21,
    TOK_CNT = 22
};

// FUNCTION: BIONICLE 0x56d810
static int ishexnum(char c) {
    switch (c) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        return 1;
    default:
        return 0;
    }
}

// FUNCTION: BIONICLE 0x56d880
static int hexcodetoint(char c) {
    switch (c) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        return c - '0';
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        return c - 'a' + 10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
        return c - 'A' + 10;
    default:
        return 0;
    }
}

static __inline int isnum(char c) {
    if (c >= '0' && c <= '9') {
        return 1;
    }
    return 0;
}

// FUNCTION: BIONICLE 0x56d8f0
static void get_tok(RDPICTXT *c) {
    char varname[256];
    int varidx;

    /* Meant to skip whitespace, but it increments the character rather than the index
     * (as in nurdpf.c). */
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
    case '%':
        c->curr_tok = TOK_MOD;
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
    case '|':
        c->curr_tok = TOK_OR;
        c->idx++;
        if (c->txtbuff[c->idx] == '|') {
            c->curr_tok = TOK_LOR;
            c->idx++;
        }
        return;
    case '&':
        c->curr_tok = TOK_AND;
        c->idx++;
        if (c->txtbuff[c->idx] == '&') {
            c->curr_tok = TOK_LAND;
            c->idx++;
        }
        return;
    case '=':
        c->curr_tok = TOK_EQ;
        c->idx++;
        if (c->txtbuff[c->idx] == '=') {
            c->idx++;
        }
        return;
    case '>':
        c->curr_tok = TOK_GT;
        c->idx++;
        if (c->txtbuff[c->idx] == '=') {
            c->idx++;
            c->curr_tok = TOK_GE;
        }
        return;
    case '<':
        c->curr_tok = TOK_LT;
        c->idx++;
        if (c->txtbuff[c->idx] == '=') {
            c->idx++;
            c->curr_tok = TOK_LE;
        }
        return;
    case '^':
        c->curr_tok = TOK_XOR;
        c->idx++;
        break;
    case '!':
        c->curr_tok = TOK_LNOT;
        c->idx++;
        if (c->txtbuff[c->idx] == '=') {
            c->curr_tok = TOK_NEQ;
            c->idx++;
        }
        return;
    case '~':
        c->curr_tok = TOK_NOT;
        c->idx++;
        break;
    case '$':
        c->idx++;
        c->curr_tok = TOK_NUM;
        c->curr_num = 0;
        while (ishexnum(c->txtbuff[c->idx])) {
            c->curr_num *= 16;
            c->curr_num += hexcodetoint(c->txtbuff[c->idx]);
            c->idx++;
        }
        break;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        c->curr_tok = TOK_NUM;
        c->curr_num = 0;
        while (isnum(c->txtbuff[c->idx])) {
            c->curr_num *= 10;
            c->curr_num += c->txtbuff[c->idx] - '0';
            c->idx++;
        }
        break;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P':
    case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':
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
        c->curr_num = 0;
        (*c->GetVar)(varname, NULL, &c->curr_num);
        break;
    default:
        break;
    }
}

static int expr(RDPICTXT *c);

// FUNCTION: BIONICLE 0x56ddb0
static int prim(RDPICTXT *c) {
    int e;

    switch (c->curr_tok) {
    case TOK_NUM:
        get_tok(c);
        return c->curr_num;
    case TOK_SUB:
        get_tok(c);
        return -prim(c);
    case TOK_NOT:
        get_tok(c);
        return ~prim(c);
    case TOK_LNOT:
        get_tok(c);
        return !prim(c);
    case TOK_LBK:
        get_tok(c);
        e = expr(c);
        get_tok(c);
        return e;
    case TOK_END:
        return 1;
    default:
        return 0;
    }
}

// FUNCTION: BIONICLE 0x56de60
static int term(RDPICTXT *c) {
    int left;

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
        case TOK_MOD:
            get_tok(c);
            left %= prim(c);
            break;
        case TOK_EQ:
            get_tok(c);
            left = left == prim(c);
            break;
        case TOK_NEQ:
            get_tok(c);
            left = left != prim(c);
            break;
        case TOK_LT:
            get_tok(c);
            left = left < prim(c);
            break;
        case TOK_GT:
            get_tok(c);
            left = left > prim(c);
            break;
        case TOK_LE:
            get_tok(c);
            left = left <= prim(c);
            break;
        case TOK_GE:
            get_tok(c);
            left = left >= prim(c);
            break;
        default:
            return left;
        }
    }
}

// FUNCTION: BIONICLE 0x56dfc0
static int expr(RDPICTXT *c) {
    int left;
    int right;

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
        case TOK_AND:
            get_tok(c);
            left &= term(c);
            break;
        case TOK_OR:
            get_tok(c);
            left |= term(c);
            break;
        case TOK_XOR:
            get_tok(c);
            left ^= term(c);
            break;
        case TOK_LAND:
            get_tok(c);
            right = term(c);
            left = left != 0 && right != 0;
            break;
        case TOK_LOR:
            get_tok(c);
            right = term(c);
            left = left != 0 || right != 0;
            break;
        default:
            return left;
        }
    }
}

// FUNCTION: BIONICLE 0x56e0b0
int NuRDPI(char *txt) {
    RDPICTXT c;

    c.txtbuff = txt;
    c.idx = 0;
    c.GetVar = NULL;
    get_tok(&c);
    return expr(&c);
}
