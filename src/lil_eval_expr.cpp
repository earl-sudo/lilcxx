/*
 * LIL - Little Interpreted Language
 * Copyright (C) 2010-2021 Kostas Michalopoulos
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Kostas Michalopoulos <badsector@runtimeterror.com>
 */
/*
 * While I started with LIL I highly modified it
 * Earl Johnson https://github.com/earl-sudo/lilcxx 2022
 */
#include "lil_inter.h"
#include "narrow_cast.h"
#include <cassert>

NS_BEGIN(Lil)

#define ND [[nodiscard]]
#define CAST(X) (X)

void _ee_expr(Lil_exprVal *ee);

ND static inline bool nextCharIs(Lil_exprVal* ee, lchar ch) { return ee->getHeadChar() == ch; }
ND static inline lilint_t getInt(Lil_exprVal* ee) { return ee->getInteger(); }
ND static inline double getDouble(Lil_exprVal* ee) { return ee->getDouble(); }
   static inline void isInt(Lil_exprVal* ee) { ee->setType()    = EE_INT; }
   static inline void isFloat(Lil_exprVal* ee) { ee->setType()    = EE_FLOAT; }
   static inline void setInvalidTypeError(Lil_exprVal* ee) { ee->setError() = EERR_INVALID_TYPE; }
ND static inline bool _ee_validParseState(Lil_exprVal* ee) { return ee->getHead() < ee->getLen() && !ee->getError(); }

ND static bool _ee_invalidpunct(INT ch) { // #private
    return LISPUNCT(ch) && ch != LC('!') && ch != LC('~') && ch != LC('(') && ch != LC(')') && ch != LC('-') && ch != LC('+');
}

static void _ee_skip_spaces(Lil_exprVal *ee) { // #private
    assert(ee!=nullptr);
    while (ee->getHead() < ee->getLen() && LISSPACE(ee->getHeadChar())) { ee->nextHead(); }
}

// Convert text to integer or float value.
static void _ee_numeric_element(Lil_exprVal* ee) { // #private
    assert(ee!=nullptr);
    lilint_t fpart = 0, fpartlen = 1;
    isInt(ee);
    _ee_skip_spaces(ee);
    ee->setInteger() = 0;
    ee->setDouble() = 0;
    while (ee->getHead() < ee->getLen()) {
        if (nextCharIs(ee, LC('.'))) { // Looks like a float literal.
            if (ee->getType() == EE_FLOAT) break;
            isFloat(ee);
            ee->nextHead();
        } else if (!LISDIGIT(ee->getHeadChar())) break;
        if (ee->getType() == EE_INT)
            ee->setInteger() = getInt(ee)*10 + (ee->getHeadChar() - LC('0'));
        else {
            fpart = fpart*10 + (ee->getHeadChar() - LC('0'));
            fpartlen *= 10;
        }
        ee->nextHead();
    }
    if (ee->getType() == EE_FLOAT) {
        ee->setDouble() = getInt(ee) + CAST(double) fpart / CAST(double) fpartlen;
    }
}

static void _ee_element(Lil_exprVal *ee) { // #private
    assert(ee!=nullptr);
    if (LISDIGIT(ee->getHeadChar())) {
        _ee_numeric_element(ee);
        return;
    }

    /* for anything else that might creep in (usually from strings), we set the
     * value to 1 so that strings evaluate as "true" when used in conditional
     * expressions */
    isInt(ee);
    ee->setInteger() = 1;
    ee->setError()   = EERR_INVALID_EXPRESSION; /* special flag, will be cleared */
}

// Handle a "(...)" like structure.
static void _ee_paren(Lil_exprVal *ee) { // #private
    assert(ee!=nullptr);
        _ee_skip_spaces(ee);
    if (nextCharIs(ee, LC('('))) {
        ee->nextHead();
        _ee_expr(ee);
        _ee_skip_spaces(ee);
        if (nextCharIs(ee, LC(')'))) { ee->nextHead(); }
        else { ee->setError() = EERR_SYNTAX_ERROR; }
    } else { _ee_element(ee); }
}

