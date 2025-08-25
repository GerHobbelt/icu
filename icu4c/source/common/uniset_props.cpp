// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
*******************************************************************************
*
*   Copyright (C) 1999-2014, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  uniset_props.cpp
*   encoding:   UTF-8
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2004aug25
*   created by: Markus W. Scherer
*
*   Character property dependent functions moved here from uniset.cpp
*/

#include "unicode/utypes.h"
#include "unicode/uniset.h"
#include "unicode/parsepos.h"
#include "unicode/uchar.h"
#include "unicode/uscript.h"
#include "unicode/symtable.h"
#include "unicode/uset.h"
#include "unicode/locid.h"
#include "unicode/brkiter.h"
#include "uset_imp.h"
#include "ruleiter.h"
#include "cmemory.h"
#include "ucln_cmn.h"
#include "util.h"
#include "uvector.h"
#include "uprops.h"
#include "propname.h"
#include "normalizer2impl.h"
#include "uinvchar.h"
#include "uprops.h"
#include "charstr.h"
#include "cstring.h"
#include "mutex.h"
#include "umutex.h"
#include "uassert.h"
#include "hash.h"
#include <optional>

U_NAMESPACE_USE

namespace {

// Special property set IDs
constexpr char ANY[]   = "ANY";   // [\u0000-\U0010FFFF]
constexpr char ASCII[] = "ASCII"; // [\u0000-\u007F]
constexpr char ASSIGNED[] = "Assigned"; // [:^Cn:]

// Unicode name property alias
constexpr char16_t NAME_PROP[] = u"na";

}  // namespace

// Cached sets ------------------------------------------------------------- ***

U_CDECL_BEGIN
static UBool U_CALLCONV uset_cleanup();

static UnicodeSet *uni32Singleton;
static icu::UInitOnce uni32InitOnce {};

/**
 * Cleanup function for UnicodeSet
 */
static UBool U_CALLCONV uset_cleanup() {
    delete uni32Singleton;
    uni32Singleton = nullptr;
    uni32InitOnce.reset();
    return true;
}

U_CDECL_END

U_NAMESPACE_BEGIN

namespace {

// Cache some sets for other services -------------------------------------- ***
void U_CALLCONV createUni32Set(UErrorCode &errorCode) {
    U_ASSERT(uni32Singleton == nullptr);
    uni32Singleton = new UnicodeSet(UnicodeString(u"[:age=3.2:]"), errorCode);
    if(uni32Singleton==nullptr) {
        errorCode=U_MEMORY_ALLOCATION_ERROR;
    } else {
        uni32Singleton->freeze();
    }
    ucln_common_registerCleanup(UCLN_COMMON_USET, uset_cleanup);
}


U_CFUNC UnicodeSet *
uniset_getUnicode32Instance(UErrorCode &errorCode) {
    umtx_initOnce(uni32InitOnce, &createUni32Set, errorCode);
    return uni32Singleton;
}

// helper functions for matching of pattern syntax pieces ------------------ ***
// these functions are parallel to the PERL_OPEN etc. strings above

// using these functions is not only faster than UnicodeString::compare() and
// caseCompare(), but they also make UnicodeSet work for simple patterns when
// no Unicode properties data is available - when caseCompare() fails

inline UBool
isPerlOpen(const UnicodeString &pattern, int32_t pos) {
    char16_t c;
    return pattern.charAt(pos)==u'\\' && ((c=pattern.charAt(pos+1))==u'p' || c==u'P');
}

/*static inline UBool
isPerlClose(const UnicodeString &pattern, int32_t pos) {
    return pattern.charAt(pos)==u'}';
}*/

inline UBool
isNameOpen(const UnicodeString &pattern, int32_t pos) {
    return pattern.charAt(pos)==u'\\' && pattern.charAt(pos+1)==u'N';
}

inline UBool
isPOSIXOpen(const UnicodeString &pattern, int32_t pos) {
    return pattern.charAt(pos)==u'[' && pattern.charAt(pos+1)==u':';
}

/*static inline UBool
isPOSIXClose(const UnicodeString &pattern, int32_t pos) {
    return pattern.charAt(pos)==u':' && pattern.charAt(pos+1)==u']';
}*/

// TODO memory debugging provided inside uniset.cpp
// could be made available here but probably obsolete with use of modern
// memory leak checker tools
#define _dbgct(me)

}  // namespace

//----------------------------------------------------------------
// Constructors &c
//----------------------------------------------------------------

/**
 * Constructs a set from the given pattern, optionally ignoring
 * white space.  See the class description for the syntax of the
 * pattern language.
 * @param pattern a string specifying what characters are in the set
 */
UnicodeSet::UnicodeSet(const UnicodeString& pattern,
                       UErrorCode& status) {
    applyPattern(pattern, status);
    _dbgct(this);
}

//----------------------------------------------------------------
// Public API
//----------------------------------------------------------------

UnicodeSet& UnicodeSet::applyPattern(const UnicodeString& pattern,
                                     UErrorCode& status) {
    // Equivalent to
    //   return applyPattern(pattern, USET_IGNORE_SPACE, nullptr, status);
    // but without dependency on closeOver().
    ParsePosition pos(0);
    applyPatternIgnoreSpace(pattern, pos, nullptr, status);
    if (U_FAILURE(status)) return *this;

    int32_t i = pos.getIndex();
    // Skip over trailing whitespace
    ICU_Utility::skipWhitespace(pattern, i, true);
    if (i != pattern.length()) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
    }
    return *this;
}

