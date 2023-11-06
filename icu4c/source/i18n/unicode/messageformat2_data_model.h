// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT_DATA_MODEL_H
#define MESSAGEFORMAT_DATA_MODEL_H

#include "unicode/utypes.h"

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#include "unicode/fmtable.h"
#include "unicode/messageformat2_utils.h"
#include "unicode/unistr.h"
#include "messageformat2_data_model_forward_decls.h"

U_NAMESPACE_BEGIN namespace message2 {

    /**
     * The `VariableName` class represents the name of a variable in a message.
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API MessageFormatDataModel::VariableName : public UObject {
    public:
        /**
         * Equality comparison.
         *
         * @param other    the object to be compared with.
         * @return        true if other is equal to this, false otherwise.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        inline bool operator== (const VariableName& other) const { return other.variableName == variableName; }
        /**
         * Constructor.
         *
         * @param s   The variable name, as a string
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        VariableName(const UnicodeString& s) : variableName(s) {}
        /**
         * Default constructor. (Needed for representing null operands)
         *
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        VariableName() {}
        /**
         * Returns the name of this variable, as a string.
         *
         * @return        Reference to the variable's name
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const UnicodeString& identifier() const { return variableName; }
        /**
         * Returns the name of this variable, as a string prefixed by the
         * variable name sigil ('$')
         *
         * @return        String representation of the variable as it appears in a declaration
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
         UnicodeString declaration() const;
         /**
          * Destructor.
          *
          * @internal ICU 74.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         virtual ~VariableName();
    private:
        const UnicodeString variableName;
    }; // class VariableName

    /**
     * The `FunctionName` class represents the name of a function referred to
     * in a message.
     *
     * It corresponds to the `FunctionRef` interface defined in
     * https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#expressions
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API MessageFormatDataModel::FunctionName : public UMemory {
    public:
        /**
         * Type representing the function's kind, which is either ':' (the default)
         * or "open" ('+')/"close" ('-'), usually used for markup functions.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
         enum Sigil {
             OPEN,
             CLOSE,
             DEFAULT
         };
         /**
          * Converts the function name to a string that includes the sigil.
          *
          * @return A string beginning with the sigil, followed by the function's name.
          *
          * @internal ICU 74.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         UnicodeString toString() const;
         /**
          * Constructor.
          *
          * @param s   The function name, as a string. Constructs a function name with the default sigil.
          *
          * @internal ICU 74.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         FunctionName(UnicodeString s) : functionName(s), functionSigil(Sigil::DEFAULT) {}
         /**
          * Constructor.
          *
          * @param n   The function name, as a string.
          * @param s   The function sigil to use.
          *
          * @internal ICU 74.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         FunctionName(UnicodeString n, Sigil s) : functionName(n), functionSigil(s) {}
         /**
          * Copy constructor.
          *
          * @param other   The function name to copy.
          *
          * @internal ICU 74.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         FunctionName(const FunctionName& other) : functionName(other.functionName), functionSigil(other.functionSigil) {}
         /**
          * Destructor.
          *
          * @internal ICU 74.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         virtual ~FunctionName();

    private:
         const UnicodeString functionName;
         const Sigil functionSigil;

         UChar sigilChar() const;
    }; // class FunctionName

    /**
     * The `Literal` class corresponds to the `literal` nonterminal in the MessageFormat 2 grammar,
     * https://github.com/unicode-org/message-format-wg/blob/main/spec/message.abnf and the
     * `Literal` interface defined in
     *   // https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#expressions
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API MessageFormatDataModel::Literal : public UObject {
    public:
        /**
         * Returns the quoted representation of this literal (enclosed in '|' characters)
         *
         * @return A string representation of the literal enclosed in quote characters.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
         UnicodeString quotedString() const;
         /**
          * Returns the parsed string contents of this literal.
          *
          * @return A reference to a Formattable whose string contents are
          *         the parsed string contents of this literal.
          *
          * @internal ICU 74.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         const Formattable& getContents() const { return contents; }
         /**
          * Returns the parsed string contents of this literal.
          *
          * @return A string representation of this literal.
          *
          * @internal ICU 74.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         const UnicodeString& stringContents() const;
         /**
          * Determines if this literal appeared as a quoted literal in the message.
          *
          * @return true if and only if this literal appeared as a quoted literal in the
          *         message.
          *
          * @internal ICU 74.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         UBool quoted() const { return isQuoted; }
         /**
          * Literal constructor.
          *
          *  @param q True if and only if this literal was parsed with the `quoted` nonterminal
          *           (appeared enclosed in '|' characters in the message text).
          *  @param s The string contents of this literal; escape sequences are assumed to have
          *           been interpreted already.
          *
          * @internal ICU 74.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         Literal(UBool q, const UnicodeString& s) : isQuoted(q), contents(s) {}
         /**
          * Destructor.
          *
          * @internal ICU 74.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         virtual ~Literal();

    private:
        friend class Key;
        friend class ImmutableVector<Literal>;
        friend class Operand;
        friend class Reserved;

        Literal(const Literal& other) : isQuoted(other.isQuoted), contents(other.contents) {}

        const bool isQuoted = false;
        // Contents is stored as a Formattable to avoid allocating
        // new Formattables during formatting, but it's guaranteed
        // to be a string
        const Formattable contents;
        // Because Key uses `Literal` as its underlying representation,
        // this provides a default constructor for wildcard keys
        Literal() {}
    };

    /**
     * The `Operand` class corresponds to the `operand` nonterminal in the MessageFormat 2 grammar,
     * https://github.com/unicode-org/message-format-wg/blob/main/spec/message.abnf .
     * It represents a `Literal | VariableRef` -- see the `operand?` field of the `FunctionRef`
     * interface defined at:
     * https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#expressions
     * with the difference that it can also represent a null operand (the absent operand in an
     * `annotation` with no operand).
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API MessageFormatDataModel::Operand : public UObject {
    public:
        /**
         * Creates a new `variable` operand.
         *
         * @param var       The variable name this operand represents.
         * @param status    Input/output error code.
         * @return The new operand, guaranteed to be non-null if U_SUCCESS(status)
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        static Operand* create(const VariableName& var, UErrorCode& status);
        /**
         * Creates a new `literal` operand.
         *
         * @param lit       The literal this operand represents.
         * @param status    Input/output error code.
         * @return The new operand, guaranteed to be non-null if U_SUCCESS(status)
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        static Operand* create(const Literal& lit, UErrorCode& status);
        /**
         * Creates a new `null` operand, which should only appear when
         * representing the following production in the grammar:
         * expression = "{" [s] annotation [s] "}"
         *
         * @param status    Input/output error code.
         * @return The new operand, guaranteed to be non-null if U_SUCCESS(status)
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        static Operand* create(UErrorCode& status);
        /**
         * Determines if this operand represents a variable.
         *
         * @return True if and only if the operand is a variable.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UBool isVariable() const;
        /**
         * Determines if this operand represents a literal.
         *
         * @return True if and only if the operand is a literal.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UBool isLiteral() const;
        /**
         * Determines if this operand is the null operand.
         *
         * @return True if and only if the operand is the null operand.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UBool isNull() const;
        /**
         * Returns a reference to this operand's variable name.
         * Precondition: isVariable()
         *
         * @return A reference to the name of the variable
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const VariableName& asVariable() const;
        /**
         * Returns a reference to this operand's literal contents.
         * Precondition: isLiteral()
         *
         * @return A reference to the literal
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const Literal& asLiteral() const;
        /**
         * Destructor.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        virtual ~Operand();
    private:
        friend class Expression;
        friend class OrderedMap<Operand>;

        enum Type {
            VARIABLE,
            LITERAL,
            NULL_OPERAND
        };
        // This wastes some space, but it's simpler than defining a copy
        // constructor for a union
        const VariableName var;
        const Literal lit;
        const Type type;
        Operand(const Operand&);
        Operand() : type(Type::NULL_OPERAND) {}

        Operand(const VariableName& v) : var(v), type(Type::VARIABLE) {}
        Operand(const Literal& l) : lit(l), type(Type::LITERAL) {}
    }; // class Operand

    /**
     * The `Key` class corresponds to the `key` nonterminal in the MessageFormat 2 grammar,
     * https://github.com/unicode-org/message-format-wg/blob/main/spec/message.abnf .
     * It also corresponds to
     * the `Literal | CatchallKey` that is the
     * element type of the `keys` array in the `Variant` interface
     * defined in https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#messages
     *
     * A key is either a literal or the wildcard symbol (represented in messages as '*')
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API MessageFormatDataModel::Key : public UObject {
    public:
        /**
        * Determines if this is a wildcard key
        *
        * @return True if and only if this is the wildcard key
        *
        * @internal ICU 74.0 technology preview
        * @deprecated This API is for technology preview only.
        */
        UBool isWildcard() const { return wildcard; }
        /**
         * Returns the contents of this key as a literal.
         * Precondition: !isWildcard()
         *
         * @return The literal contents of the key
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const Literal& asLiteral() const;
        /**
         * Creates a new wildcard key.
         *
         * @param status    Input/output error code.
         * @return The new key, which is non-null if U_SUCCESS(status).
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        static Key* create(UErrorCode& status);
        /**
         * Creates a new literal key.
         *
         * @param lit       The literal that this key matches with.
         * @param status    Input/output error code.
         * @return The new key, which is non-null if U_SUCCESS(status).
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        static Key* create(const Literal& lit, UErrorCode& status);

    private:
        friend class ImmutableVector<Key>;
        friend class VariantMap;

        Key(const Key& other) : wildcard(other.wildcard), contents(other.contents) {}
        void toString(UnicodeString& result) const;
    
        // Wildcard constructor
        Key() : wildcard(true) {}
        // Concrete key constructor
        Key(const Literal& lit) : wildcard(false), contents(lit) {}
        const bool wildcard; // True if this represents the wildcard "*"
        const Literal contents;
    }; // class Key

    /**
     * The `SelectorKeys` class represents the key list for a single variant.
     * It corresponds to the `keys` array in the `Variant` interface
     * defined in https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#messages
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API MessageFormatDataModel::SelectorKeys : public UObject {
    public:
        /**
         * Returns the underlying list of keys.
         *
         * @return A reference to the list of keys for this variant.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const KeyList& getKeys() const;
        /**
         * The mutable `SelectorKeys::Builder` class allows the key list to be constructed
         * one key at a time.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        class U_I18N_API Builder : public UMemory {
        private:
            friend class SelectorKeys;
            Builder(UErrorCode&);
            LocalPointer<ImmutableVector<Key>::Builder> keys;
        public:
            /**
             * Adds a single key to the list. Adopts `key`.
             *
             * @param key The key to be added.
             * @param status Input/output error code.
             * @return A reference to the builder.
             *
             * @internal ICU 74.0 technology preview
             * @deprecated This API is for technology preview only.
             */
             Builder& add(Key* key, UErrorCode& status);
             /**
              * Constructs a new immutable `SelectorKeys` using the list of keys
              * set with previous `add()` calls.
              *
              * The builder object (`this`) can still be used after calling `build()`.
              *
              * @param status    Input/output error code.
              * @return          The new SelectorKeys object, which is non-null if
              *                  U_SUCCESS(status).
              *
              * @internal ICU 74.0 technology preview
              * @deprecated This API is for technology preview only.
              */
             SelectorKeys* build(UErrorCode& status) const;
             /**
              * Destructor.
              *
              * @internal ICU 74.0 technology preview
              * @deprecated This API is for technology preview only.
              */
             virtual ~Builder();
        }; // class SelectorKeys::Builder
        /**
         * Returns a new `SelectorKeys::Builder` object.
         *
         * @param status  Input/output error code.
         * @return The new Builder object, which is non-null if U_SUCCESS(status).
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        static Builder* builder(UErrorCode& status);

    private:
        friend class ImmutableVector<SelectorKeys>;
        friend class VariantMap;

        SelectorKeys(const SelectorKeys& other);

        const LocalPointer<KeyList> keys;
        bool isBogus() const { return !keys.isValid(); }
        // Adopts `keys`
        SelectorKeys(KeyList* ks) : keys(ks) {}
    }; // class SelectorKeys

    /**
     * The `VariantMap` class represents the set of all variants in a message that has selectors,
     * relating `SelectorKeys` objects to `Pattern` objects,
     * following  the `variant` production in the grammar:
     *
     * variant = when 1*(s key) [s] pattern
     *
     * https://github.com/unicode-org/message-format-wg/blob/main/spec/message.abnf#L9
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API MessageFormatDataModel::VariantMap : public UMemory {
    public:
        /**
         * The initial iterator position to be used with `next()`.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        static constexpr int32_t FIRST = OrderedMap<Pattern>::FIRST;
        /**
         * Iterates over all variants. The order in which variants are returned is unspecified.
         *
         * @param pos A mutable reference to the current iterator position. Should be set to
         *            `FIRST` before the first call to `next()`.
         * @param k   A mutable reference to a const pointer to a SelectorKeys object,
         *            representing the key list for a single variant.
         *            If the return value is true, then `k` refers to a non-null pointer.
         * @param v   A mutable reference to a const pointer to a Pattern object,
         *            representing the pattern of a single variant.
         *            If the return value is true, then `v` refers to a non-null pointer.
         * @return    True if and only if there are no further options after `pos`.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UBool next(int32_t &pos, const SelectorKeys*& k, const Pattern*& v) const;
        /**
         * Returns the number of variants.
         *
         * @return The size of this VariantMap.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        int32_t size() const;
        /**
         * The mutable `VariantMap::Builder` class allows the variant map to be
         * constructed one variant at a time.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        class U_I18N_API Builder : public UMemory {
        public:
            /**
             * Adds a single variant to the map. Adopts `key` and `value`.
             *
             * @param key The key list for this variant.
             * @param value The pattern for this variant.
             * @param status Input/output error code.
             * @return A reference to the builder.
             *
             * @internal ICU 74.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& add(SelectorKeys* key, Pattern* value, UErrorCode& status);
            /**
             * Constructs a new immutable `VariantMap` using the variants
             * added with previous `add()` calls.
             *
             * The builder object (`this`) can still be used after calling `build()`.
             *
             * @param status    Input/output error code.
             * @return          The new VariantMap, which is non-null if
             *                  U_SUCCESS(status).
             *
             * @internal ICU 74.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            VariantMap* build(UErrorCode& status) const;
             /**
              * Destructor.
              *
              * @internal ICU 74.0 technology preview
              * @deprecated This API is for technology preview only.
              */
             virtual ~Builder();
        private:
            friend class VariantMap;
          
            static void concatenateKeys(const SelectorKeys& keys, UnicodeString& result);
            Builder(UErrorCode& errorCode);
            LocalPointer<OrderedMap<Pattern>::Builder> contents;
            LocalPointer<ImmutableVector<SelectorKeys>::Builder> keyLists;
        }; // class VariantMap::Builder

        /**
         * Returns a new `VariantMap::Builder` object.
         *
         * @param status  Input/output error code.
         * @return        The new builder, which is non-null if U_SUCCESS(status).
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        static Builder* builder(UErrorCode& status);
    private:
    /*
      Internally, the map uses the `SelectorKeys` as its key, and the `pattern` as the value.

      This representation mirrors the ICU4J API:
      public OrderedMap<SelectorKeys, Pattern> getVariants();

      Since the `OrderedMap` class defined above is not polymorphic on its key
      values, `VariantMap` is defined as a separate data type that wraps an
      `OrderedMap<Pattern>`.
      The `VariantMap::Builder::add()` method encodes its `SelectorKeys` as
      a string, and the VariantMap::next() method decodes it.
    */
        friend class Builder;
        VariantMap(OrderedMap<Pattern>* vs, ImmutableVector<SelectorKeys>* ks);
        const LocalPointer<OrderedMap<Pattern>> contents;
        // See the method implementations for comments on
        // how `keyLists` is used.
        const LocalPointer<ImmutableVector<SelectorKeys>> keyLists;
    }; // class VariantMap

    /**
     * The `Reserved` class represents a `reserved` annotation, as in the `reserved` nonterminal
     * in the MessageFormat 2 grammar or the `Reserved` interface
     * defined in
     * https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#expressions
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class MessageFormatDataModel::Reserved : public UMemory {
    public:
        /**
         * A `Reserved` is a sequence of literals.
         *
         * @return The number of literals.
         *         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        int32_t numParts() const;
        /**
         * Indexes into the sequence.
         * Precondition: i < numParts()
         *
         * @param i Index of the part being accessed.
         * @return The i'th literal in the sequence, which is
         *         guaranteed to be non-null.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const Literal* getPart(int32_t i) const;
        /**
         * The mutable `Reserved::Builder` class allows the reserved sequence to be
         * constructed one part at a time.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        class U_I18N_API Builder : public UMemory {
        private:
            friend class Reserved;
          
            Builder(UErrorCode &errorCode);
            LocalPointer<ImmutableVector<Literal>::Builder> parts;
          
        public:
            /**
             * Adds a single literal to the reserved sequence.
             *
             * @param part The literal to be added
             * @param status Input/output error code
             * @return A reference to the builder.
             *
             * @internal ICU 74.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& add(const Literal& part, UErrorCode& status);
            /**
             * Constructs a new immutable `Reserved` using the list of parts
             * set with previous `add()` calls.
             *
             * The builder object (`this`) can still be used after calling `build()`.
             *
             * @param status    Input/output error code.
             * @return          The new Reserved object, which is non-null if
             *                  U_SUCCESS(status).
             *
             * @internal ICU 74.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Reserved* build(UErrorCode& status) const;
             /**
              * Destructor.
              *
              * @internal ICU 74.0 technology preview
              * @deprecated This API is for technology preview only.
              */
             virtual ~Builder();
        }; // class Reserved::Builder
        /**
         * Returns a new `Reserved::Builder` object.
         *
         * @param status  Input/output error code.
         * @return       The new Builder, which is non-null if U_SUCCESS(status).
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        static Builder *builder(UErrorCode& status);
    private:
        friend class Operator;
      
        // See comments under SelectorKeys' copy constructor; this is analogous
        bool isBogus() const { return !parts.isValid(); }
      
        // Reserved needs a copy constructor in order to make Expression deeply copyable
        Reserved(const Reserved& other);

        // Possibly-empty list of parts
        // `literal` reserved as a quoted literal; `reserved-char` / `reserved-escape`
        // strings represented as unquoted literals
        const LocalPointer<ImmutableVector<Literal>> parts;
      
        // Should only be called by Builder
        // Takes ownership of `ps`
        Reserved(ImmutableVector<Literal> *ps);
    };

    /**
     * The `Operator` class corresponds to the `FunctionRef | Reserved` type in the
     * `Expression` interface defined in
     * https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#patterns
     *
     * It represents the annotation that an expression can have: either a function name paired
     * with a map from option names to operands (possibly empty),
     * or a reserved sequence, which has no meaning and results in an error if the formatter
     * is invoked.
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API MessageFormatDataModel::Operator : public UMemory {
    public:
        /**
         * Determines if this operator is a reserved annotation.
         *
         * @return true if and only if this operator represents a reserved sequence.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UBool isReserved() const { return isReservedSequence; }
        /**
         * Accesses the function name.
         * Precondition: !isReserved()
         *
         * @return The function name of this operator.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const FunctionName& getFunctionName() const;
        /**
         * Accesses the underlying reserved sequence.
         * Precondition: isReserved()
         *
         * @return The reserved sequence represented by this operator.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const Reserved& asReserved() const;
        /**
         * Accesses function options.
         * Precondition: !isReserved()
         *
         * @return A reference to the function options for this operator.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const OptionMap& getOptions() const;

        /**
         * The mutable `Operator::Builder` class allows the operator to be constructed
         * incrementally.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        class U_I18N_API Builder : public UMemory {
        private:
            friend class Operator;
            Builder() {}
            LocalPointer<Reserved> asReserved;
            LocalPointer<FunctionName> functionName;
            LocalPointer<OptionMap::Builder> options;
        public:
            /**
             * Sets this operator to be a reserved sequence.
             * If a function name and/or options were previously set,
             * clears them. Adopts `reserved`.
             *
             * @param reserved The reserved sequence to set as the contents of this Operator.
             * @return A reference to the builder.
             *
             * @internal ICU 74.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& setReserved(Reserved* reserved);
            /**
             * Sets this operator to be a function annotation and sets its name
             * to `func`.
             * If a reserved sequence was previously set, clears it.
             *
             * @param func The function name.
             * @param status Input/output error code.
             * @return A reference to the builder.
             *
             * @internal ICU 74.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& setFunctionName(const FunctionName& func, UErrorCode& status);
            /**
             * Sets this operator to be a function annotation and adds a
             * single option.
             * If a reserved sequence was previously set, clears it.
             *
             * @param key The name of the option.
             * @param value The value (right-hand side) of the option.
             * @param status Input/output error code.
             * @return A reference to the builder.
             *
             * @internal ICU 74.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& addOption(const UnicodeString &key, Operand* value, UErrorCode& status);
            /**
             * Constructs a new immutable `Operator` using the `reserved` annotation
             * or the function name and options that were previously set.
             * If neither `setReserved()` nor `setFunctionName()` was previously
             * called, then `status` is set to U_INVALID_STATE_ERROR.
             *
             * The builder object (`this`) can still be used after calling `build()`.
             *
             * @param status    Input/output error code.
             * @return          The new Operator, which is non-null if U_SUCCESS(status).
             *
             * @internal ICU 74.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Operator* build(UErrorCode& status) const;
             /**
              * Destructor.
              *
              * @internal ICU 74.0 technology preview
              * @deprecated This API is for technology preview only.
              */
             virtual ~Builder();
        }; // class Operator::Builder
        /**
         * Returns a new `Operator::Builder` object.
         *
         * @param status  Input/output error code.
         * @return        The new Builder, which is non-null if U_SUCCESS)status).
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        static Builder* builder(UErrorCode& status);
    private:
        friend class Expression;
      
        // Postcondition: if U_SUCCESS(errorCode), then return value is non-bogus
        static Operator* create(const Reserved& r, UErrorCode& errorCode);

        // Adopts `opts`
        // Postcondition: if U_SUCCESS(errorCode), then return value is non-bogus
        static Operator* create(const FunctionName& f, OptionMap* opts, UErrorCode& errorCode);

        // Function call constructor; adopts `l` if it's non-null (creates empty options otherwise)
        Operator(const FunctionName& f, OptionMap *l);
        // Reserved sequence constructor
        // Result is bogus if copy of `r` fails
        Operator(const Reserved& r) : isReservedSequence(true), functionName(FunctionName(UnicodeString(""))), options(nullptr), reserved(new Reserved(r)) {}
        // Copy constructor
        Operator(const Operator& other);

        bool isBogus() const;
        const bool isReservedSequence;
        const FunctionName functionName;
        const LocalPointer<OptionMap> options;
        const LocalPointer<Reserved> reserved;
    }; // class Operator

    /**
     * The `Expression` class corresponds to the `expression` nonterminal in the MessageFormat 2
     * grammar and the `Expression` interface defined in
     * https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#patterns
     *
     * It represents either an operand with no annotation; an annotation with no operand;
     * or an operand annotated with an annotation.
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API MessageFormatDataModel::Expression : public UObject {
    public:
        /**
         * Checks if this expression is an annotation
         * with no operand.
         *
         * @return True if and only if the expression has
         *         an annotation and has no operand.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UBool isStandaloneAnnotation() const;
        /**
         * Checks if this expression has a function
         * annotation (with or without an operand). A reserved
         * sequence is not a function annotation.
         *
         * @return True if and only if the expression has an annotation
         *         that is a function.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UBool isFunctionCall() const;
        /**
         * Returns true if and only if this expression is
         * annotated with a reserved sequence.
         *
         * @return True if and only if the expression has an
         *         annotation that is a reserved sequence,
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UBool isReserved() const;
        /**
         * Accesses the function or reserved sequence
         * annotating this expression.
         * Precondition: isFunctionCall() || isReserved()
         *
         * @return A reference to the operator of this expression.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const Operator& getOperator() const;
        /**
         * Accesses the operand of this expression.
         *
         * @return A reference to the operand of this expression,
         *         which may be the null operand.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const Operand& getOperand() const;

        /**
         * The mutable `Expression::Builder` class allows the operator to be constructed
         * incrementally.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        class U_I18N_API Builder : public UMemory {
        private:
            friend class Expression;
            Builder() {}
            LocalPointer<Operand> rand;
            LocalPointer<Operator> rator;
        public:
            /**
             * Sets the operand of this expression. Adopts `rAnd`.
             *
             * @param rAnd The operand to set. Must be non-null.
             * @return A reference to the builder.
             *
             * @internal ICU 74.0 technology preview
             * @deprecated This API is for technology preview only.
             */
             Builder& setOperand(Operand* rAnd);
             /**
              * Sets the operator of this expression. Adopts `rAtor`.
              *
              * @param rAtor The operator to set. Must be non-null.
              * @return A reference to the builder.
              *
              * @internal ICU 74.0 technology preview
              * @deprecated This API is for technology preview only.
              */
             Builder& setOperator(Operator* rAtor);
             /**
              * Constructs a new immutable `Expression` using the operand and operator that
              * were previously set. If neither `setOperand()` nor `setOperator()` was
              * previously called, or if `setOperand()` was called with the null operand
              * and `setOperator()` was never called, then `status` is set to
              * U_INVALID_STATE_ERROR.
              *
              * The builder object (`this`) can still be used after calling `build()`.
              *
              * @param status    Input/output error code.
              * @return          The new Expression, which is non-null if U_SUCCESS(status).
              *
              * @internal ICU 74.0 technology preview
              * @deprecated This API is for technology preview only.
              */
             Expression* build(UErrorCode& status) const;
             /**
              * Destructor.
              *
              * @internal ICU 74.0 technology preview
              * @deprecated This API is for technology preview only.
              */
             virtual ~Builder();
        }; // class Expression::Builder
        /**
         * Returns a new `Expression::Builder` object.
         *
         * @param status  Input/output error code.
         * @return        The new Builder, which is non-null if U_SUCCESS(status).
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        static Builder* builder(UErrorCode& status);

    private:
        friend class ImmutableVector<Expression>;
        friend class PatternPart;
        friend class Binding;

        /*
          Internally, an expression is represented as the application of an optional operator to an operand.
          The operand is always present; for function calls with no operand, it's represented
          as an operand for which `isNull()` is true.

                                      Operator               | Operand
                                      --------------------------------
          { |42| :fun opt=value } =>  (FunctionName=fun,     | Literal(quoted=true, contents="42")
                                      options={opt: value})
          { abcd }                =>  null                   | Literal(quoted=false, contents="abcd")
          { : fun opt=value }     =>  (FunctionName=fun,
                                      options={opt: value})  | NullOperand()
        */

        // Here, a separate variable isBogus tracks if any copies failed.
        // This is because rator = nullptr and rand = nullptr are semantic here,
        // so this can't just be a predicate that checks if those are null
        bool bogus = false; // copy constructors explicitly set this to true on failure

        bool isBogus() const;

        // Expression needs a copy constructor in order to make Pattern deeply copyable
        // (and for closures)
        Expression(const Expression& other);

        Expression(const Operator &rAtor, const Operand &rAnd) : rator(new Operator(rAtor)), rand(new Operand(rAnd)) {}
        Expression(const Operand &rAnd) : rator(nullptr), rand(new Operand(rAnd)) {}
        Expression(const Operator &rAtor) : rator(new Operator(rAtor)), rand(new Operand()) {}
        const LocalPointer<Operator> rator;
        const LocalPointer<Operand> rand;
    }; // class Expression

    /**
     *  A `PatternPart` is a single element (text or expression) in a `Pattern`.
     * It corresponds to the `body` field of the `Pattern` interface
     *  defined in https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#patterns
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API MessageFormatDataModel::PatternPart : public UObject {
    public:
        /**
         * Creates a new text part.
         *
         * @param t         An arbitrary string.
         * @param status    Input/output error code.
         * @return          The new PatternPart, which is non-null if U_SUCCESS(status).
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        static PatternPart* create(const UnicodeString& t, UErrorCode& status);
        /**
         * Creates a new expression part. Adopts `e`, which must be non-null.
         *
         * @param e      Expression to use for this part.
         * @param status Input/output error code.
         * @return          The new PatternPart, which is non-null if U_SUCCESS(status).
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        static PatternPart* create(Expression* e, UErrorCode& status);
        /**
         * Checks if the part is a text part.
         *
         * @return True if and only if this is a text part.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UBool isText() const { return isRawText; }
        /**
         * Accesses the expression of the part.
         * Precondition: !isText()
         *
         * @return A reference to the part's underlying expression.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const Expression& contents() const;
        /**
         * Accesses the text contents of the part.
         * Precondition: isText()
         *
         * @return A reference to a string representing the part's text..
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const UnicodeString& asText() const;

    private:
        friend class ImmutableVector<PatternPart>;
        friend class Pattern;

        // Text
        PatternPart(const UnicodeString& t) : isRawText(true), text(t), expression(nullptr) {}
        // Expression
        PatternPart(Expression* e) : isRawText(false), expression(e) {}

        // PatternPart needs a copy constructor in order to make Pattern deeply copyable
        PatternPart(const PatternPart& other);

        const bool isRawText;
        // Not used if !isRawText
        const UnicodeString text;
        // null if isRawText
        const LocalPointer<Expression> expression;
      
        bool isBogus() const { return (!isRawText && !expression.isValid()); }
    }; // class PatternPart

    /**
     *  A `Pattern` is a sequence of formattable parts.
     * It corresponds to the `Pattern` interface
     * defined in https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#patterns
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API MessageFormatDataModel::Pattern : public UObject {
    public:
        /**
         * Returns the size.
         *
         * @return The number of parts in the pattern.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        int32_t numParts() const { return parts->length(); }
        /**
         * Returns the `i`th part in the pattern.
         * Precondition: i < numParts()
         *
         * @param i Index of the part being accessed.
         * @return  The part at index `i`, which is guaranteed
         *          to be non-null.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const PatternPart* getPart(int32_t i) const;

        /**
         * The mutable `Pattern::Builder` class allows the pattern to be
         * constructed one part at a time.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        class U_I18N_API Builder : public UMemory {
        private:
            friend class Pattern;
          
            Builder(UErrorCode &errorCode);
            // Note this is why PatternPart and all its enclosed classes need
            // copy constructors: when the build() method is called on `parts`,
            // it should copy `parts` rather than moving it
            LocalPointer<ImmutableVector<PatternPart>::Builder> parts;
          
        public:
            /**
             * Adds a single part to the pattern. Adopts `part`.
             *
             * @param part The part to be added. Must be non-null.
             * @param status Input/output error code
             * @return A reference to the builder.
             *
             * @internal ICU 74.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& add(PatternPart *part, UErrorCode& status);
            /**
             * Constructs a new immutable `Pattern` using the list of parts
             * set with previous `add()` calls.
             *
             * The builder object (`this`) can still be used after calling `build()`.
             *
             * @param status    Input/output error code.
             * @return          The new pattern, which is non-null if U_SUCCESS(status).
             *
             * @internal ICU 74.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Pattern* build(UErrorCode &status) const;
             /**
              * Destructor.
              *
              * @internal ICU 74.0 technology preview
              * @deprecated This API is for technology preview only.
              */
             virtual ~Builder();
        }; // class Pattern::Builder

        /**
         * Returns a new `Pattern::Builder` object.
         *
         * @param status  Input/output error code.
         * @return        The new Builder, which is non-null if U_SUCCESS(status).
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        static Builder* builder(UErrorCode& status);
    private:
        friend class MessageFormatDataModelImpl;
        friend class OrderedMap<Pattern>;

        // Possibly-empty list of parts
        const LocalPointer<ImmutableVector<PatternPart>> parts;
      
        bool isBogus() const { return !parts.isValid(); }

        // Should only be called by Builder
        // Takes ownership of `ps`
        Pattern(ImmutableVector<PatternPart> *ps);

        // Pattern needs a copy constructor in order to make MessageFormatDataModel::build() be a copying rather than
        // moving build
        Pattern(const Pattern& other);
    }; // class Pattern

    /**
     *  A `Binding` pairs a variable name with an expression.
     * It corresponds to the `Declaration` interface
     * defined in https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#messages
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API MessageFormatDataModel::Binding : public UObject {
    public:
        /**
         * Creates a new binding.  Adopts `e`, which must be non-null.
         *
         * @param var       The variable name of the declaration.
         * @param e         The expression (right-hand side) of the declaration.
         * @param status    Input/output error code.
         * @return          The new binding, which is non-null if U_SUCCESS(status).
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        static Binding* create(const VariableName& var, Expression* e, UErrorCode& status);
        /**
          * Accesses the right-hand side of the binding.
          *
          * @return A reference to the expression.
          *
          * @internal ICU 74.0 technology preview
          * @deprecated This API is for technology preview only.
          */
        const Expression& getValue() const;
        /**
          * Accesses the left-hand side of the binding.
          *
          * @return A reference to the variable name.
          *
          * @internal ICU 74.0 technology preview
          * @deprecated This API is for technology preview only.
          */
        const VariableName& getVariable() const { return var; }
        /**
          * Destructor.
          *
          * @internal ICU 74.0 technology preview
          * @deprecated This API is for technology preview only.
          */
        virtual ~Binding();
    private:
        friend class ImmutableVector<Binding>;

        const VariableName var;
        const LocalPointer<Expression> value;

        bool isBogus() const { return !value.isValid(); }

        Binding(const VariableName& v, Expression* e) : var(v), value(e){}
        // This needs a copy constructor so that `Bindings` is deeply-copyable,
        // which is in turn so that MessageFormatDataModel::build() can be copying
        // (it has to copy the builder's locals)
        Binding(const Binding& other);
    }; // class Binding
} // namespace message2