// Handle unary math operations. (i.e. '-','+','~','!'...)
static void _ee_unaryOperators(Lil_exprVal* ee) { // #private
    assert(ee!=nullptr);
        _ee_skip_spaces(ee);
    if (_ee_validParseState(ee) &&
        (nextCharIs(ee, LC('-')) ||
        nextCharIs(ee, LC('+')) ||
        nextCharIs(ee, LC('~')) ||
        nextCharIs(ee, LC('!')))) {
        lchar op = ee->getHeadChar(); ee->nextHead();
        _ee_unaryOperators(ee);
        if (ee->getError()) return;
        switch (op) {
            case LC('-'):
                switch (ee->getType()) {
                    case EE_FLOAT: ee->setDouble() = -getDouble(ee); break;
                    case EE_INT:   ee->setInteger() = -getInt(ee); break;
                    default: setInvalidTypeError(ee); break; // #ERR_RET
                }
                break;
            case LC('+'):
                /* ignore it, doesn't change a thing */
                break;
            case LC('~'):
                switch (ee->getType()) {
                    case EE_FLOAT: ee->setInteger() = ~(CAST(lilint_t)getDouble(ee)); isInt(ee); break;
                    case EE_INT:   ee->setInteger() = ~getInt(ee); break;
                    default: setInvalidTypeError(ee); break; // #ERR_RET
                }
                break;
            case LC('!'):
                switch (ee->getType()) {
                    case EE_FLOAT:  ee->setDouble() = !getDouble(ee); break;
                    case EE_INT:    ee->setInteger() = !getInt(ee); break;
                    default: setInvalidTypeError(ee); break; // #ERR_RET
                }
                break;
            default: assert(false);
        }
    } else {
        _ee_paren(ee);
    }
}