void
UnicodeSet::applyPatternIgnoreSpace(const UnicodeString& pattern,
                                    ParsePosition& pos,
                                    const SymbolTable* symbols,
                                    UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }
    if (isFrozen()) {
        status = U_NO_WRITE_PERMISSION;
        return;
    }
    // Need to build the pattern in a temporary string because
    // _applyPattern calls add() etc., which set pat to empty.
    UnicodeString rebuiltPat;
    RuleCharacterIterator chars(pattern, symbols, pos);
    applyPattern(pattern, pos, chars, symbols, rebuiltPat, USET_IGNORE_SPACE, nullptr, status);
    if (U_FAILURE(status)) return;
    if (chars.inVariable()) {
        // syntaxError(chars, "Extra chars in variable value");
        status = U_MALFORMED_SET;
        return;
    }
    setPattern(rebuiltPat);
}

/**
 * Return true if the given position, in the given pattern, appears
 * to be the start of a UnicodeSet pattern.
 */
UBool UnicodeSet::resemblesPattern(const UnicodeString& pattern, int32_t pos) {
    return ((pos+1) < pattern.length() &&
            pattern.charAt(pos) == static_cast<char16_t>(91)/*[*/) ||
        resemblesPropertyPattern(pattern, pos);
}

//----------------------------------------------------------------
// Implementation: Pattern parsing
//----------------------------------------------------------------

class UnicodeSet::Lexer {
  public:
    Lexer(const UnicodeString &pattern,
          const ParsePosition &parsePosition,
          RuleCharacterIterator &chars,
          uint32_t unicodeSetOptions,
          const SymbolTable *const symbols)
        : pattern_(pattern), parsePosition_(parsePosition), chars_(chars),
          charsOptions_(RuleCharacterIterator::PARSE_VARIABLES | RuleCharacterIterator::PARSE_ESCAPES |
                        ((unicodeSetOptions & USET_IGNORE_SPACE) != 0
                             ? RuleCharacterIterator::SKIP_WHITESPACE
                             : 0)),
          symbols_(symbols) {}

    class Lookahead {
      public:
        bool isUnescaped(UChar32 codePoint) const {
            return !escaped_ && codePoint_ == codePoint;
        }

        bool isUnescapedNotStandIn(UChar32 codePoint) {
            return isUnescaped(codePoint) && standIn() == nullptr;
        }

        void moveAfter() {
            lexer_.chars_.setPos(after_);
            lexer_.ahead_.reset();
        }

        bool acceptUnescapedNotStandIn(UChar32 codePoint) {
            if (isUnescapedNotStandIn(codePoint)) {
                moveAfter();
                return true;
            }
            return false;
        }

        bool acceptUnescaped(UChar32 codePoint) {
            if (isUnescaped(codePoint)) {
                moveAfter();
                return true;
            }
            return false;
        }

        UChar32 codePoint(UErrorCode &errorCode) const {
            if (!U_FAILURE(errorCode)) {
                errorCode = errorCode;
            }
            return codePoint_;
        }

        bool escaped() const {
            return escaped_;
        }

        const UnicodeSet *standIn() {
            if (!standIn_.has_value()) {
                if (lexer_.symbols_ == nullptr) {
                    standIn_ = nullptr;
                } else {
                    standIn_ =
                        dynamic_cast<const UnicodeSet *>(lexer_.symbols_->lookupMatcher(codePoint_));
                }
            }
            return *standIn_;
        };

        // Some parts of the grammar need two tokens of lookahead.  The second lookahead is not cached.
        Lookahead oneMore() {
            return oneMore(lexer_.charsOptions_);
        }

        Lookahead oneMore(int32_t charsOptions) {
            RuleCharacterIterator::Pos before;
            lexer_.chars_.getPos(before);
            lexer_.chars_.setPos(after_);
            auto const result = Lookahead(lexer_, lexer_.chars_, charsOptions);
            lexer_.chars_.setPos(before);
            return result;
        }

        Lookahead(Lexer &lexer, RuleCharacterIterator &chars, int32_t charsOptions)
            : lexer_(lexer) {
            RuleCharacterIterator::Pos before;
            chars.getPos(before);
            codePoint_ = chars.next(charsOptions, escaped_, errorCode_);
            chars.getPos(after_);
            chars.setPos(before);
        }

      private:
        Lexer &lexer_;
        RuleCharacterIterator::Pos after_;
        UErrorCode errorCode_;
        UChar32 codePoint_;
        UBool escaped_;
        // `std::nullopt` if we have not yet called `lookupMatcher`, otherwise the result of
        // `lookupMatcher` (which may be `nullptr`).
        std::optional<const UnicodeSet *> standIn_;

        friend class Lexer;
    };

    UnicodeString getPositionForDebugging() const {
        return pattern_.tempSubString(0, parsePosition_.getIndex()) + u"☞" +
               pattern_.tempSubString(parsePosition_.getIndex(), 60);
    }

    Lookahead &lookahead() {
        if (!ahead_.has_value()) {
            ahead_.emplace(*this, chars_, charsOptions_);
        }
        return *ahead_;
    }

    bool resemblesPropertyPattern() {
        Lookahead first =
            Lookahead(*this, chars_, charsOptions_ & ~RuleCharacterIterator::PARSE_ESCAPES);
        if (first.codePoint_ != u'[' && first.codePoint_ != u'\\') {
            return false;
        }
        Lookahead second = first.oneMore(charsOptions_ & ~(RuleCharacterIterator::PARSE_ESCAPES |
                                                           RuleCharacterIterator::SKIP_WHITESPACE));
        return (first.codePoint_ == u'[' && second.codePoint_ == ':') ||
               (first.codePoint_ == u'\\' &&
                (second.codePoint_ == u'p' || second.codePoint_ == u'P' || second.codePoint_ == u'N'));
    }

    // For use in functions that take the `RuleCharacterIterator` directly; clears the lookahead cache so
    // that any advancement of the `RuleCharacterIterator` is taken into account by subsequent calls to
    // `lookahead`.  The resulting `RuleCharacterIterator` must not be used once `lookahead` has been
    // called.
    RuleCharacterIterator &getCharacterIterator() {
        ahead_.reset();
        return chars_;
    }

    int32_t charsOptions() {
        return charsOptions_;
    }

    bool atEnd() const {
        return chars_.atEnd();
    }