/// @cond DOXYGEN_IGNORE
// Export an explicit template instantiation of the LocalPointer that is used as a
// data member of various MessageFormatDataModel classes.
// (When building DLLs for Windows this is required.)
// (See messageformat2_data_model_forward_decls.h for similar examples.)
#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
#if defined(_MSC_VER)
// Ignore warning 4661 as LocalPointerBase does not use operator== or operator!=
#pragma warning(push)
#pragma warning(disable: 4661)
#endif
template class U_I18N_API LocalPointer<message2::MessageFormatDataModel::VariantMap::Builder>;
template class U_I18N_API LocalPointerBase<message2::MessageFormatDataModel::VariantMap::Builder>;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#endif
/// @endcond

namespace message2 {
    /**
     * The mutable `MessageFormatDataModel::Builder` class allows the data model to be
     * constructed incrementally.
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API MessageFormatDataModel::Builder : public UMemory {
    private:
        friend class MessageFormatDataModel;
        friend class MessageFormatDataModelImpl;

        Builder(UErrorCode& errorCode);
        void buildSelectorsMessage(UErrorCode& errorCode);
        LocalPointer<Pattern> pattern;
        LocalPointer<ExpressionList::Builder> selectors;
        LocalPointer<VariantMap::Builder> variants;
        LocalPointer<Bindings::Builder> locals;
      
    public:
        /**
         * Adds a local variable declaration. Adopts `expression`, which must be non-null.
         *
         * @param variableName The variable name of the declaration.
         * @param expression The expression to which `variableName` should be bound.
         * @param status Input/output error code.
         * @return A reference to the builder.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Builder& addLocalVariable(const VariableName& variableName, Expression* expression, UErrorCode &status);
        /**
         * Adds a selector expression. Adopts `expression`, which must be non-null.
         * If a pattern was previously set, clears the pattern.
         *
         * @param selector Expression to add as a selector.
         * @param status Input/output error code.
         * @return A reference to the builder.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Builder& addSelector(Expression* selector, UErrorCode& status);
        /**
         * Adds a single variant. Adopts `keys` and `pattern`, which must be non-null.
         * If a pattern was previously set using `setPattern()`, clears the pattern.
         *
         * @param keys Keys for the variant.
         * @param pattern Pattern for the variant.
         * @param status Input/output error code.
         * @return A reference to the builder.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Builder& addVariant(SelectorKeys* keys, Pattern* pattern, UErrorCode& status);
        /**
         * Sets the body of the message as a pattern.
         * If selectors and/or variants were previously set, clears them.
         * Adopts `pattern`, which must be non-null.
         *
         * @param pattern Pattern to represent the body of the message.
         * @return A reference to the builder.
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Builder& setPattern(Pattern* pattern);
        /**
         * Constructs a new immutable data model.
         * If `setPattern()` has not been called and if `addSelector()` and
         * `addVariant()` were not each called at least once,
         * `status` is set to `U_INVALID_STATE_ERROR`.
         * If `addSelector()` was called and `addVariant()` was never called,
         * or vice versa, then `status` is set to U_INVALID_STATE_ERROR.
         * Otherwise, either a Pattern or Selectors message is constructed
         * based on the pattern that was previously set, or selectors and variants
         * that were previously set.
         *
         * The builder object (`this`) can still be used after calling `build()`.
         *
         * @param status Input/output error code.
         * @return       The new MessageFormatDataModel, which is non-null if
         *               U_SUCCESS(status).
         *
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        MessageFormatDataModel* build(UErrorCode& status) const;
    }; // class MessageFormatDataModel::Builder


// These explicit instantiations have to come before the
// destructor definitions
template<>
ImmutableVector<MessageFormatDataModel::Binding>::Builder::~Builder();
template<>
ImmutableVector<MessageFormatDataModel::Binding>::~ImmutableVector();
template<>
ImmutableVector<MessageFormatDataModel::Expression>::Builder::~Builder();
template<>
ImmutableVector<MessageFormatDataModel::Expression>::~ImmutableVector();
template<>
ImmutableVector<MessageFormatDataModel::Key>::Builder::~Builder();
template<>
ImmutableVector<MessageFormatDataModel::Key>::~ImmutableVector();
template<>
ImmutableVector<MessageFormatDataModel::Literal>::Builder::~Builder();
template<>
ImmutableVector<MessageFormatDataModel::Literal>::~ImmutableVector();
template<>
ImmutableVector<MessageFormatDataModel::PatternPart>::Builder::~Builder();
template<>
ImmutableVector<MessageFormatDataModel::PatternPart>::~ImmutableVector();
template<>
ImmutableVector<MessageFormatDataModel::SelectorKeys>::Builder::~Builder();
template<>
ImmutableVector<MessageFormatDataModel::SelectorKeys>::~ImmutableVector();
template<>
OrderedMap<MessageFormatDataModel::Pattern>::Builder::~Builder();
template<>
OrderedMap<MessageFormatDataModel::Pattern>::~OrderedMap();
template<>
OrderedMap<MessageFormatDataModel::Operand>::Builder::~Builder();
template<>
OrderedMap<MessageFormatDataModel::Operand>::~OrderedMap();

// Explicit instantiations in source/i18n/messageformat2_utils.cpp
// See numberformatter.h for another example

// (MSVC treats imports/exports of explicit instantiations differently.)
#ifndef _MSC_VER
extern template class ImmutableVector<MessageFormatDataModel::Binding>;
extern template class ImmutableVector<MessageFormatDataModel::Expression>;
extern template class ImmutableVector<MessageFormatDataModel::Key>;
extern template class ImmutableVector<MessageFormatDataModel::Literal>;
extern template class ImmutableVector<MessageFormatDataModel::PatternPart>;
extern template class ImmutableVector<MessageFormatDataModel::SelectorKeys>;
extern template class OrderedMap<MessageFormatDataModel::Operand>;
extern template class OrderedMap<MessageFormatDataModel::Pattern>;
#endif

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT_DATA_MODEL_H

#endif // U_HIDE_DEPRECATED_API
// eof