// Handle multiple and divide math operations.
static void _ee_mutlipleDivide(Lil_exprVal* ee) { // #private
    assert(ee!=nullptr);
        _ee_unaryOperators(ee);
    if (ee->getError()) return;
        _ee_skip_spaces(ee);
    while (_ee_validParseState(ee) && !_ee_invalidpunct(ee->getHeadChar(1)) &&
               (nextCharIs(ee, LC('*'))  ||
                nextCharIs(ee, LC('/'))  ||
                nextCharIs(ee, LC('\\')) ||
                nextCharIs(ee, LC('%')))) {
        double odval = getDouble(ee);
        lilint_t oival = getInt(ee);

        switch (ee->getHeadChar()) {
            case LC('*'):
                switch (ee->getType()) {
                    case EE_FLOAT:
                        ee->nextHead();
                        _ee_unaryOperators(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setDouble() = getDouble(ee)*odval; break;
                            case EE_INT:   ee->setDouble() = getInt(ee)*odval; isFloat(ee); break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    case EE_INT:
                        ee->nextHead();
                        _ee_unaryOperators(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT:  ee->setDouble() = getDouble(ee)*oival; isFloat(ee); break;
                            case EE_INT:    ee->setInteger() = getInt(ee)*oival; break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    default: setInvalidTypeError(ee); break; // #ERR_RET
                }
                break;
            case LC('%'):
                switch (ee->getType()) {
                    case EE_FLOAT:
                        ee->nextHead();
                        _ee_unaryOperators(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT:
                                if (getDouble(ee) == 0.0) { ee->setError() = EERR_DIVISION_BY_ZERO;
                                } else {                      ee->setDouble() = fmod(odval, getDouble(ee)); }
                                break;
                            case EE_INT:
                                if (getInt(ee) == 0) { ee->setError() = EERR_DIVISION_BY_ZERO;
                                } else {                     ee->setDouble() = fmod(odval, getInt(ee)); }
                                isFloat(ee);
                                break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    case EE_INT:
                        ee->nextHead();
                        _ee_unaryOperators(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT:
                                if (getDouble(ee) == 0.0) { ee->setError() = EERR_DIVISION_BY_ZERO;
                                } else {                      ee->setDouble() = fmod(oival, getDouble(ee)); }
                                break;
                            case EE_INT:
                                if (getInt(ee) == 0) {  ee->setError() = EERR_DIVISION_BY_ZERO;
                                } else {                      ee->setInteger() = oival%getInt(ee); }
                                break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    default:
                        assert(false);
                }
                break;
            case LC('/'):
                switch (ee->getType()) {
                    case EE_FLOAT:
                        ee->nextHead();
                        _ee_unaryOperators(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT:
                                if (getDouble(ee) == 0.0) { ee->setError() = EERR_DIVISION_BY_ZERO;
                                } else {                      ee->setDouble() = odval/getDouble(ee); }
                                break;
                            case EE_INT:
                                if (getInt(ee) == 0) {  ee->setError() = EERR_DIVISION_BY_ZERO;
                                } else {                      ee->setDouble() = odval/CAST(double)getInt(ee); }
                                isFloat(ee);
                                break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    case EE_INT:
                        ee->nextHead();
                        _ee_unaryOperators(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT:
                                if (getDouble(ee) == 0.0) { ee->setError() = EERR_DIVISION_BY_ZERO;
                                } else {                      ee->setDouble() = CAST(double)oival/getDouble(ee); }
                                break;
                            case EE_INT:
                                if (getInt(ee) == 0) {  ee->setError() = EERR_DIVISION_BY_ZERO;
                                } else {                      ee->setDouble() = CAST(double)oival/CAST(double)getInt(ee); }
                                isFloat(ee);
                                break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    default:
                        assert(false);
                }
                break;
            case LC('\\'):
                switch (ee->getType()) {
                    case EE_FLOAT:
                        ee->nextHead();
                        _ee_unaryOperators(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT:
                                if (getDouble(ee) == 0.0) { ee->setError() = EERR_DIVISION_BY_ZERO;
                                } else {                      ee->setInteger() = CAST(lilint_t)(odval/getDouble(ee));  }
                                isInt(ee);
                                break;
                            case EE_INT:
                                if (getInt(ee) == 0) { ee->setError() = EERR_DIVISION_BY_ZERO;
                                } else {                     ee->setInteger() = CAST(lilint_t)(odval/CAST(double)getInt(ee)); }
                                break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    case EE_INT:
                        ee->nextHead();
                        _ee_unaryOperators(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT:
                                if (getDouble(ee) == 0.0) { ee->setError() = EERR_DIVISION_BY_ZERO;
                                } else {                      ee->setInteger() = CAST(lilint_t)(CAST(double)oival/getDouble(ee)); }
                                isInt(ee);
                                break;
                            case EE_INT:
                                if (getInt(ee) == 0) { ee->setError() = EERR_DIVISION_BY_ZERO;
                                } else {                     ee->setInteger() = oival/getInt(ee); }
                                break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    default: setInvalidTypeError(ee); break; // #ERR_RET
                }
                break;
        }

        _ee_skip_spaces(ee);
    }
}

// Handle add/subtract math operations.
static void _ee_addsub(Lil_exprVal* ee) { // #private
    assert(ee!=nullptr);
        _ee_mutlipleDivide(ee);
        _ee_skip_spaces(ee);
    while (_ee_validParseState(ee) && !_ee_invalidpunct(ee->getHeadChar(1)) &&
           (nextCharIs(ee, LC('+')) ||
            nextCharIs(ee, LC('-')))) {
        double odval = getDouble(ee);
        lilint_t oival = getInt(ee);

        switch (ee->getHeadChar()) {
            case LC('+'):
                switch (ee->getType()) {
                    case EE_FLOAT:
                        ee->nextHead();
                        _ee_mutlipleDivide(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setDouble() = getDouble(ee)+odval; break;
                            case EE_INT:   ee->setDouble() = getInt(ee)+odval; isFloat(ee); break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    case EE_INT:
                        ee->nextHead();
                        _ee_mutlipleDivide(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setDouble() = getDouble(ee)+oival; isFloat(ee); break;
                            case EE_INT:   ee->setInteger() = getInt(ee)+oival; break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    default:
                        setInvalidTypeError(ee);
                        break;
                }
                break;
            case LC('-'):
                switch (ee->getType()) {
                    case EE_FLOAT:
                        ee->nextHead();
                        _ee_mutlipleDivide(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setDouble() = odval-getDouble(ee); break;
                            case EE_INT:   ee->setDouble() = odval-getInt(ee); isFloat(ee); break;
                            default: setInvalidTypeError(ee); break;
                        }
                        break;
                    case EE_INT:
                        ee->nextHead();
                        _ee_mutlipleDivide(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setDouble() = CAST(double)oival-getDouble(ee); isFloat(ee); break;
                            case EE_INT:   ee->setInteger() = oival-getInt(ee); break;
                            default: setInvalidTypeError(ee); break;
                        }
                        break;
                    default: setInvalidTypeError(ee); break; // #ERR_RET
                }
                break;
            default:
                assert(false);
        }

        _ee_skip_spaces(ee);
    }
}

// Handle left/right computer math shift operations.
static void _ee_shift(Lil_exprVal* ee) { // #private
    assert(ee!=nullptr);
        _ee_addsub(ee);
        _ee_skip_spaces(ee);
    while (_ee_validParseState(ee) &&
           ((nextCharIs(ee, LC('<')) && ee->getHeadChar(1) == LC('<')) ||
            (nextCharIs(ee, LC('>')) && ee->getHeadChar(1) == LC('>')))) {
        double odval = getDouble(ee);
        lilint_t oival = getInt(ee);
        ee->nextHead();

        switch (ee->getHeadChar()) {
            case LC('<'):
                switch (ee->getType()) {
                    case EE_FLOAT:
                        ee->nextHead();
                        _ee_addsub(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setInteger() = CAST(lilint_t)odval << CAST(lilint_t)getDouble(ee); isInt(ee); break;
                            case EE_INT:   ee->setInteger() = CAST(lilint_t)odval << getInt(ee); break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    case EE_INT:
                        ee->nextHead();
                        _ee_addsub(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setInteger() = oival << CAST(lilint_t)getDouble(ee); isInt(ee); break;
                            case EE_INT:   ee->setInteger() = oival << getInt(ee); break;
                            default: setInvalidTypeError(ee); break;
                        }
                        break;
                    default:
                        setInvalidTypeError(ee);
                        break;
                }
                break;
            case LC('>'):
                switch (ee->getType()) {
                    case EE_FLOAT:
                        ee->nextHead();
                        _ee_addsub(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setInteger() = CAST(lilint_t)odval >> CAST(lilint_t)getDouble(ee); isInt(ee); break;
                            case EE_INT:   ee->setInteger() = CAST(lilint_t)odval >> getInt(ee); break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    case EE_INT:
                        ee->nextHead();
                        _ee_addsub(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setInteger() = oival >> CAST(lilint_t)getDouble(ee); isInt(ee); break;
                            case EE_INT:   ee->setInteger() = oival >> getInt(ee); break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    default: setInvalidTypeError(ee); break; // #ERR_RET
                }
                break;
            default: assert(false);
        }

        _ee_skip_spaces(ee);
    }
}

// Handle math comparisons.
static void _ee_compare(Lil_exprVal* ee) {// #private
    assert(ee!=nullptr);
        _ee_shift(ee);
        _ee_skip_spaces(ee);
    while (_ee_validParseState(ee) &&
           ((nextCharIs(ee, LC('<')) && !_ee_invalidpunct(ee->getHeadChar(1))) ||
            (nextCharIs(ee, LC('>')) && !_ee_invalidpunct(ee->getHeadChar(1))) ||
            (nextCharIs(ee, LC('<')) && ee->getHeadChar(1) == LC('=')) ||
            (nextCharIs(ee, LC('>')) && ee->getHeadChar(1) == LC('=')))) {
        double odval = getDouble(ee);
        lilint_t oival = getInt(ee);
        INT op = 4;
        if (nextCharIs(ee, LC('<')) && !_ee_invalidpunct(ee->getHeadChar(1))) op = 1;
        else if (nextCharIs(ee, LC('>')) && !_ee_invalidpunct(ee->getHeadChar(1))) op = 2;
        else if (nextCharIs(ee, LC('<')) && ee->getHeadChar(1) == LC('=')) op = 3;

        ee->setHead() += op > 2 ? 2 : 1;

        switch (op) {
            case 1:
                switch (ee->getType()) {
                    case EE_FLOAT:
                        _ee_shift(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setInteger() = (odval < getDouble(ee))?1:0; isInt(ee);  break;
                            case EE_INT:   ee->setInteger() = (odval < getInt(ee))?1:0; break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    case EE_INT:
                        _ee_shift(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setInteger() = (oival < getDouble(ee))?1:0; isInt(ee); break;
                            case EE_INT:   ee->setInteger() = (oival < getInt(ee))?1:0; break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    default:
                        setInvalidTypeError(ee);
                        break;
                }
                break;
            case 2:
                switch (ee->getType()) {
                    case EE_FLOAT:
                        _ee_shift(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setInteger() = (odval > getDouble(ee))?1:0; isInt(ee); break;
                            case EE_INT:   ee->setInteger() = (odval > getInt(ee))?1:0; break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    case EE_INT:
                        _ee_shift(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setInteger() = (oival > getDouble(ee))?1:0; isInt(ee); break;
                            case EE_INT:   ee->setInteger() = (oival > getInt(ee))?1:0; break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    default: setInvalidTypeError(ee); break; // #ERR_RET
                }
                break;
            case 3:
                switch (ee->getType()) {
                    case EE_FLOAT:
                        _ee_shift(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setInteger() = (odval <= getDouble(ee))?1:0; isInt(ee); break;
                            case EE_INT:   ee->setInteger() = (odval <= getInt(ee))?1:0; break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    case EE_INT:
                        _ee_shift(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setInteger() = (oival <= getDouble(ee))?1:0; isInt(ee); break;
                            case EE_INT:   ee->setInteger() = (oival <= getInt(ee))?1:0; break;
                            default: setInvalidTypeError(ee); break;
                        }
                        break;
                    default: setInvalidTypeError(ee); break; // #ERR_RET
                }
                break;
            case 4:
                switch (ee->getType()) {
                    case EE_FLOAT:
                        _ee_shift(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setInteger() = (odval >= getDouble(ee))?1:0; isInt(ee); break;
                            case EE_INT:   ee->setInteger() = (odval >= getInt(ee))?1:0; break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    case EE_INT:
                        _ee_shift(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setInteger() = (oival >= getDouble(ee))?1:0; isInt(ee); break;
                            case EE_INT:   ee->setInteger() = (oival >= getInt(ee))?1:0; break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    default: setInvalidTypeError(ee);  break; // #ERR_RET
                }
                break;
            default: assert(false);
        }

        _ee_skip_spaces(ee);
    }
}

// Handles math =/!=.
static void _ee_equals(Lil_exprVal* ee) { // #private
    assert(ee!=nullptr);
        _ee_compare(ee);
        _ee_skip_spaces(ee);
    while (_ee_validParseState(ee)  &&
           ((nextCharIs(ee, LC('=')) && ee->getHeadChar(1) == LC('=')) ||
            (nextCharIs(ee, LC('!')) && ee->getHeadChar(1) == LC('=')))) {
        double odval = getDouble(ee);
        lilint_t oival = getInt(ee);
        INT op = nextCharIs(ee, LC('=')) ? 1 : 2;
        ee->setHead() += 2;

        switch (op) {
            case 1:
                switch (ee->getType()) {
                    case EE_FLOAT:
                        _ee_compare(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setInteger() = (odval == getDouble(ee))?1:0; isInt(ee); break;
                            case EE_INT:   ee->setInteger() = (odval == getInt(ee))?1:0; break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    case EE_INT:
                        _ee_compare(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setInteger() = (oival == getDouble(ee))?1:0; isInt(ee); break;
                            case EE_INT:   ee->setInteger() = (oival == getInt(ee))?1:0; break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    default: setInvalidTypeError(ee); break; // #ERR_RET
                } // switch (ee->getType())
                break;
            case 2:
                switch (ee->getType()) {
                    case EE_FLOAT:
                        _ee_compare(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setInteger() = (odval != getDouble(ee))?1:0; isInt(ee); break;
                            case EE_INT:   ee->setInteger() = (odval != getInt(ee))?1:0; break;
                            default: setInvalidTypeError(ee); break;
                        }
                        break;
                    case EE_INT:
                        _ee_compare(ee);
                        if (ee->getError()) return;
                        switch (ee->getType()) {
                            case EE_FLOAT: ee->setInteger() = (oival != getDouble(ee))?1:0; isInt(ee); break;
                            case EE_INT:   ee->setInteger() = (oival != getInt(ee))?1:0; break;
                            default: setInvalidTypeError(ee); break; // #ERR_RET
                        }
                        break;
                    default: setInvalidTypeError(ee); break; // #ERR_RET
                } // switch (ee->getType())
                break;
            default: assert(false);
        } // switch (op)

        _ee_skip_spaces(ee);
    } // while (...)
}

// Handles math bitand.
static void _ee_bitwiseAnd(Lil_exprVal* ee) {// #private
    assert(ee!=nullptr);
        _ee_equals(ee);
        _ee_skip_spaces(ee);
    while (_ee_validParseState(ee) &&
           (nextCharIs(ee, LC('&')) && !_ee_invalidpunct(ee->getHeadChar(1)))) {
        double odval = getDouble(ee);
        lilint_t oival = getInt(ee);
        ee->nextHead();

        switch (ee->getType()) {
            case EE_FLOAT:
                _ee_equals(ee);
                if (ee->getError()) return;
                switch (ee->getType()) {
                    case EE_FLOAT: ee->setInteger() = CAST(lilint_t)odval & CAST(lilint_t)getDouble(ee); isInt(ee); break;
                    case EE_INT:   ee->setInteger() = CAST(lilint_t)odval & getInt(ee); break;
                    default: setInvalidTypeError(ee); break; // #ERR_RET
                } // switch (ee->getType())
                break;
            case EE_INT:
                _ee_equals(ee);
                if (ee->getError()) return;
                switch (ee->getType()) {
                    case EE_FLOAT: ee->setInteger() = oival & CAST(lilint_t)getDouble(ee); isInt(ee);    break;
                    case EE_INT:   ee->setInteger() = oival & getInt(ee); break;
                    default: setInvalidTypeError(ee); break; // #ERR_RET
                } // switch (ee->getType())
                break;
            default: setInvalidTypeError(ee); break; // #ERR_RET
        } // switch (ee->getType())

        _ee_skip_spaces(ee);
    } // while(...)
}

// Handle math bitor.
static void _ee_bitwiseOr(Lil_exprVal* ee) { // #private
    assert(ee!=nullptr);
        _ee_bitwiseAnd(ee);
        _ee_skip_spaces(ee);
    while (_ee_validParseState(ee) &&
           (nextCharIs(ee, LC('|')) && !_ee_invalidpunct(ee->getHeadChar(1)))) {
        double odval = getDouble(ee);
        lilint_t oival = getInt(ee);
        ee->nextHead();

        switch (ee->getType()) {
            case EE_FLOAT:
                _ee_bitwiseAnd(ee);
                if (ee->getError()) return;
                switch (ee->getType()) {
                    case EE_FLOAT: ee->setInteger() = CAST(lilint_t)odval | CAST(lilint_t)getDouble(ee); isInt(ee); break;
                    case EE_INT:   ee->setInteger() = CAST(lilint_t)odval | getInt(ee); break;
                    default: setInvalidTypeError(ee); break; // #ERR_RET
                } // switch (ee->getType())
                break;
            case EE_INT:
                _ee_bitwiseAnd(ee);
                if (ee->getError()) return;
                switch (ee->getType()) {
                    case EE_FLOAT: ee->setInteger() = oival | CAST(lilint_t)getDouble(ee); isInt(ee); break;
                    case EE_INT:   ee->setInteger() = oival | getInt(ee); break;
                    default: setInvalidTypeError(ee); break; // #ERR_RET
                } // switch (ee->getType())
                break;
            default: setInvalidTypeError(ee); break; // #ERR_RET
        } // switch (ee->getType())

        _ee_skip_spaces(ee);
    } // while(...)
}

// Handle math logical and.
static void _ee_logicalAnd(Lil_exprVal* ee) {// #private
    assert(ee!=nullptr);
        _ee_bitwiseOr(ee);
        _ee_skip_spaces(ee);
    while (_ee_validParseState(ee) &&
           (nextCharIs(ee, LC('&')) && ee->getHeadChar(1) == LC('&'))) {
        double odval = getDouble(ee);
        lilint_t oival = getInt(ee);
        ee->setHead() += 2;

        switch (ee->getType()) {
            case EE_FLOAT:
                _ee_bitwiseOr(ee);
                if (ee->getError()) return;
                switch (ee->getType()) {
                    case EE_FLOAT: ee->setInteger() = (odval && getDouble(ee))?1:0; isInt(ee); break;
                    case EE_INT:   ee->setInteger() = (odval && getInt(ee))?1:0; break;
                    default: setInvalidTypeError(ee); break; // #ERR_RET
                } // switch (ee->getType())
                break;
            case EE_INT:
                _ee_bitwiseOr(ee);
                if (ee->getError()) return;
                switch (ee->getType()) {
                    case EE_FLOAT: ee->setInteger() = (oival && getDouble(ee))?1:0; isInt(ee); break;
                    case EE_INT:   ee->setInteger() = (oival && getInt(ee))?1:0; break;
                    default: setInvalidTypeError(ee); break; // #ERR_RET
                } // switch (ee->getType())
                break;
            default: setInvalidTypeError(ee); break; // #ERR_RET
        } // switch (ee->getType())

        _ee_skip_spaces(ee);
    } // while(...)
}

// Handle logical or.
static void _ee_logicalOr(Lil_exprVal* ee) {// #private
    assert(ee!=nullptr);
        _ee_logicalAnd(ee);
        _ee_skip_spaces(ee);
    while (_ee_validParseState(ee) &&
           (nextCharIs(ee, LC('|')) && ee->getHeadChar(1) == LC('|'))) {
        double odval = getDouble(ee);
        lilint_t oival = getInt(ee);
        ee->setHead() += 2;

        switch (ee->getType()) {
            case EE_FLOAT:
                _ee_logicalAnd(ee);
                if (ee->getError()) return;
                switch (ee->getType()) {
                    case EE_FLOAT: ee->setInteger() = (odval || getDouble(ee))?1:0; isInt(ee); break;
                    case EE_INT:   ee->setInteger() = (odval || getInt(ee))?1:0; break;
                    default: setInvalidTypeError(ee); break; // #ERR_RET
                } // switch (ee->getType())
                break;
            case EE_INT:
                _ee_logicalAnd(ee);
                if (ee->getError()) return;
                switch (ee->getType()) {
                    case EE_FLOAT: ee->setInteger() = (oival || getDouble(ee))?1:0; isInt(ee); break;
                    case EE_INT:   ee->setInteger() = (oival || getInt(ee))?1:0; break;
                    default: setInvalidTypeError(ee); break; // #ERR_RET
                } // switch (ee->getType())
                break;
            default: setInvalidTypeError(ee); break; // #ERR_RET
        } // switch (ee->getType())

        _ee_skip_spaces(ee);
    } // while(...)
}

void _ee_expr(Lil_exprVal *ee) { // #private
    assert(ee!=nullptr);
    _ee_logicalOr(ee);
    /* invalid expression doesn't really matter, it is only used to stop
     * the expression parsing. */
    if (ee->getError() == EERR_INVALID_EXPRESSION) {
        ee->setError()   = EERR_NO_ERROR;
        ee->setInteger() = 1;
    }
}

#undef ND

NS_END(Lil)