  private:
    const UnicodeString &pattern_;
    const ParsePosition &parsePosition_;
    RuleCharacterIterator &chars_;
    const int32_t charsOptions_;
    const SymbolTable *const symbols_;
    std::optional<Lookahead> ahead_;
};

namespace {

constexpr int32_t MAX_DEPTH = 100;

#define U_DEBUGGING_UNICODESET_PARSING 0

#if U_DEBUGGING_UNICODESET_PARSING

#define U_UNICODESET_RETURN_IF_ERROR(ec)                                                                \
    do {                                                                                                \
    constexpr std::string_view functionName = __func__;\
    static_assert (functionName.substr(0, 5) == "parse");\
        if (U_FAILURE(ec)) {                                                                            \
            if (depth < 5) {                                                                            \
                printf("--- in %s l. %d\n", __func__+5, __LINE__);                                        \
            } else if (depth == 5 && std::string_view(__func__+5) == "UnicodeSet") {                 \
                printf("--- [...]\n");                                                                  \
            }                                                                                           \
            return;                                                                                     \
        }                                                                                               \
    } while (false)
#define U_UNICODESET_RETURN_WITH_PARSE_ERROR(expected, actual, lexer, ec)                               \
    do {                                                                                                \
        constexpr std::string_view functionName = __func__;                                             \
        static_assert(functionName.substr(0, 5) == "parse");                                            \
        std::string actualUTF8;                                                                         \
        std::string contextUTF8;                                                                        \
        printf("*** Expected %s, got '%s' %s\n", (expected),                                            \
               UnicodeString(actual).toUTF8String(actualUTF8).c_str(),                                  \
               lexer.getPositionForDebugging().toUTF8String(contextUTF8).c_str());                      \
        printf("--- in %s l. %d\n", __func__ + 5, __LINE__);                                            \
        (ec) = U_MALFORMED_SET;                                                                         \
        return;                                                                                         \
    } while (false)

#else

#define U_UNICODESET_RETURN_IF_ERROR(ec)                                                                \
    do {                                                                                                \
        if (U_FAILURE(ec)) {                                                                            \
            return;                                                                                     \
        }                                                                                               \
    } while (false)
#define U_UNICODESET_RETURN_WITH_PARSE_ERROR(expected, actual, lexer, ec)                               \
    do {                                                                                                \
        (ec) = U_MALFORMED_SET;                                                                         \
        return;                                                                                         \
    } while (false)

#endif

}  // namespace

/**
 * Parse the pattern from the given RuleCharacterIterator.  The
 * iterator is advanced over the parsed pattern.
 * @param pattern The pattern, only used by debug traces.
 * @param parsePosition The ParsePosition underlying chars, only used by debug traces.
 * @param chars iterator over the pattern characters.  Upon return
 * it will be advanced to the first character after the parsed
 * pattern, or the end of the iteration if all characters are
 * parsed.
 * @param symbols symbol table to use to parse and dereference
 * variables, or null if none.
 * @param rebuiltPat the pattern that was parsed, rebuilt or
 * copied from the input pattern, as appropriate.
 * @param options a bit mask of zero or more of the following:
 * IGNORE_SPACE, CASE.
 */

void UnicodeSet::applyPattern(const UnicodeString &pattern,
                              const ParsePosition &parsePosition,
                              RuleCharacterIterator &chars,
                              const SymbolTable *symbols,
                              UnicodeString &rebuiltPat,
                              uint32_t options,
                              UnicodeSet &(UnicodeSet::*caseClosure)(int32_t attribute),
                              UErrorCode &ec) {
    if (U_FAILURE(ec)) return;
    Lexer lexer(pattern, parsePosition, chars, options, symbols);
    parseUnicodeSet(lexer, rebuiltPat, options, caseClosure, /*depth=*/0, ec);
}

void UnicodeSet::parseUnicodeSet(Lexer &lexer,
                                 UnicodeString& rebuiltPat,
                                 uint32_t options,
                                 UnicodeSet& (UnicodeSet::*caseClosure)(int32_t attribute),
                                 int32_t depth,
                                 UErrorCode &ec) {
    clear();

    if (depth > MAX_DEPTH) {
        U_UNICODESET_RETURN_WITH_PARSE_ERROR(("depth <= " + std::to_string(MAX_DEPTH)).c_str(),
                                             ("depth = " + std::to_string(depth)).c_str(), lexer, ec);
    }

    bool isComplement = false;
    // Whether to keep the syntax of the pattern at this level, only doing basic pretty-printing, e.g.,
    // turn [ c - z[a]a - b ] into [c-z[a]a-b], but not into [a-z].
    // This is true for a property query, or when there is a nested set.  Note that since we recurse,
    // innermost sets consisting only of ranges will get simplified.
    bool preserveSyntaxInPattern = false;
    // A pattern that preserves the original syntax but strips spaces, normalizes escaping, etc.
    UnicodeString prettyPrintedPattern;
    if (lexer.resemblesPropertyPattern()) {
        // UnicodeSet ::= property-query | named-element
        lexer.getCharacterIterator().skipIgnored(lexer.charsOptions());
        UnicodeSet propertyQuery;
        propertyQuery.applyPropertyPattern(lexer.getCharacterIterator(), prettyPrintedPattern, ec);
        U_UNICODESET_RETURN_IF_ERROR(ec);
        addAll(propertyQuery);
        preserveSyntaxInPattern = true;
    } else {
        // TODO(egg): In PD UTS 61, add ^ to set-operator, remove [^.
        // UnicodeSet ::=                [   Union ]
        //              | Complement ::= [ ^ Union ]
        // Extension:
        //              | MatcherSymbol
        // Where a MatcherSymbol may be a character or an escape.
        // Strings that would match MatcherSymbol effectively get removed from
        // all other terminals of the grammar, except [.
        if (lexer.lookahead().acceptUnescaped(u'[')) {
            prettyPrintedPattern.append(u'[');
            if (lexer.lookahead().acceptUnescaped(u'^')) {
                prettyPrintedPattern.append(u'^');
                isComplement = true;
            }
            parseUnion(lexer, prettyPrintedPattern, options, caseClosure, depth,
                       /*containsRestrictions=*/preserveSyntaxInPattern, ec);
            U_UNICODESET_RETURN_IF_ERROR(ec);
            if (!lexer.lookahead().acceptUnescaped(u']')) {
                U_UNICODESET_RETURN_WITH_PARSE_ERROR("]", lexer.lookahead().codePoint(ec), lexer, ec);
            }
            prettyPrintedPattern.append(u']');
        } else {
            const UnicodeSet *set = lexer.lookahead().standIn();
            if (set != nullptr) {
                *this = *set;
                this->_toPattern(rebuiltPat, /*escapeUnprintable=*/false);
                lexer.lookahead().moveAfter();
                return;
            }
            U_UNICODESET_RETURN_WITH_PARSE_ERROR(R"([: | \p | \P | \N | [)",
                                                 lexer.lookahead().codePoint(ec), lexer, ec);
        }
    }

    /**
     * Handle global flags (isComplement, case insensitivity).  If this
     * pattern should be compiled case-insensitive, then we need
     * to close over case BEFORE COMPLEMENTING.  This makes
     * patterns like /[^abc]/i work.
     */
    if ((options & USET_CASE_MASK) != 0) {
        (this->*caseClosure)(options);
    }
    if (isComplement) {
        complement().removeAllStrings();  // code point complement
    }
    if (preserveSyntaxInPattern) {
        rebuiltPat.append(prettyPrintedPattern);
    } else {
        _generatePattern(rebuiltPat, /*escapeUnprintable=*/false);
    }
}

