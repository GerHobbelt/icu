// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2.h"
#include "unicode/messageformat2_data_model.h"
#include "uvector.h" // U_ASSERT

U_NAMESPACE_BEGIN namespace message2 {

// Generates a string representation of a data model
// ------------------------------------------------

using Binding    = MessageFormatDataModel::Binding;
using Bindings    = MessageFormatDataModel::Bindings;
using Key         = MessageFormatDataModel::Key;
using SelectorKeys = MessageFormatDataModel::SelectorKeys;
using KeyList     = MessageFormatDataModel::KeyList;
using Literal     = MessageFormatDataModel::Literal;
using OptionMap   = MessageFormatDataModel::OptionMap;
using Expression  = MessageFormatDataModel::Expression;
using ExpressionList  = MessageFormatDataModel::ExpressionList;
using Operand     = MessageFormatDataModel::Operand;
using Operator    = MessageFormatDataModel::Operator;
using Pattern     = MessageFormatDataModel::Pattern;
using PatternPart = MessageFormatDataModel::PatternPart;
using Reserved    = MessageFormatDataModel::Reserved;
using VariantMap    = MessageFormatDataModel::VariantMap;

#define SERIALIZER MessageFormatter::Serializer

// Private helper methods

void SERIALIZER::whitespace() {
    result += SPACE;
}

void SERIALIZER::emit(UChar32 c) {
    result += c;
}

void SERIALIZER::emit(const UnicodeString& s) {
    result += s;
}

void SERIALIZER::emit(const Name& s) {
    result += s.sigil();
    result += s.name();
}

template <int32_t N>
void SERIALIZER::emit(const UChar32 (&token)[N]) {
    // Don't emit the terminator
    for (int32_t i = 0; i < N - 1; i++) {
        emit(token[i]);
    }
}

void SERIALIZER::emit(const Literal& l) {
    if (l.isQuoted) {
      emit(PIPE);
      const UnicodeString& contents = l.stringContents();
      for (int32_t i = 0; ((int32_t) i) < contents.length(); i++) {
        // Re-escape any PIPE or BACKSLASH characters
        switch(contents[i]) {
        case BACKSLASH:
        case PIPE: {
          emit(BACKSLASH);
          break;
        }
        default: {
          break;
        }
        }
        emit(contents[i]);
      }
      emit(PIPE);
    } else {
      emit(l.stringContents());
    }
}

void SERIALIZER::emit(const Key& k) {
    if (k.isWildcard()) {
        emit(ASTERISK);
        return;
    }
    emit(k.asLiteral());
}

void SERIALIZER::emit(const SelectorKeys& k) {
  const KeyList& ks = k.getKeys();
  int32_t len = ks.length();
  // It would be an error for `keys` to be empty;
  // that would mean this is the single `pattern`
  // variant, and in that case, this method shouldn't be called
  U_ASSERT(len > 0);
  for (int32_t i = 0; i < len; i++) {
    if (i != 0) {
      whitespace();
    }
    emit(*ks.get(i));
  }
}

void SERIALIZER::emit(const Operand& rand) {
    U_ASSERT(!rand.isNull());

    if (rand.isVariable()) {
        emit(*rand.asVariable());
    } else {
        // Literal: quoted or unquoted
        emit(*rand.asLiteral());
    }
}

void SERIALIZER::emit(const OptionMap& options) {
    int32_t pos = OptionMap::FIRST;
    UnicodeString k;
    const Operand* v;
    while (options.next(pos, k, v)) {
      whitespace();
      emit(k);
      emit(EQUALS);
      emit(*v);
    }
}

void SERIALIZER::emit(const Expression& expr) {
    emit(LEFT_CURLY_BRACE);

    if (!expr.isReserved() && !expr.isFunctionCall()) {
        // Literal or variable, no annotation
        emit(expr.getOperand());
    } else {
        // Function call or reserved
        if (!expr.isStandaloneAnnotation()) {
          // Must be a function call that has an operand
          emit(expr.getOperand());
          whitespace();
        }
        const Operator& rator = expr.getOperator();
        if (rator.isReserved()) {
          const Reserved& reserved = rator.asReserved();
          // Re-escape '\' / '{' / '|' / '}'
          for (int32_t i = 0; i < reserved.numParts(); i++) {
            const Literal& l = *reserved.getPart(i);
            if (l.isQuoted) {
              emit(l);
            } else {
              const UnicodeString& s = l.stringContents();
              for (int32_t j = 0; ((int32_t) j) < s.length(); j++) {
                switch(s[j]) {
                case LEFT_CURLY_BRACE:
                case PIPE:
                case RIGHT_CURLY_BRACE:
                case BACKSLASH: {
                  emit(BACKSLASH);
                  break;
                }
                default:
                  break;
                }
                emit(s[j]);
              }
            }
          }
        } else {
            emit(rator.getFunctionName());
            // No whitespace after function name, in case it has
            // no options. (when there are options, emit(OptionMap) will
            // emit the leading whitespace)
            emit(rator.getOptions());
        }
    }
    
    emit(RIGHT_CURLY_BRACE);
}

void SERIALIZER::emit(const PatternPart& part) {
    if (part.isText()) {
        // Raw text
        const UnicodeString& text = part.asText();
        // Re-escape '{'/'}'/'\'
        for (int32_t i = 0; ((int32_t) i) < text.length(); i++) {
          switch(text[i]) {
          case BACKSLASH:
          case LEFT_CURLY_BRACE:
          case RIGHT_CURLY_BRACE: {
            emit(BACKSLASH);
            break;
          }
          default:
            break;
          }
          emit(text[i]);
        }
        return;
    }
    // Expression
    emit(part.contents());
}

void SERIALIZER::emit(const Pattern& pat) {
    int32_t len = pat.numParts();
    emit(LEFT_CURLY_BRACE);
    for (int32_t i = 0; i < len; i++) {
        // No whitespace is needed here -- see the `pattern` nonterminal in the grammar
        emit(*pat.getPart(i));
    }
    emit(RIGHT_CURLY_BRACE);
}
                    
void SERIALIZER::serializeDeclarations() {
    const Bindings& locals = dataModel.getLocalVariables();
    
    for (int32_t i = 0; i < locals.length(); i++) {
        const Binding& b = *locals.get(i);
        // No whitespace needed here -- see `message` in the grammar
        emit(ID_LET);
        whitespace();
        emit(b.var);
        // No whitespace needed here -- see `declaration` in the grammar
        emit(EQUALS);
        // No whitespace needed here -- see `declaration` in the grammar
        // getValue() is guaranteed to return a non-null pointer
        emit(*(b.getValue()));
    }
}

void SERIALIZER::serializeSelectors() {
    U_ASSERT(dataModel.hasSelectors());
    const ExpressionList& selectors = dataModel.getSelectors();
    int32_t len = selectors.length();
    U_ASSERT(len > 0);

    emit(ID_MATCH);
    for (int32_t i = 0; i < len; i++) {
        // No whitespace needed here -- see `selectors` in the grammar
        emit(*selectors.get(i));
    }
}

void SERIALIZER::serializeVariants() {
    U_ASSERT(dataModel.hasSelectors());
    const VariantMap& variants = dataModel.getVariants();
    int32_t pos = VariantMap::FIRST;

    const SelectorKeys* selectorKeys;
    const Pattern* pattern;

    while (variants.next(pos, selectorKeys, pattern)) {
      emit(ID_WHEN);
      whitespace();
      emit(*selectorKeys);
      // No whitespace needed here -- see `variant` in the grammar
      emit(*pattern);
    }    
}


// Main (public) serializer method
void SERIALIZER::serialize() {
    serializeDeclarations();
    // Pattern message
    if (!dataModel.hasSelectors()) {
      emit(dataModel.getPattern());
    } else {
      // Selectors message
      serializeSelectors();
      serializeVariants();
    }
}

} // namespace message2
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

