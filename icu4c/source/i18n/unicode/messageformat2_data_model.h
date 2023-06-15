// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT_DATA_MODEL_H
#define MESSAGEFORMAT_DATA_MODEL_H

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2_macros.h"
#include "unicode/parseerr.h"
#include "unicode/unistr.h"
#include "unicode/utypes.h"
#include "hash.h"
#include "uvector.h"

U_NAMESPACE_BEGIN  namespace message2 {

/*
  Use an internal "parse error" structure to make it easier to translate
  absolute offsets to line offsets.
  This is translated back to a `UParseError` at the end of parsing.
*/
typedef struct MessageParseError {
    // The line on which the error occurred
    uint32_t line;
    // The offset, relative to the erroneous line, on which the error occurred
    uint32_t offset;
    // The total number of characters seen before advancing to the current line. It has a value of 0 if line == 0.
    // It includes newline characters, because the index does too.
    uint32_t lengthBeforeCurrentLine;

    // This parser doesn't yet use the last two fields.
    UChar   preContext[U_PARSE_CONTEXT_LEN];
    UChar   postContext[U_PARSE_CONTEXT_LEN];
} MessageParseError;

// TBD
using FunctionName   = UnicodeString;
// TBD
using VariableName   = UnicodeString;
// TBD
using String         = UnicodeString;


// -----------------------------------------------------------------------
// Helpers (not public)

// For now, represent variable names as strings
#define compareVariableName uhash_compareUnicodeString


// -----------------------------------------------------------------------
// Public MessageFormatDataModel class

/**
 * <p>MessageFormat2 is a Technical Preview API implementing MessageFormat 2.0.
 * Since it is not final, documentation has not yet been added everywhere.
 *
 * <p>See <a target="github"
href="https://github.com/unicode-org/message-format-wg/blob/main/spec/syntax.md">the
 * description of the syntax with examples and use cases</a> and the corresponding
 * <a target="github"
href="https://github.com/unicode-org/message-format-wg/blob/main/spec/message.abnf">ABNF</a> grammar.</p>
 *
 * The `MessageFormatDataModel` class describes a parsed representation of the text of a message.
 * This representation is public as higher-level APIs for messages will need to know its public
 * interface: for example, to re-instantiate a parsed message with different values for imported
variables.
 *
 * @internal ICU 74.0 technology preview
 * @deprecated This API is for technology preview only.
 */
class MessageFormatter;

// TODO: this is hurting my brain -- think about whether this is necessary
/*
  This exists because even though AST nodes are immutable, it's
  possible someone could use build() to create data models that have
  different lifetimes
*/

template <typename T1>
static void copyElements(UElement *dst, UElement *src) {
  dst->pointer = T1::copy(*(static_cast<T1 *>(src->pointer)));
}

static void copyStrings(UElement *dst, UElement *src) {
  dst->pointer = new UnicodeString(*(static_cast<UnicodeString *>(src->pointer)));
}

// Helpers
template <typename V>
static Hashtable* copyHashtable(const Hashtable& other) {
  UErrorCode errorCode = U_ZERO_ERROR;
  // No value comparator needed
  LocalPointer<Hashtable> adoptedContents(new Hashtable(compareVariableName, nullptr, errorCode));
  if (U_FAILURE(errorCode)) {
    return nullptr;
  }
  // The hashtable owns the values
  adoptedContents->setValueDeleter(uprv_deleteUObject);

  // Copy all the key/value bindings over
  const UHashElement *e;
  int32_t pos = UHASH_FIRST;
  V *val;
  while ((e = other.nextElement(pos)) != nullptr) {
    val = V::copy(*(static_cast<V *>(e->value.pointer)));
    if (val == nullptr) {
      return nullptr;
    }
    UnicodeString *s = static_cast<UnicodeString *>(e->key.pointer);
    adoptedContents->put(*s, val, errorCode);
    if (U_FAILURE(errorCode)) {
      return nullptr;
    }
  }

  return adoptedContents.orphan();
}

static UVector* copyStringVector(const UVector& other) {
  UErrorCode errorCode = U_ZERO_ERROR;
  LocalPointer<UVector> adoptedKeys(new UVector(other.size(), errorCode));
  if (U_FAILURE(errorCode)) {
    return nullptr;
  }
  adoptedKeys->assign(other, &copyStrings, errorCode);
  if (U_FAILURE(errorCode)) {
    return nullptr;
  }
  return adoptedKeys.orphan();
}

class U_I18N_API MessageFormatDataModel : public UMemory {
    friend class MessageFormatter;

  public:

    class Pattern;
    class Reserved;
    class SelectorKeys;
    // Immutable list
    template<typename T>
    class List : public UMemory {
        // Provides a wrapper around a vector
      private:
          const LocalPointer<UVector> contents;

      public:
        size_t length() const { return (contents == nullptr ? 0 : contents->size()); }

        // Out-of-bounds is an internal error
        const T* get(size_t i) const {
            U_ASSERT(!(length() <= 0 || i >= length()));
            return static_cast<const T *>(contents->elementAt(i));
        }
        
        class Builder : public UMemory {
           // Provides a wrapper around a vector
         public:
           // adding adopts the thing being added
           Builder& add(T *element, UErrorCode errorCode) {
               if (U_FAILURE(errorCode)) {
                   return *this;
               }
               U_ASSERT(contents != nullptr);
               contents->adoptElement(element, errorCode);
               return *this;
           }
           List<T>* build(UErrorCode &errorCode) const {
               if (U_FAILURE(errorCode)) {
                   return nullptr;
               }
               LocalPointer<List<T>> adopted(buildList(*this, errorCode));
               if (U_FAILURE(errorCode)) {
                   return nullptr;
               }
               return adopted.orphan();
           }

         private:
           friend class List;

           LocalPointer<UVector> contents;

           // Creates an empty list
           Builder(UErrorCode& errorCode) {
               if (U_FAILURE(errorCode)) {
                   return;
               }
               // initialize `contents`
               contents.adoptInstead(new UVector(errorCode));
               if (U_FAILURE(errorCode)) {
                   return;
               }
               contents->setDeleter(uprv_deleteUObject);
           }
        };
        static Builder* builder(UErrorCode &errorCode) {
            if (U_FAILURE(errorCode)) {
                return nullptr;
            }
            LocalPointer<Builder> result(new Builder(errorCode));
            if (U_FAILURE(errorCode)) {
                return nullptr;
            }
            return result.orphan();
        }
     private:
        friend class Builder; // NOTE: Builder should only call buildList(); not the constructors
        friend class Pattern;
        friend class Reserved;
        friend class SelectorKeys;

        // Copies the contents of `builder`
        // This won't compile unless T is a type that has a copy assignment operator
        static List<T>* buildList(const Builder &builder, UErrorCode &errorCode) {
            if (U_FAILURE(errorCode)) {
                return nullptr;
            }
            List<T>* result;
            U_ASSERT(builder.contents != nullptr);

            LocalPointer<UVector> adoptedContents(new UVector(builder.contents->size(), errorCode));
            adoptedContents->assign(*builder.contents, &copyElements<T>, errorCode);
            if (U_FAILURE(errorCode)) {
                return nullptr;
            }
            result = new List<T>(adoptedContents.orphan());

            // Finally, check for null
            if (result == nullptr) {
                errorCode = U_MEMORY_ALLOCATION_ERROR;
            }
            return result;
        }

        static List<T>* copy(const List<T>& other) {
          UErrorCode errorCode = U_ZERO_ERROR;
          LocalPointer<UVector> adoptedContents(new UVector(other.length(), errorCode));
          adoptedContents->assign(*other.contents, &copyElements<T>, errorCode);
          if (U_FAILURE(errorCode)) {
            return nullptr;
          }
          return new List<T>(adoptedContents.orphan());
        }
        // Adopts `contents`
        List(UVector* things) : contents(things) {}
  };

    class VariantMap;
    // Immutable ordered map from strings to pointers to arbitrary values
    template<typename V>
    class OrderedMap : public UMemory {
      // Provides an immutable wrapper around a hash table.
      // Also stores the order in which keys were added.

    private:
      size_t size() const {
        return keys->size();
      }

    public:
      static constexpr size_t FIRST = 0;
      // Returns true if there are elements remaining
      bool next(size_t &pos, UnicodeString& k, V*& v) const {
        U_ASSERT(pos >= FIRST);
        if (pos >= size()) {
          return false;
        }
        k = *((UnicodeString*)keys->elementAt(pos));
        v = (V*) contents->get(k);
        pos = pos + 1;
        return true;
      }

      static OrderedMap<V>* copy(const OrderedMap<V>& other) {
          UErrorCode errorCode = U_ZERO_ERROR;
          LocalPointer<Hashtable> adoptedContents(copyHashtable<V>(*other.contents));
          LocalPointer<UVector> adoptedKeys(copyStringVector(*other.keys));

          if (!adoptedContents.isValid() || !adoptedKeys.isValid()) {
            return nullptr;
          }
          LocalPointer<OrderedMap<V>> result(
              OrderedMap<V>::create(adoptedContents.orphan(),
                                    adoptedKeys.orphan(),
                                    errorCode));
          
          if (U_FAILURE(errorCode)) {
            return nullptr;
          }
          return result.orphan();
      }
 
      class Builder : public UMemory {
      public:
        // Adopts `value`
        Builder& add(const UnicodeString& key, V* value, UErrorCode& errorCode) {
          if (U_FAILURE(errorCode)) {
            return *this;
          }
          // Copy `key` so it can be stored in the vector
          LocalPointer<UnicodeString> adoptedKey(new UnicodeString(key));
          if (!adoptedKey.isValid()) {
            return *this;
          }
          UnicodeString* k = adoptedKey.orphan();
          keys->adoptElement(k, errorCode);
          contents->put(key, value, errorCode);
          return *this;
        }

        // this should be a *copying* build() (leaves `this` valid)
        OrderedMap<V>* build(UErrorCode& errorCode) const {
          if (U_FAILURE(errorCode)) {
            return nullptr;
          }

          LocalPointer<Hashtable> adoptedContents(copyHashtable<V>(*contents));
          LocalPointer<UVector> adoptedKeys(copyStringVector(*keys));

          if (!adoptedContents.isValid() || !adoptedKeys.isValid()) {
            return nullptr;
          }
          LocalPointer<OrderedMap<V>> result(
              OrderedMap<V>::create(adoptedContents.orphan(),
                                    adoptedKeys.orphan(),
                                    errorCode));
          
          if (U_FAILURE(errorCode)) {
            return nullptr;
          }
          return result.orphan();
        }
        // TODO
        // could add a non-copying / moving build() that invalidates `this`
        // OrderedMap<V>* build(UErrorCode& errorCode);
    private:
        friend class OrderedMap;
        
      // Only called by builder()
      Builder(UErrorCode& errorCode) {
        // initialize `keys`
        keys.adoptInstead(new UVector(errorCode));
        if (U_FAILURE(errorCode)) {
          return;
        }
        keys->setDeleter(uprv_deleteUObject);
 
        // initialize `contents`
        // No value comparator needed
        contents.adoptInstead(new Hashtable(compareVariableName, nullptr, errorCode));
        if (U_FAILURE(errorCode)) {
          return;
        }
        // The `contents` hashtable owns the values, but does not own the keys
        contents->setValueDeleter(uprv_deleteUObject);
      }
        
      // Hashtable representing the underlying map
      LocalPointer<Hashtable> contents;
      // Maintain a list of keys that encodes the order in which
      // keys are added. This wastes some space, but allows us to
      // re-use ICU4C's Hashtable abstraction without re-implementing
      // an ordered version of it.
      LocalPointer<UVector> keys;
    }; // class OrderedMap<V>::Builder

      static Builder* builder(UErrorCode &errorCode) {
         if (U_FAILURE(errorCode)) {
                return nullptr;
            }
            LocalPointer<Builder> result(new Builder(errorCode));
            if (U_FAILURE(errorCode)) {
                return nullptr;
            }
            return result.orphan();
      }

    private:
      friend class VariantMap;

      static OrderedMap<V>* create(Hashtable* c, UVector* k, UErrorCode& errorCode) {
          if (U_FAILURE(errorCode)) {
            return nullptr;
          }
          OrderedMap<V>* result = new OrderedMap<V>(c, k);
          if (result == nullptr) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
          }
          return result;
      }
      OrderedMap<V>(Hashtable* c, UVector* k) : contents(c), keys(k) {
        // It would be an error if `c` and `k` had different sizes
        U_ASSERT(c->count() == k->size());
      }
      // Hashtable representing the underlying map
      const LocalPointer<Hashtable> contents;
      // List of keys
      const LocalPointer<UVector> keys;
    }; // class OrderedMap<V>

    class Key;
    class Operand;
    class Literal : public UObject {
  public:
  static Literal* copy(Literal l) {
    return new Literal(l.isQuoted, l.contents);
  }

  const bool isQuoted = false;
  const UnicodeString contents;

  Literal(bool q, const UnicodeString& s) : isQuoted(q), contents(s) {}

  private:
    friend class Key;
  
    // Makes it easier for new wildcard Keys to be initialized
    Literal() {}
};
    
class Operand : public UObject {
    // An operand can either be a variable reference or a literal.
    // There is a separate Literal class (which can be quoted or unquoted)
    // to make it easier to distinguish |x| from x when serializing the data model.
      public:
    // Variable
    static Operand* create(const VariableName& s, UErrorCode& errorCode) {
        if (U_FAILURE(errorCode)) {
            return nullptr;
        }
        Operand* result = new Operand(s);
        if (result == nullptr) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
        }
        return result;
    }

    // Literal
    static Operand* create(Literal lit, UErrorCode& errorCode) {
        if (U_FAILURE(errorCode)) {
            return nullptr;
        }
        Operand* result = new Operand(lit);
        if (result == nullptr) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
        }
        return result;
    }

  // TODO
  // Should be private
    static Operand* copy(const Operand& other) {
         return new Operand(other);
    }

    // Represent variable names as unquoted literals
    Operand(const VariableName& var) : isVariableReference(true), string(Literal(false, var)) {}
    Operand(const Literal& l) : isVariableReference(false), string(l) {}

    // copy constructor is used so that builders work properly -- see comment under copyElements()
    Operand(const Operand& other) : isVariableReference(other.isVariableReference), string(other.string) {}

    bool isVariable() const { return isVariableReference; }
    VariableName asVariable() const {
        U_ASSERT(isVariable());
        return string.contents;
    }
    bool isLiteral() const { return !isVariable(); }
    const Literal& asLiteral() const {
        U_ASSERT(isLiteral());
        return string;
    }
  private:

    const bool isVariableReference;
    const Literal string;
};

  class Key : public UObject {
  // A key is either a literal or the "wildcard" symbol.
  public:
    bool isWildcard() const { return wildcard; }
    // Precondition: !isWildcard()
    const Literal& asLiteral() const {
        U_ASSERT(!isWildcard());
        return contents;
    }
    Key(const Key& other) : wildcard(other.wildcard), contents(other.contents) {};

    static Key* create(UErrorCode& errorCode) {
        if (U_FAILURE(errorCode)) {
            return nullptr;
        }
        Key* k = new Key();
        if (k == nullptr) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
        }
        return k;
    }

    static Key* create(const Literal& lit, UErrorCode& errorCode) {
        if (U_FAILURE(errorCode)) {
            return nullptr;
        }
        Key* k = new Key(lit);
        if (k == nullptr) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
        }
        return k;
    }
    static Key* copy(const Key& other) {
      return new Key(other);
    }

  // TODO: maybe shouldn't be public
  void toString(UnicodeString& result) const {
    if (isWildcard()) {
      result += ASTERISK;
    }
    result += contents.contents;
  }
  private:
    // wildcard constructor
    Key() : wildcard(true) {}
    // concrete key constructor
    Key(const Literal& lit) : wildcard(false), contents(lit) {}
    const bool wildcard; // True if this represents the wildcard "*"
    const Literal contents;
};

  using KeyList = List<Key>;

    // Represents the left-hand side of a `when` clause
    class SelectorKeys : public UObject {
       public:
         const KeyList& getKeys() const;
         class Builder {
            private:
                friend class SelectorKeys;
                // prevent direct construction
                Builder(UErrorCode& errorCode) {
                    if (U_FAILURE(errorCode)) {
                        return;
                    }
                    keys.adoptInstead(KeyList::builder(errorCode));
                }
                LocalPointer<List<Key>::Builder> keys;
            public:
                Builder& add(Key* key, UErrorCode& errorCode);
                // TODO: is addAll() necessary?
                SelectorKeys* build(UErrorCode& errorCode) const;
         };
         static Builder* builder(UErrorCode& errorCode);
         static SelectorKeys* copy(const SelectorKeys& other) {
           KeyList* otherK = List<Key>::copy(*other.keys);
           if (otherK == nullptr) {
             return nullptr;
           }
           return new SelectorKeys(otherK);
         }
       private:
         const LocalPointer<KeyList> keys;
         // Adopts `keys`
         SelectorKeys(KeyList* ks) : keys(ks) {}
     };

    class Pattern;
    class VariantMap : public UMemory {
      // Wraps an OrderedMap<Pattern>
      // Has a different put() function since we want to stringify the key
      // list to hash it

    public:
      static constexpr size_t FIRST = OrderedMap<Pattern>::FIRST;
      bool next(size_t &pos, const SelectorKeys*& k, Pattern*& v) const {
        UnicodeString unused;
        if (!contents->next(pos, unused, v)) {
          return false;
        }
        k = keyLists->get(pos - 1);
        return true;
      }
      class Builder : public UMemory {
      public:
        Builder& add(SelectorKeys* key, Pattern* value, UErrorCode& errorCode) {
          if (U_FAILURE(errorCode)) {
            return *this;
          }
          // Stringify `key`
          UnicodeString keyResult;
          concatenateKeys(*key, keyResult);
          contents->add(keyResult, value, errorCode);
          keyLists->add(key, errorCode);
          return *this;
        }
        // this should be a *copying* build() (leaves `this` valid)
        VariantMap* build(UErrorCode& errorCode) const {
          if (U_FAILURE(errorCode)) {
            return nullptr;
          }

          LocalPointer<OrderedMap<Pattern>> adoptedContents(contents->build(errorCode));
          if (U_FAILURE(errorCode)) {
            return nullptr;
          }
          LocalPointer<List<SelectorKeys>> adoptedKeyLists(keyLists->build(errorCode));
          if (U_FAILURE(errorCode)) {
            return nullptr;
          }
          VariantMap* result = new VariantMap(adoptedContents.orphan(), adoptedKeyLists.orphan());
          if (result == nullptr) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
          }
          return result;
        }

     private:
        friend class VariantMap;

        static void concatenateKeys(const SelectorKeys& keys, UnicodeString& result) {
          const KeyList& ks = keys.getKeys();
          size_t len = ks.length();
          for (size_t i = 0; i < len; i++) {
            ks.get(i)->toString(result);
            if (i != len - 1) {
              result += SPACE;
            }
          }
        }

        // Only called by builder()
        Builder(UErrorCode& errorCode) {
            // initialize `contents`
            // No value comparator needed
            contents.adoptInstead(OrderedMap<Pattern>::builder(errorCode));
            // initialize `keyLists`
            keyLists.adoptInstead(List<SelectorKeys>::builder(errorCode));
            // `keyLists` does not adopt its elements
        }

        LocalPointer<OrderedMap<Pattern>::Builder> contents;
        LocalPointer<List<SelectorKeys>::Builder> keyLists;
    }; // class VariantMap::Builder

      static Builder* builder(UErrorCode& errorCode) {
        if (U_FAILURE(errorCode)) {
          return nullptr;
        }
        LocalPointer<Builder> result(new Builder(errorCode));
        if (U_FAILURE(errorCode)) {
          return nullptr;
        }
        return result.orphan();
      }
    private:
      friend class Builder;
      VariantMap(OrderedMap<Pattern>* vs, List<SelectorKeys>* ks) : contents(vs), keyLists(ks) {
        // it would be an error for `vs` and `ks` to have different sizes
        U_ASSERT(vs->contents->count() == ((int32_t) ks->length()));
      }
      const LocalPointer<OrderedMap<Pattern>> contents;
      // TODO: this is really annoying
      // however, it will be in the same order as the contents of the OrderedMap
      const LocalPointer<List<SelectorKeys>> keyLists;
}; // class VariantMap
    
  using OptionMap = OrderedMap<Operand>;

  /*
Note: all the static copy() methods are because UVector and Hashtable don't have copy constructors
TODO
   */
 public:

    class Operator;
    // TODO: maybe this should be private? actually having a reserved string would be an error;
    // this is there for testing purposes

    // Corresponds to `reserved` in the grammar
    // Represent the structure implicitly to make it easier to serialize correctly
    class Reserved : public UMemory {
    public:
        size_t numParts() const { return parts->length(); }
        // Precondition: i < numParts()
        const Literal* getPart(size_t i) const {
            U_ASSERT(i < numParts());
            return parts->get(i);
        }
        class Builder {
          private:
            friend class Reserved;
  
            Builder(UErrorCode &errorCode) {
                if (U_FAILURE(errorCode)) {
                    return;
                }
                parts.adoptInstead(List<Literal>::builder(errorCode));
            }
            LocalPointer<List<Literal>::Builder> parts;

          public:
            Builder& add(Literal part, UErrorCode &errorCode);
            // TODO: is addAll() necessary?
            Reserved *build(UErrorCode &errorCode) const;
        };

        static Builder *builder(UErrorCode &errorCode);

      static Reserved* copy(const Reserved& other) {
        List<Literal>* otherParts = List<Literal>::copy(*other.parts);
        if (otherParts == nullptr) {
          return nullptr;
        }
        return new Reserved(otherParts);
      }
      private:
        friend class Operator;

        // Possibly-empty list of parts
        // `literal` reserved as a quoted literal; `reserved-char` / `reserved-escape`
        // strings represented as unquoted literals
        const LocalPointer<List<Literal>> parts;

        // Can only be called by Builder
        // Takes ownership of `ps`
        Reserved(List<Literal> *ps) : parts(ps) { U_ASSERT(ps != nullptr); }
    };

      // TODO: This class should really be private. left public for the convenience of the parser
  class Operator : public UMemory {
         // An operator represents either a function name together with
         // a list of options, which may be empty;
         // or a reserved sequence (which has no meaning and may result
         // in a formatting error).
       public:

    /*
         // copy constructor is used so that builders work properly -- see comment under copyElements()
         Operator(const Operator &other)
             : isReservedSequence(other.isReservedSequence), functionName(other.functionName),
               options(other.options == nullptr ? nullptr : new OptionMap(*other.options)),
               reserved(other.reserved == nullptr ? nullptr : new Reserved(*other.reserved)) {}
    */
         const FunctionName& getFunctionName() const {
             U_ASSERT(!isReserved());
             return functionName;
         }
         const Reserved& asReserved() const {
             U_ASSERT(isReserved());
             return *reserved;
         }
         const OptionMap &getOptions() const {
             U_ASSERT(!isReserved());
             return *options;
         }
         bool isReserved() const { return isReservedSequence; }

         static Operator* create(Reserved* r, UErrorCode& errorCode) {
             if (U_FAILURE(errorCode)) {
                 return nullptr;
             }
             Operator* result = new Operator(r);
             if (result == nullptr) {
                 errorCode = U_MEMORY_ALLOCATION_ERROR;
             }
             return result;
         }

         static Operator* create(FunctionName f, OptionMap* l, UErrorCode& errorCode) {
             if (U_FAILURE(errorCode)) {
                 return nullptr;
             }
             LocalPointer<OptionMap> adoptedOptions(l);
             U_ASSERT(adoptedOptions.isValid());
             Operator* result = new Operator(f, adoptedOptions.orphan());
             if (result == nullptr) {
                 errorCode = U_MEMORY_ALLOCATION_ERROR;
             }
             return result;
         }

         static Operator* copy(const Operator& other) {
           if (other.isReservedSequence) {
             Reserved* otherReserved = Reserved::copy(*other.reserved);
             if (otherReserved == nullptr) {
               return nullptr;
             }
             return new Operator(otherReserved);
           }
           OptionMap* otherOptions = OptionMap::copy(*other.options);
           if (otherOptions == nullptr) {
             return nullptr;
           }
           return new Operator(other.functionName, otherOptions);
         }
       private:
         // Function call constructor; adopts `l`
         Operator(FunctionName f, OptionMap *l)
           : isReservedSequence(false), functionName(f), options(l), reserved(nullptr) {}

         // Reserved sequence constructor
         Operator(Reserved* r) : isReservedSequence(true), functionName(""), options(nullptr), reserved(r) {}

         const bool isReservedSequence;
         const FunctionName functionName;
         const LocalPointer<OptionMap> options;
         const LocalPointer<Reserved> reserved;
     };

  
    class Expression : public UObject {
        /*
          An expression is the application of an optional operator to an optional operand.
          For example (using a made-up quasi-s-expression notation):

          { |42| :fun opt=value } => Expression(operator=Some(fun, {opt: value}),
                                                operand=Some(Literal(42)))
          abcd                    => Expression(operator=None, operand=Some(String("abcd")))
          { : fun opt=value }     => Expression(operator=Some(fun, {opt: value}),
                                                operand=None)

          An expression where both operand and operator are None can't be constructed.
        */
      public:

      static Expression* copy(const Expression& other) {
        if (other.rator.isValid() && other.rand.isValid()) {
          LocalPointer<Operator> otherRator(Operator::copy(*other.rator));
          LocalPointer<Operand> otherRand(Operand::copy(*other.rand));
          if (!otherRator.isValid() || !otherRand.isValid()) {
            return nullptr;
          }
          return new Expression(otherRator.orphan(), otherRand.orphan());
        }
        if (other.rator.isValid()) {
          LocalPointer<Operator> otherRator(Operator::copy(*other.rator));
          if (!otherRator.isValid()) {
            return nullptr;
          }
          return new Expression(otherRator.orphan());
        }
        U_ASSERT(other.rand.isValid());
        LocalPointer<Operand> otherRand(Operand::copy(*other.rand));
        if (!otherRand.isValid()) {
          return nullptr;
        }
        return new Expression(otherRand.orphan());
      }
        // copy constructor is used so that builders work properly -- see comment under copyElements()
      //        Expression(const Expression &other) { *this = other; }

        // The following three methods should be private and the parser should be a friend class,
        // once the parser becomes its own class
        // TODO

      /*
        const Expression &operator=(const Expression &other) {
            if (other.rator == nullptr) {
                rator = nullptr;
            } else {
                rator = new Operator(*other.rator);
            }
            if (other.rand == nullptr) {
                rand = nullptr;
            } else {
                rand = new Operand(*other.rand);
            }
            return *this;
        }
      */
        // TODO: include these or not?
        bool isStandaloneAnnotation() const { return (rand == nullptr); }
        // Returns true for function calls with operands as well as
        // standalone annotations.
        // Reserved sequences are not function calls
        bool isFunctionCall() const { return (rator != nullptr && !rator->isReserved()); }
        bool isReserved() const { return (rator != nullptr && rator->isReserved()); }

        const Operand& getOperand() const {
            U_ASSERT(rand != nullptr);
            return *rand;
        }
 
        const UnicodeString& getFunctionName() const {
            U_ASSERT(isFunctionCall());
            return rator->getFunctionName();
        }

        const Reserved& asReserved() const {
          U_ASSERT(isReserved());
          return rator->asReserved();
        }

        const OptionMap& getOptions() const {
            U_ASSERT(isFunctionCall());
            return rator->getOptions();
        }

        /*
          TODO: I thought about using an Optional class here instead of nullable
          pointers. But that doesn't really work since we can't use the STL and therefore
          can't use `std::move` and therefore can't use move constructors/move assignment
          operators.
         */

        class Builder {
          private:
            Builder() {} // prevent direct construction
          public:
            Builder& setOperand(Operand operand);
            Builder& setFunctionName(const UnicodeString &functionName);
            Builder& addOption(const UnicodeString &key, Expression *value);
            Expression *build(UErrorCode& errorCode) const;
        };

    private:
      friend class MessageFormatter;
 
      static Expression* create(Operand *rAnd, UErrorCode &errorCode) {
            if (U_FAILURE(errorCode)) {
                return nullptr;
            }
            Expression* result = new Expression(rAnd);
            if (result == nullptr) {
                errorCode = U_MEMORY_ALLOCATION_ERROR;
            }
            return result;
        }

        static Expression* create(Operator *rAtor, UErrorCode &errorCode) {
            if (U_FAILURE(errorCode)) {
                return nullptr;
            }
            Expression* result = new Expression(rAtor);
            if (result == nullptr) {
                errorCode = U_MEMORY_ALLOCATION_ERROR;
            }
            return result;
        }

        static Expression* create(Operator *rAtor, Operand *rAnd, UErrorCode &errorCode) {
            if (U_FAILURE(errorCode)) {
                return nullptr;
            }
            Expression* result = new Expression(rAtor, rAnd);
            if (result == nullptr) {
                errorCode = U_MEMORY_ALLOCATION_ERROR;
            }
            return result;
        }

        // All constructors adopt their arguments
        // Both operator and operands must be non-null
        Expression(Operator *rAtor, Operand *rAnd) : rator(rAtor), rand(rAnd) {
            U_ASSERT(rAtor != nullptr && rAnd != nullptr);
        }

        // Operand must be non-null
        Expression(Operand *rAnd) : rator(nullptr), rand(rAnd) { U_ASSERT(rAnd != nullptr); }

        // Operator must be non-null
        Expression(Operator *rAtor) : rator(rAtor), rand(nullptr) { U_ASSERT(rAtor != nullptr); }

        const LocalPointer<Operator> rator;
        const LocalPointer<Operand> rand;
    };

    using ExpressionList = List<Expression>;
    // TODO: anything stored in a UVector or Hashtable should be a UObject?
    class PatternPart : public UObject {
      public:
        static PatternPart* create(const UnicodeString& t, UErrorCode& errorCode) {
          if (U_FAILURE(errorCode)) {
            return nullptr;
          }
          PatternPart* result = new PatternPart(t);
          if (result == nullptr) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
          }
          return result;
        }
        // Takes ownership of `e`
        static PatternPart* create(Expression* e, UErrorCode& errorCode) {
            if (U_FAILURE(errorCode)) {
                return nullptr;
            }
            U_ASSERT(e != nullptr);
            LocalPointer<Expression> adoptedExpr(e);
            PatternPart* result = new PatternPart(adoptedExpr.orphan());
            if (result == nullptr) {
                errorCode = U_MEMORY_ALLOCATION_ERROR;
            }
            return result;
        }
      // TODO: Should be private
        static PatternPart* copy(const PatternPart& other) {
          if (other.isText()) {
            return new PatternPart(other.asText());
          }
          Expression* otherExpr = Expression::copy(other.contents());
          if (otherExpr == nullptr) {
            return nullptr;
          }
          return new PatternPart(otherExpr);
        }
        bool isText() const { return isRawText; }
        // Precondition: !isText()
        const Expression& contents() const {
          U_ASSERT(!isText());
          return *expression;
        }
        // Precondition: isText();
        const UnicodeString& asText() const {
            U_ASSERT(isText());
            return text;
        }
      private:

      // Text
      PatternPart(const UnicodeString& t) : isRawText(true), text(t), expression(nullptr) {}
      // Expression
      PatternPart(Expression* e) : isRawText(false), expression(e) {}

        // Either Expression or TextPart can show up in a pattern
        // This class exists so Text can be distinguished from Expression
        // when serializing a Pattern
        const bool isRawText;
        // Not used if !isRawText
        const UnicodeString text;
        // null if isRawText
        const LocalPointer<Expression> expression;
    };

    class Pattern : public UObject {
      public:
        size_t numParts() const { return parts->length(); }
        // Precondition: i < numParts()
        const PatternPart* getPart(size_t i) const {
            U_ASSERT(i < numParts());
            return parts->get(i);
        }

        class Builder {
          private:
            friend class Pattern;

            Builder(UErrorCode &errorCode) {
                if (U_FAILURE(errorCode)) {
                    return;
                }
                parts.adoptInstead(List<PatternPart>::builder(errorCode));
            }
            LocalPointer<List<PatternPart>::Builder> parts;

          public:
            // Takes ownership of `part`
            Builder& add(PatternPart *part, UErrorCode &errorCode);
            // TODO: is addAll() necessary?
            Pattern *build(UErrorCode &errorCode);
        };

        static Builder *builder(UErrorCode &errorCode);

      // TODO: Should be private    
        static Pattern* copy(const Pattern& other) {
          LocalPointer<List<PatternPart>> parts(List<PatternPart>::copy(*other.parts));
          if (!parts.isValid()) {
            return nullptr;
          }
          return new Pattern(parts.orphan());
        }
  
      private:
        // Possibly-empty list of parts
        const LocalPointer<List<PatternPart>> parts;

        // Can only be called by Builder
        // Takes ownership of `ps`
        Pattern(List<PatternPart> *ps) : parts(ps) { U_ASSERT(ps != nullptr); }
    };

     class Binding {
     public:
       static Binding* create(const UnicodeString& var, Expression* e, UErrorCode& errorCode) {
         if (U_FAILURE(errorCode)) {
           return nullptr;
         }
         Binding *b = new Binding(var, e);
         if (b == nullptr) {
           errorCode = U_MEMORY_ALLOCATION_ERROR;
         }
         return b;
       }
       static Binding* copy(const Binding& b) {
         Expression* otherValue = Expression::copy(*b.value);
         if (otherValue == nullptr) {
           return nullptr;
         }
         return new Binding(b.var, otherValue);
       }
       Binding(const UnicodeString& v, Expression* e) : var(v), value(e) {}
       const UnicodeString var;
       const LocalPointer<Expression> value;
     };
     using Bindings = List<Binding>;
     const Bindings& getLocalVariables() const {
         return *bindings;
     }
     ExpressionList& getSelectors() const;
     VariantMap& getVariants() const;
     Pattern& getPattern() const;

     class Builder {
       private:
         friend class MessageFormatDataModel;

         // prevent direct construction
         Builder(UErrorCode& errorCode) {
             if (U_FAILURE(errorCode)) {
                 return;
             }
             selectors.adoptInstead(ExpressionList::builder(errorCode));
             variants.adoptInstead(VariantMap::builder(errorCode));
             locals.adoptInstead(Bindings::builder(errorCode));
         }

         // Invalidate pattern and create selectors/variants if necessary
         void buildSelectorsMessage(UErrorCode& errorCode) {
             if (U_FAILURE(errorCode)) {
                 return;
             }
             if (pattern.isValid()) {
                 pattern.adoptInstead(nullptr);
             }
             if (!selectors.isValid()) {
                 U_ASSERT(!variants.isValid());
                 selectors.adoptInstead(ExpressionList::builder(errorCode));
                 variants.adoptInstead(VariantMap::builder(errorCode));
             } else {
                 U_ASSERT(variants.isValid());
             }
         }
       
         LocalPointer<Pattern> pattern;
         LocalPointer<ExpressionList::Builder> selectors;
         LocalPointer<VariantMap::Builder> variants;
         LocalPointer<Bindings::Builder> locals;

       public:
         // Takes ownership of `expression`
         Builder& addLocalVariable(const UnicodeString& variableName, Expression* expression, UErrorCode &errorCode);
         // No addLocalVariables() yet
         // Takes ownership
         Builder& addSelector(Expression* selector, UErrorCode& errorCode);
         // No addSelectors() yet
         // Takes ownership
         Builder& addVariant(SelectorKeys* keys, Pattern* pattern, UErrorCode& errorCode);
         Builder& setPattern(Pattern* pattern);
         MessageFormatDataModel* build(UErrorCode& errorCode);
     };

     static Builder* builder(UErrorCode& errorCode);

     // TODO more comments
     // Converts a data model back to a string
     void serialize(UnicodeString& result) const {
         Serializer serializer(*this, result);
         serializer.serialize();
     };
     
     virtual ~MessageFormatDataModel();

  private:
     
     // Converts a data model back to a string
     class Serializer : UMemory {
       public:
         Serializer(const MessageFormatDataModel& m, UnicodeString& s) : dataModel(m), result(s) {}
         void serialize();

         const MessageFormatDataModel& dataModel;
         UnicodeString& result;

       private:
         void whitespace();
         void emit(UChar32);
         template <size_t N>
         void emit(const UChar32 (&)[N]);
         void emit(const UnicodeString&);
         void emit(const Literal&);
         void emit(const Key&);
         void emit(const SelectorKeys&);
         void emit(const Operand&);
         void emit(const Expression&);
         void emit(const PatternPart&);
         void emit(const Pattern&);
         void emit(const VariantMap&);
         void emit(const OptionMap&);
         void serializeDeclarations();
         void serializeSelectors();
         void serializeVariants();
     };

     friend class Serializer;

     class MessageBody : public UMemory {
       public:
         // Constructs a body out of a list of scrutinees (expressions) and
         // a list of variants, which represents the `(selectors 1*([s] variant))`
         // alternative in the grammar
         // Adopts its arguments; takes an error code for consistency
         MessageBody(ExpressionList *es, VariantMap *vs, UErrorCode& errorCode) {
             if (U_FAILURE(errorCode)) {
                 return;
             }
             U_ASSERT(es != nullptr && vs != nullptr);
             scrutinees.adoptInstead(es);
             variants.adoptInstead(vs);
             pattern.adoptInstead(nullptr);
         }

         // Constructs a body out of a single Pattern
         // (body -> pattern alternative in the grammar)
         // a `pattern` in the grammar
         // This copies the pattern, as it's called by the builder, which
         // is non-destructive
         MessageBody(const Pattern &pat, UErrorCode &errorCode) {
             if (U_FAILURE(errorCode)) {
               return;
             }
             scrutinees = nullptr;
             variants = nullptr;
             pattern.adoptInstead(Pattern::copy(pat));
             if (!pattern.isValid()) {
               errorCode = U_MEMORY_ALLOCATION_ERROR;
             }
         }
       private:
         friend class Serializer;
         /*
           A message body is a `selectors` construct as in the grammar.
           A bare pattern is represented as a `selectors` with no scrutinees
           and a single `when`-clause with empty keys.
          */

         // The expressions that are being matched on.
         // Null iff this is a `pattern` message.
         LocalPointer<ExpressionList> scrutinees;

         // The list of `when` clauses (case arms).
         // Null iff this is a `pattern` message.
         LocalPointer<VariantMap> variants;

         // The pattern forming the body of the message.
         // If this is non-null, then `variants` and `scrutinees` must be null.
         LocalPointer<Pattern> pattern;
     };

    /*
      A parsed message consists of an environment and a body.
      Initially, the environment contains bindings for local variables
      (those declared with `let` in the message). API calls can extend
      the environment with new bindings or change the values of existing ones.

      Once the data model is constructed, only the environment can be mutated.
      (It's constructed bottom-up.)
    */
    LocalPointer<Bindings> bindings;

    /*
      See the `MessageBody` class.
     */
    LocalPointer<MessageBody> body;

    // Normalized version of the input string (optional whitespace omitted)
    // Used for testing purposes
    LocalPointer<UnicodeString> normalizedInput;

    // Do not define default assignment operator
    const MessageFormatDataModel &operator=(const MessageFormatDataModel &) = delete;

    // This *copies* the contents of `builder`, so that it can be re-used / mutated
    // while preserving the immutability of this data model
    // TODO: add tests for this
    MessageFormatDataModel(const Builder& builder, UErrorCode &status);
}; // class MessageFormatDataModel

} // namespace message2
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT_DATA_MODEL_H

#endif // U_HIDE_DEPRECATED_API
// eof