void UnicodeSet::parseUnion(Lexer &lexer,
                            UnicodeString &rebuiltPat,
                            uint32_t options,
                            UnicodeSet &(UnicodeSet::*caseClosure)(int32_t attribute),
                            int32_t depth,
                            bool &containsRestrictions,
                            UErrorCode &ec) {
    // Union ::= Terms
    //         | UnescapedHyphenMinus Terms
    //         | Terms UnescapedHyphenMinus
    //         | UnescapedHyphenMinus Terms UnescapedHyphenMinus
    // Terms ::= ""
    //         | Terms Term
    if (lexer.lookahead().acceptUnescapedNotStandIn(u'-')) {
        add(u'-');
        // When we otherwise preserve the syntax, we escape an initial UnescapedHyphenMinus, but not a
        // final one, for consistency with older ICU behaviour.
        rebuiltPat.append(u"\\-");
    }
    while (!lexer.atEnd()) {
        if (lexer.lookahead().acceptUnescapedNotStandIn(u'-')) {
            // We can be here on the first iteration: [--] is allowed by the
            // grammar and by the old parser.
            rebuiltPat.append(u'-');
            add(u'-');
            return;
        } else if (lexer.lookahead().isUnescapedNotStandIn(u'$')) {
            Lexer::Lookahead afterDollar = lexer.lookahead().oneMore();
            if (afterDollar.isUnescaped(u']')) {
                // ICU extensions: A $ is allowed as a literal-element.
                // A Term at the end of a Union consisting of a single $ is an anchor.
                rebuiltPat.append(u'$');
                // Consume the dollar.
                lexer.lookahead().moveAfter();
                add(U_ETHER);
                containsRestrictions = true;
                return;
            }
        }
        if (lexer.lookahead().isUnescapedNotStandIn(u']')) {
            return;
        }
        parseTerm(lexer, rebuiltPat, options, caseClosure, depth, containsRestrictions, ec);
        U_UNICODESET_RETURN_IF_ERROR(ec);
    }
}

void UnicodeSet::parseTerm(Lexer &lexer,
                           UnicodeString &rebuiltPat,
                           uint32_t options,
                           UnicodeSet &(UnicodeSet::*caseClosure)(int32_t attribute),
                           int32_t depth,
                           bool &containsRestriction,
                           UErrorCode &ec) {
    // Term ::= Elements
    //        | Restriction
    if (lexer.lookahead().standIn() != nullptr || lexer.lookahead().isUnescaped('[') ||
        lexer.resemblesPropertyPattern()) {
        containsRestriction = true;
        parseRestriction(lexer, rebuiltPat, options, caseClosure, depth, ec);
        U_UNICODESET_RETURN_IF_ERROR(ec);
    } else {
        parseElements(lexer, rebuiltPat, caseClosure, depth, ec);
        U_UNICODESET_RETURN_IF_ERROR(ec);
    }
}

void UnicodeSet::parseRestriction(Lexer &lexer,
                                  UnicodeString &rebuiltPat,
                                  uint32_t options,
                                  UnicodeSet &(UnicodeSet::*caseClosure)(int32_t attribute),
                                  int32_t depth,
                                  UErrorCode &ec) {
    // Restriction ::= UnicodeSet
    //               | Intersection ::= Restriction & UnicodeSet
    //               | Difference   ::= Restriction - UnicodeSet
    // Start by parsing the first UnicodeSet.
    UnicodeSet leftHandSide;
    leftHandSide.parseUnicodeSet(lexer, rebuiltPat, options, caseClosure, depth + 1, ec);
    addAll(leftHandSide);
    U_UNICODESET_RETURN_IF_ERROR(ec);
    // Now keep looking for an operator that would continue the Restriction.
    // The loop terminates because when chars.atEnd(), op == DONE, so we go into the else branch and
    // return.
    for (;;) {
        if (lexer.lookahead().standIn() != nullptr) {
            // Not an operator, end of the Restriction.
            return;
        }
        if (lexer.lookahead().acceptUnescaped(u'&')) {
            // Intersection ::= Restriction & UnicodeSet
            rebuiltPat.append(u'&');
            UnicodeSet rightHandSide;
            rightHandSide.parseUnicodeSet(lexer, rebuiltPat, options, caseClosure, depth + 1, ec);
            U_UNICODESET_RETURN_IF_ERROR(ec);
            retainAll(rightHandSide);
        } else if (lexer.lookahead().isUnescaped(u'-')) {
            // Here the grammar requires two tokens of lookahead to figure out whether the - the operator
            // of a Difference or an UnescapedHyphenMinus in the enclosing Union.
            if (lexer.lookahead().oneMore().isUnescaped(u']')) {
                // The operator is actually an UnescapedHyphenMinus; terminate the Restriction before it.
                return;
            }
            // Consume the hyphen-minus.
            lexer.lookahead().moveAfter();
            // Difference ::= Restriction - UnicodeSet
            rebuiltPat.append(u'-');
            UnicodeSet rightHandSide;
            rightHandSide.parseUnicodeSet(lexer, rebuiltPat, options, caseClosure, depth + 1, ec);
            U_UNICODESET_RETURN_IF_ERROR(ec);
            removeAll(rightHandSide);
        } else {
            // Not an operator, end of the Restriction.
            return;
        }
    }
}

void UnicodeSet::parseElements(Lexer &lexer,
                               UnicodeString &rebuiltPat,
                               UnicodeSet &(UnicodeSet::*caseClosure)(int32_t attribute),
                               int32_t depth,
                               UErrorCode &ec) {
    // Elements     ::= Element
    //                | Range
    // Range        ::= RangeElement - RangeElement
    // RangeElement ::= literal-element
    //                | escaped-element
    // Element      ::= RangeElement
    //                | string-literal
    const UChar32 first = lexer.lookahead().codePoint(ec);
    U_UNICODESET_RETURN_IF_ERROR(ec);
    if (!lexer.lookahead().escaped()) {
        switch (first) {
        case u'-':
        case u'&':
        case u'[':
        case u']':
        case u'^':
            U_UNICODESET_RETURN_WITH_PARSE_ERROR("RangeElement | string-literal", first, lexer, ec);
        case u'{': {
            lexer.lookahead().moveAfter();
            rebuiltPat.append(u'{');
            UnicodeString string;
            while (!lexer.atEnd()) {
                if (lexer.lookahead().acceptUnescaped('}')) {
                    rebuiltPat.append(u'}');
                    add(string);
                    return;
                }
                const UChar32 c = lexer.lookahead().codePoint(ec);
                U_UNICODESET_RETURN_IF_ERROR(ec);
                lexer.lookahead().moveAfter();
                _appendToPat(rebuiltPat, c, /*escapeUnprintable=*/false);
                string.append(c);
            }
            U_UNICODESET_RETURN_WITH_PARSE_ERROR("}", RuleCharacterIterator::DONE, lexer, ec);
        }
        case u'}':
        case u'$':
            // Disallowed by UTS #61, but historically accepted by ICU.  This is an extension.
        default:
            break;
        }
    }
    lexer.lookahead().moveAfter();
    _appendToPat(rebuiltPat, first, /*escapeUnprintable=*/false);
    if (!lexer.lookahead().isUnescapedNotStandIn(u'-')) {
        // No operator,
        // Elements ::= Element
        add(first);
        return;
    }
    // Here the grammar requires two tokens of lookahead to figure out whether the - the operator
    // of a Range or an UnescapedHyphenMinus in the enclosing Union.
    if (lexer.lookahead().oneMore().isUnescaped(u']')) {
        // The operator is actually an UnescapedHyphenMinus; terminate the Elements before it.
        add(first);
        return;
    }
    // Consume the hyphen-minus.
    lexer.lookahead().moveAfter();
    // Elements ::= Range ::= RangeElement - RangeElement
    rebuiltPat.append(u'-');
    const UChar32 last = lexer.lookahead().codePoint(ec);
    U_UNICODESET_RETURN_IF_ERROR(ec);
    if (lexer.lookahead().standIn() != nullptr) {
        U_UNICODESET_RETURN_WITH_PARSE_ERROR("RangeElement", last, lexer, ec);
    }
    if (!lexer.lookahead().escaped()) {
        switch (last) {
        case u'-':
        case u'&':
        case u'[':
        case u']':
        case u'^':
        case u'{':
            U_UNICODESET_RETURN_WITH_PARSE_ERROR("RangeElement", last, lexer, ec);
        case u'$': {
            // Disallowed by UTS #61, but historically accepted by ICU except at the end of a Union.
            // This is an extension.
            if (lexer.lookahead().oneMore().isUnescaped(u']')) {
                U_UNICODESET_RETURN_WITH_PARSE_ERROR("Term after Range ending in unescaped $", u']',
                                                     lexer, ec);
            }
            break;
        }
        case u'}':
            // Disallowed by UTS #61, but historically accepted by ICU.  This is an extension.
        default:
            break;
        }
    }
    lexer.lookahead().moveAfter();
    _appendToPat(rebuiltPat, last, /*escapeUnprintable=*/false);
    if (last <= first) {
        U_UNICODESET_RETURN_WITH_PARSE_ERROR(
            "first < last in Range", UnicodeString(last) + u"-" + UnicodeString(first), lexer, ec);
    }
    add(first, last);
    return;
}

//----------------------------------------------------------------
// Property set implementation
//----------------------------------------------------------------

namespace {

UBool numericValueFilter(UChar32 ch, void* context) {
    return u_getNumericValue(ch) == *static_cast<double*>(context);
}

UBool generalCategoryMaskFilter(UChar32 ch, void* context) {
    int32_t value = *static_cast<int32_t*>(context);
    return (U_GET_GC_MASK((UChar32) ch) & value) != 0;
}

UBool versionFilter(UChar32 ch, void* context) {
    static const UVersionInfo none = { 0, 0, 0, 0 };
    UVersionInfo v;
    u_charAge(ch, v);
    UVersionInfo* version = static_cast<UVersionInfo*>(context);
    return uprv_memcmp(&v, &none, sizeof(v)) > 0 && uprv_memcmp(&v, version, sizeof(v)) <= 0;
}

typedef struct {
    UProperty prop;
    int32_t value;
} IntPropertyContext;

UBool intPropertyFilter(UChar32 ch, void* context) {
    IntPropertyContext* c = static_cast<IntPropertyContext*>(context);
    return u_getIntPropertyValue(ch, c->prop) == c->value;
}

UBool scriptExtensionsFilter(UChar32 ch, void* context) {
    return uscript_hasScript(ch, *static_cast<UScriptCode*>(context));
}

UBool idTypeFilter(UChar32 ch, void* context) {
    return u_hasIDType(ch, *static_cast<UIdentifierType*>(context));
}

}  // namespace

/**
 * Generic filter-based scanning code for UCD property UnicodeSets.
 */
void UnicodeSet::applyFilter(UnicodeSet::Filter filter,
                             void* context,
                             const UnicodeSet* inclusions,
                             UErrorCode &status) {
    if (U_FAILURE(status)) return;

    // Logically, walk through all Unicode characters, noting the start
    // and end of each range for which filter.contain(c) is
    // true.  Add each range to a set.
    //
    // To improve performance, use an inclusions set which
    // encodes information about character ranges that are known
    // to have identical properties.
    // inclusions contains the first characters of
    // same-value ranges for the given property.

    clear();

    UChar32 startHasProperty = -1;
    int32_t limitRange = inclusions->getRangeCount();

    for (int j=0; j<limitRange; ++j) {
        // get current range
        UChar32 start = inclusions->getRangeStart(j);
        UChar32 end = inclusions->getRangeEnd(j);

        // for all the code points in the range, process
        for (UChar32 ch = start; ch <= end; ++ch) {
            // only add to this UnicodeSet on inflection points --
            // where the hasProperty value changes to false
            if ((*filter)(ch, context)) {
                if (startHasProperty < 0) {
                    startHasProperty = ch;
                }
            } else if (startHasProperty >= 0) {
                add(startHasProperty, ch-1);
                startHasProperty = -1;
            }
        }
    }
    if (startHasProperty >= 0) {
        add(startHasProperty, static_cast<UChar32>(0x10FFFF));
    }
    if (isBogus() && U_SUCCESS(status)) {
        // We likely ran out of memory. AHHH!
        status = U_MEMORY_ALLOCATION_ERROR;
    }
}

namespace {

UBool mungeCharName(char* dst, const char* src, int32_t dstCapacity) {
    /* Note: we use ' ' in compiler code page */
    int32_t j = 0;
    char ch;
    --dstCapacity; /* make room for term. zero */
    while ((ch = *src++) != 0) {
        if (ch == ' ' && (j==0 || (j>0 && dst[j-1]==' '))) {
            continue;
        }
        if (j >= dstCapacity) return false;
        dst[j++] = ch;
    }
    if (j > 0 && dst[j-1] == ' ') --j;
    dst[j] = 0;
    return true;
}

}  // namespace

//----------------------------------------------------------------
// Property set API
//----------------------------------------------------------------

#define FAIL(ec) UPRV_BLOCK_MACRO_BEGIN { \
    ec=U_ILLEGAL_ARGUMENT_ERROR; \
    return *this; \
} UPRV_BLOCK_MACRO_END

UnicodeSet&
UnicodeSet::applyIntPropertyValue(UProperty prop, int32_t value, UErrorCode& ec) {
    if (U_FAILURE(ec) || isFrozen()) { return *this; }
    if (prop == UCHAR_GENERAL_CATEGORY_MASK) {
        const UnicodeSet* inclusions = CharacterProperties::getInclusionsForProperty(prop, ec);
        applyFilter(generalCategoryMaskFilter, &value, inclusions, ec);
    } else if (prop == UCHAR_SCRIPT_EXTENSIONS) {
        const UnicodeSet* inclusions = CharacterProperties::getInclusionsForProperty(prop, ec);
        UScriptCode script = static_cast<UScriptCode>(value);
        applyFilter(scriptExtensionsFilter, &script, inclusions, ec);
    } else if (prop == UCHAR_IDENTIFIER_TYPE) {
        const UnicodeSet* inclusions = CharacterProperties::getInclusionsForProperty(prop, ec);
        UIdentifierType idType = static_cast<UIdentifierType>(value);
        applyFilter(idTypeFilter, &idType, inclusions, ec);
    } else if (0 <= prop && prop < UCHAR_BINARY_LIMIT) {
        if (value == 0 || value == 1) {
            const USet *set = u_getBinaryPropertySet(prop, &ec);
            if (U_FAILURE(ec)) { return *this; }
            copyFrom(*UnicodeSet::fromUSet(set), true);
            if (value == 0) {
                complement().removeAllStrings();  // code point complement
            }
        } else {
            clear();
        }
    } else if (UCHAR_INT_START <= prop && prop < UCHAR_INT_LIMIT) {
        const UnicodeSet* inclusions = CharacterProperties::getInclusionsForProperty(prop, ec);
        IntPropertyContext c = {prop, value};
        applyFilter(intPropertyFilter, &c, inclusions, ec);
    } else {
        ec = U_ILLEGAL_ARGUMENT_ERROR;
    }
    return *this;
}

UnicodeSet&
UnicodeSet::applyPropertyAlias(const UnicodeString& prop,
                               const UnicodeString& value,
                               UErrorCode& ec) {
    if (U_FAILURE(ec) || isFrozen()) return *this;

    // prop and value used to be converted to char * using the default
    // converter instead of the invariant conversion.
    // This should not be necessary because all Unicode property and value
    // names use only invariant characters.
    // If there are any variant characters, then we won't find them anyway.
    // Checking first avoids assertion failures in the conversion.
    if( !uprv_isInvariantUString(prop.getBuffer(), prop.length()) ||
        !uprv_isInvariantUString(value.getBuffer(), value.length())
    ) {
        FAIL(ec);
    }
    CharString pname, vname;
    pname.appendInvariantChars(prop, ec);
    vname.appendInvariantChars(value, ec);
    if (U_FAILURE(ec)) return *this;

    UProperty p;
    int32_t v;
    UBool invert = false;

    if (value.length() > 0) {
        p = u_getPropertyEnum(pname.data());
        if (p == UCHAR_INVALID_CODE) FAIL(ec);

        // Treat gc as gcm
        if (p == UCHAR_GENERAL_CATEGORY) {
            p = UCHAR_GENERAL_CATEGORY_MASK;
        }

        if ((p >= UCHAR_BINARY_START && p < UCHAR_BINARY_LIMIT) ||
            (p >= UCHAR_INT_START && p < UCHAR_INT_LIMIT) ||
            (p >= UCHAR_MASK_START && p < UCHAR_MASK_LIMIT)) {
            v = u_getPropertyValueEnum(p, vname.data());
            if (v == UCHAR_INVALID_CODE) {
                // Handle numeric CCC
                if (p == UCHAR_CANONICAL_COMBINING_CLASS ||
                    p == UCHAR_TRAIL_CANONICAL_COMBINING_CLASS ||
                    p == UCHAR_LEAD_CANONICAL_COMBINING_CLASS) {
                    char* end;
                    double val = uprv_strtod(vname.data(), &end);
                    // Anything between 0 and 255 is valid even if unused.
                    // Cast double->int only after range check.
                    // We catch NaN here because comparing it with both 0 and 255 will be false
                    // (as are all comparisons with NaN).
                    if (*end != 0 || !(0 <= val && val <= 255) ||
                            (v = static_cast<int32_t>(val)) != val) {
                        // non-integral value or outside 0..255, or trailing junk
                        FAIL(ec);
                    }
                } else {
                    FAIL(ec);
                }
            }
        }

        else {

            switch (p) {
            case UCHAR_NUMERIC_VALUE:
                {
                    char* end;
                    double val = uprv_strtod(vname.data(), &end);
                    if (*end != 0) {
                        FAIL(ec);
                    }
                    applyFilter(numericValueFilter, &val,
                                CharacterProperties::getInclusionsForProperty(p, ec), ec);
                    return *this;
                }
            case UCHAR_NAME:
                {
                    // Must munge name, since u_charFromName() does not do
                    // 'loose' matching.
                    char buf[128]; // it suffices that this be > uprv_getMaxCharNameLength
                    if (!mungeCharName(buf, vname.data(), sizeof(buf))) FAIL(ec);
                    UChar32 ch = u_charFromName(U_EXTENDED_CHAR_NAME, buf, &ec);
                    if (U_SUCCESS(ec)) {
                        clear();
                        add(ch);
                        return *this;
                    } else {
                        FAIL(ec);
                    }
                }
            case UCHAR_UNICODE_1_NAME:
                // ICU 49 deprecates the Unicode_1_Name property APIs.
                FAIL(ec);
            case UCHAR_AGE:
                {
                    // Must munge name, since u_versionFromString() does not do
                    // 'loose' matching.
                    char buf[128];
                    if (!mungeCharName(buf, vname.data(), sizeof(buf))) FAIL(ec);
                    UVersionInfo version;
                    u_versionFromString(version, buf);
                    applyFilter(versionFilter, &version,
                                CharacterProperties::getInclusionsForProperty(p, ec), ec);
                    return *this;
                }
            case UCHAR_SCRIPT_EXTENSIONS:
                v = u_getPropertyValueEnum(UCHAR_SCRIPT, vname.data());
                if (v == UCHAR_INVALID_CODE) {
                    FAIL(ec);
                }
                // fall through to calling applyIntPropertyValue()
                break;
            case UCHAR_IDENTIFIER_TYPE:
                v = u_getPropertyValueEnum(p, vname.data());
                if (v == UCHAR_INVALID_CODE) {
                    FAIL(ec);
                }
                // fall through to calling applyIntPropertyValue()
                break;
            default:
                // p is a non-binary, non-enumerated property that we
                // don't support (yet).
                FAIL(ec);
            }
        }
    }

    else {
        // value is empty.  Interpret as General Category, Script, or
        // Binary property.
        p = UCHAR_GENERAL_CATEGORY_MASK;
        v = u_getPropertyValueEnum(p, pname.data());
        if (v == UCHAR_INVALID_CODE) {
            p = UCHAR_SCRIPT;
            v = u_getPropertyValueEnum(p, pname.data());
            if (v == UCHAR_INVALID_CODE) {
                p = u_getPropertyEnum(pname.data());
                if (p >= UCHAR_BINARY_START && p < UCHAR_BINARY_LIMIT) {
                    v = 1;
                } else if (0 == uprv_comparePropertyNames(ANY, pname.data())) {
                    set(MIN_VALUE, MAX_VALUE);
                    return *this;
                } else if (0 == uprv_comparePropertyNames(ASCII, pname.data())) {
                    set(0, 0x7F);
                    return *this;
                } else if (0 == uprv_comparePropertyNames(ASSIGNED, pname.data())) {
                    // [:Assigned:]=[:^Cn:]
                    p = UCHAR_GENERAL_CATEGORY_MASK;
                    v = U_GC_CN_MASK;
                    invert = true;
                } else {
                    FAIL(ec);
                }
            }
        }
    }

    applyIntPropertyValue(p, v, ec);
    if(invert) {
        complement().removeAllStrings();  // code point complement
    }

    if (isBogus() && U_SUCCESS(ec)) {
        // We likely ran out of memory. AHHH!
        ec = U_MEMORY_ALLOCATION_ERROR;
    }
    return *this;
}

//----------------------------------------------------------------
// Property set patterns
//----------------------------------------------------------------

/**
 * Return true if the given position, in the given pattern, appears
 * to be the start of a property set pattern.
 */
UBool UnicodeSet::resemblesPropertyPattern(const UnicodeString& pattern,
                                           int32_t pos) {
    // Patterns are at least 5 characters long
    if ((pos+5) > pattern.length()) {
        return false;
    }

    // Look for an opening [:, [:^, \p, or \P
    return isPOSIXOpen(pattern, pos) || isPerlOpen(pattern, pos) || isNameOpen(pattern, pos);
}

/**
 * Return true if the given iterator appears to point at a
 * property pattern.  Regardless of the result, return with the
 * iterator unchanged.
 * @param chars iterator over the pattern characters.  Upon return
 * it will be unchanged.
 * @param iterOpts RuleCharacterIterator options
 */
UBool UnicodeSet::resemblesPropertyPattern(RuleCharacterIterator& chars,
                                           int32_t iterOpts) {
    // NOTE: literal will always be false, because we don't parse escapes.
    UBool result = false, literal;
    UErrorCode ec = U_ZERO_ERROR;
    iterOpts &= ~RuleCharacterIterator::PARSE_ESCAPES;
    RuleCharacterIterator::Pos pos;
    chars.getPos(pos);
    UChar32 c = chars.next(iterOpts, literal, ec);
    if (c == u'[' || c == u'\\') {
        UChar32 d = chars.next(iterOpts & ~RuleCharacterIterator::SKIP_WHITESPACE,
                               literal, ec);
        result = (c == u'[') ? (d == u':') :
                               (d == u'N' || d == u'p' || d == u'P');
    }
    chars.setPos(pos);
    return result && U_SUCCESS(ec);
}

/**
 * Parse the given property pattern at the given parse position.
 */
UnicodeSet& UnicodeSet::applyPropertyPattern(const UnicodeString& pattern,
                                             ParsePosition& ppos,
                                             UErrorCode &ec) {
    int32_t pos = ppos.getIndex();

    UBool posix = false; // true for [:pat:], false for \p{pat} \P{pat} \N{pat}
    UBool isName = false; // true for \N{pat}, o/w false
    UBool invert = false;

    if (U_FAILURE(ec)) return *this;

    // Minimum length is 5 characters, e.g. \p{L}
    if ((pos+5) > pattern.length()) {
        FAIL(ec);
    }

    // On entry, ppos should point to one of the following locations:
    // Look for an opening [:, [:^, \p, or \P
    if (isPOSIXOpen(pattern, pos)) {
        posix = true;
        pos += 2;
        pos = ICU_Utility::skipWhitespace(pattern, pos);
        if (pos < pattern.length() && pattern.charAt(pos) == u'^') {
            ++pos;
            invert = true;
        }
    } else if (isPerlOpen(pattern, pos) || isNameOpen(pattern, pos)) {
        char16_t c = pattern.charAt(pos+1);
        invert = (c == u'P');
        isName = (c == u'N');
        pos += 2;
        pos = ICU_Utility::skipWhitespace(pattern, pos);
        if (pos == pattern.length() || pattern.charAt(pos++) != u'{') {
            // Syntax error; "\p" or "\P" not followed by "{"
            FAIL(ec);
        }
    } else {
        // Open delimiter not seen
        FAIL(ec);
    }

    // Look for the matching close delimiter, either :] or }
    int32_t close;
    if (posix) {
      close = pattern.indexOf(u":]", 2, pos);
    } else {
      close = pattern.indexOf(u'}', pos);
    }
    if (close < 0) {
        // Syntax error; close delimiter missing
        FAIL(ec);
    }

    // Look for an '=' sign.  If this is present, we will parse a
    // medium \p{gc=Cf} or long \p{GeneralCategory=Format}
    // pattern.
    int32_t equals = pattern.indexOf(u'=', pos);
    UnicodeString propName, valueName;
    if (equals >= 0 && equals < close && !isName) {
        // Equals seen; parse medium/long pattern
        pattern.extractBetween(pos, equals, propName);
        pattern.extractBetween(equals+1, close, valueName);
    }

    else {
        // Handle case where no '=' is seen, and \N{}
        pattern.extractBetween(pos, close, propName);
            
        // Handle \N{name}
        if (isName) {
            // This is a little inefficient since it means we have to
            // parse NAME_PROP back to UCHAR_NAME even though we already
            // know it's UCHAR_NAME.  If we refactor the API to
            // support args of (UProperty, char*) then we can remove
            // NAME_PROP and make this a little more efficient.
            valueName = propName;
            propName = NAME_PROP;
        }
    }

    applyPropertyAlias(propName, valueName, ec);

    if (U_SUCCESS(ec)) {
        if (invert) {
            complement().removeAllStrings();  // code point complement
        }

        // Move to the limit position after the close delimiter if the
        // parse succeeded.
        ppos.setIndex(close + (posix ? 2 : 1));
    }

    return *this;
}

/**
 * Parse a property pattern.
 * @param chars iterator over the pattern characters.  Upon return
 * it will be advanced to the first character after the parsed
 * pattern, or the end of the iteration if all characters are
 * parsed.
 * @param rebuiltPat the pattern that was parsed, rebuilt or
 * copied from the input pattern, as appropriate.
 */
void UnicodeSet::applyPropertyPattern(RuleCharacterIterator& chars,
                                      UnicodeString& rebuiltPat,
                                      UErrorCode& ec) {
    if (U_FAILURE(ec)) return;
    UnicodeString pattern;
    chars.lookahead(pattern);
    ParsePosition pos(0);
    applyPropertyPattern(pattern, pos, ec);
    if (U_FAILURE(ec)) return;
    if (pos.getIndex() == 0) {
        // syntaxError(chars, "Invalid property pattern");
        ec = U_MALFORMED_SET;
        return;
    }
    chars.jumpahead(pos.getIndex());
    rebuiltPat.append(pattern, 0, pos.getIndex());
}

U_NAMESPACE_END
