// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT2_FUNCTION_REGISTRY_H
#define MESSAGEFORMAT2_FUNCTION_REGISTRY_H

#include "unicode/utypes.h"

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#include "unicode/datefmt.h"
#include "unicode/format.h"
#include "unicode/messageformat2_data_model.h"
#include "unicode/messageformat2_formattable.h"
#include "unicode/numberformatter.h"
#include "unicode/unistr.h"
#include "unicode/upluralrules.h"

#include <map>

U_NAMESPACE_BEGIN

class Hashtable;

namespace message2 {

    class Formatter;
    class Selector;

/**
 * Internal use only, but has to be included here as part of the implementation
 * of the header-only `FunctionOptions::getOptions()` method
 *
 *  A `ResolvedFunctionOption` represents the result of evaluating
 * a single named function option. It pairs the given name with the `Formattable`
 * value resulting from evaluating the option's value.
 *
 * `ResolvedFunctionOption` is immutable and is not copyable or movable.
 *
 * @internal ICU 75.0 technology preview
 * @deprecated This API is for technology preview only.
 */
#ifndef U_IN_DOXYGEN
class U_I18N_API ResolvedFunctionOption : public UObject {
  private:

    /* const */ UnicodeString name;
    /* const */ Formattable value;

  public:
      const UnicodeString& getName() const { return name; }
      const Formattable& getValue() const { return value; }
      ResolvedFunctionOption(const UnicodeString& n, const Formattable& f) : name(n), value(f) {}
      ResolvedFunctionOption() {}
      ResolvedFunctionOption(ResolvedFunctionOption&&);
      ResolvedFunctionOption& operator=(ResolvedFunctionOption&& other) noexcept {
          name = std::move(other.name);
          value = std::move(other.value);
          return *this;
    }
    virtual ~ResolvedFunctionOption();
}; // class ResolvedFunctionOption
#endif

/**
 * Mapping from option names to `message2::Formattable` objects, obtained
 * by calling `getOptions()` on a `FunctionOptions` object.
 *
 * @internal ICU 75.0 technology preview
 * @deprecated This API is for technology preview only.
 */
using FunctionOptionsMap = std::map<UnicodeString, message2::Formattable>;

/**
 * Structure encapsulating named options passed to a custom selector or formatter.
 *
 * @internal ICU 75.0 technology preview
 * @deprecated This API is for technology preview only.
 */
class U_I18N_API FunctionOptions : public UObject {
 public:
    /**
     * Returns a map of all name-value pairs provided as options to this function.
     * The syntactic order of options is not guaranteed to
     * be preserved.
     *
     * @return           A map from strings to `message2::Formattable` objects representing
     *                   the results of resolving each option value.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    FunctionOptionsMap getOptions() const {
        int32_t len;
        const ResolvedFunctionOption* resolvedOptions = getResolvedFunctionOptions(len);
        FunctionOptionsMap result;
        for (int32_t i = 0; i < len; i++) {
            const ResolvedFunctionOption& opt = resolvedOptions[i];
            result[opt.getName()] = opt.getValue();
        }
        return result;
    }
    /**
     * Default constructor.
     * Returns an empty mapping.
     *     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    FunctionOptions() { options = nullptr; }
 private:
    friend class MessageFormatter;
    friend class StandardFunctions;

    explicit FunctionOptions(UVector&&, UErrorCode&);

    const ResolvedFunctionOption* getResolvedFunctionOptions(int32_t& len) const;
    UBool getFunctionOption(const UnicodeString&, Formattable&) const;
    int32_t optionsCount() const { return functionOptionsLen; }

    // Named options passed to functions
    // This is not a Hashtable in order to make it possible for code in a public header file
    // to construct a std::map from it, on-the-fly. Otherwise, it would be impossible to put
    // that code in the header because it would have to call internal Hashtable methods.
    ResolvedFunctionOption* options;
    int32_t functionOptionsLen = 0;
};

    /**
     * Interface that factory classes for creating formatters must implement.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API FormatterFactory : public UObject {
        // TODO: the coding guidelines say that interface classes
        // shouldn't inherit from UObject, but if I change it so these
        // classes don't, and the individual formatter factory classes
        // inherit from public FormatterFactory, public UObject, then
        // memory leaks ensue
    public:
        /**
         * Constructs a new formatter object. This method is not const;
         * formatter factories with local state may be defined.
         *
         * @param locale Locale to be used by the formatter.
         * @param status    Input/output error code.
         * @return The new Formatter, which is non-null if U_SUCCESS(status).
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        virtual Formatter* createFormatter(const Locale& locale, UErrorCode& status) = 0;
        virtual ~FormatterFactory();
        FormatterFactory& operator=(const FormatterFactory&) = delete;
    }; // class FormatterFactory

    /**
     * Interface that factory classes for creating selectors must implement.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API SelectorFactory : public UObject {
    public:
        /**
         * Constructs a new selector object.
         *
         * @param locale    Locale to be used by the selector.
         * @param status    Input/output error code.
         * @return          The new selector, which is non-null if U_SUCCESS(status).
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        virtual Selector* createSelector(const Locale& locale, UErrorCode& status) const = 0;
        virtual ~SelectorFactory();
        SelectorFactory& operator=(const SelectorFactory&) = delete;
    }; // class SelectorFactory

    /**
     * Defines mappings from names of formatters and selectors to functions implementing them.
     * The required set of formatter and selector functions is defined in the spec. Users can
     * also define custom formatter and selector functions.
     *
     * `FunctionRegistry` is immutable and movable. It is not copyable.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API FunctionRegistry : public UObject {
    private:

        using FormatterMap = Hashtable; // Map from stringified function names to FormatterFactory*
        using SelectorMap  = Hashtable; // Map from stringified function names to SelectorFactory*

    public:
        /**
         * Looks up a formatter factory by the name of the formatter. The result is non-const,
         * since formatter factories may have local state. (This returns the result by pointer
         * rather than by reference since `FormatterFactory` is an abstract class.)
         *
         * @param formatterName Name of the desired formatter.
         * @return A pointer to the `FormatterFactory` registered under `formatterName`, or null
         *         if no formatter was registered under that name.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        FormatterFactory* getFormatter(const data_model::FunctionName& formatterName) const;
        /**
         * Looks up a selector factory by the name of the selector. (This returns the result by pointer
         * rather than by reference since `FormatterFactory` is an abstract class.)
         *
         * @param selectorName Name of the desired selector.
         * @return A pointer to the `SelectorFactory` registered under `selectorName`, or null
         *         if no formatter was registered under that name.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const SelectorFactory* getSelector(const data_model::FunctionName& selectorName) const;
        /**
         * The mutable Builder class allows each formatter and selector factory
         * to be initialized separately; calling its `build()` method yields an
         * immutable FunctionRegistry object.
         *
         * Builder is not copyable or movable.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        class U_I18N_API Builder : public UObject {
        private:
            // Must use raw pointers to avoid instantiating `LocalPointer` on an internal type
            FormatterMap* formatters;
            SelectorMap* selectors;

            // Do not define copy constructor/assignment operator
            Builder& operator=(const Builder&) = delete;
            Builder(const Builder&) = delete;

        public:
            /**
             * Registers a formatter factory to a given formatter name. Adopts `formatterFactory`.
             *
             * @param formatterName Name of the formatter being registered.
             * @param formatterFactory A FormatterFactory object to use for creating `formatterName`
             *        formatters.
             * @param errorCode Input/output error code
             * @return A reference to the builder.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& setFormatter(const data_model::FunctionName& formatterName, FormatterFactory* formatterFactory, UErrorCode& errorCode);
            /**
             * Registers a selector factory to a given selector name. Adopts `selectorFactory`.
             *
             * @param selectorName Name of the selector being registered.
             * @param selectorFactory A SelectorFactory object to use for creating `selectorName`
             *        selectors.
             * @param errorCode Input/output error code
             * @return A reference to the builder.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& setSelector(const data_model::FunctionName& selectorName, SelectorFactory* selectorFactory, UErrorCode& errorCode);
            /**
             * Creates an immutable `FunctionRegistry` object with the selectors and formatters
             * that were previously registered. The builder cannot be used after this call.
             * The `build()` method is destructive to avoid the need for a deep copy of the
             * `FormatterFactory` and `SelectorFactory` objects (this would be necessary because
             * `FormatterFactory` can have mutable state), which in turn would require implementors
             * of those interfaces to implement a `clone()` method.
             *
             * @return The new FunctionRegistry
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            FunctionRegistry build();
            /**
             * Default constructor.
             * Returns a Builder with no functions registered.
             *
             * @param errorCode Input/output error code
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder(UErrorCode& errorCode);
            /**
             * Destructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            virtual ~Builder();
        }; // class FunctionRegistry::Builder

        /**
         * Move assignment operator:
         * The source FunctionRegistry will be left in a valid but undefined state.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        FunctionRegistry& operator=(FunctionRegistry&&) noexcept;
        /**
         * Move constructor:
         * The source FunctionRegistry will be left in a valid but undefined state.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        FunctionRegistry(FunctionRegistry&& other) { *this = std::move(other); }
        /**
         * Destructor.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        virtual ~FunctionRegistry();

    private:
        friend class MessageContext;
        friend class MessageFormatter;

        // Do not define copy constructor or copy assignment operator
        FunctionRegistry& operator=(const FunctionRegistry&) = delete;
        FunctionRegistry(const FunctionRegistry&) = delete;

        FunctionRegistry(FormatterMap* f, SelectorMap* s);

        FunctionRegistry() {}

        // Debugging; should only be called on a function registry with
        // all the standard functions registered
        void checkFormatter(const char*) const;
        void checkSelector(const char*) const;
        void checkStandard() const;

        bool hasFormatter(const data_model::FunctionName& f) const;
        bool hasSelector(const data_model::FunctionName& s) const;

        // Must use raw pointers to avoid instantiating `LocalPointer` on an internal type
        FormatterMap* formatters;
        SelectorMap* selectors;
    }; // class FunctionRegistry

    /**
     * Interface that formatter classes must implement.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API Formatter : public UObject {
    public:
        /**
         * Formats the input passed in `context` by setting an output using one of the
         * `FormattingContext` methods or indicating an error.
         *
         * @param toFormat Placeholder, including a source formattable value and possibly
         *        the output of a previous formatter applied to it; see
         *        `message2::FormattedPlaceholder` for details. Passed by move.
         * @param options The named function options. Passed by move
         * @param status    Input/output error code. Should not be set directly by the
         *        custom formatter, which should use `FormattingContext::setFormattingWarning()`
         *        to signal errors. The custom formatter may pass `status` to other ICU functions
         *        that can signal errors using this mechanism.
         *
         * @return The formatted value.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        virtual FormattedPlaceholder format(FormattedPlaceholder&& toFormat,
                                      FunctionOptions&& options,
                                      UErrorCode& status) const = 0;
        virtual ~Formatter();
    }; // class Formatter

    /**
     * Interface that selector classes must implement.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API Selector : public UObject {
    public:
        /**
         * Compares the input to an array of keys, and returns an array of matching
         * keys sorted by preference.
         *
         * @param toFormat The unnamed function argument; passed by move.
         * @param options A reference to the named function options.
         * @param keys An array of strings that are compared to the input
         *        (`context.getFormattableInput()`) in an implementation-specific way.
         * @param keysLen The length of `keys`.
         * @param prefs An array of strings with length `keysLen`. The contents of
         *        the array is undefined. `selectKey()` should set the contents
         *        of `prefs` to a subset of `keys`, with the best match placed at the lowest index.
         * @param prefsLen A reference that `selectKey()` should set to the length of `prefs`,
         *        which must be less than or equal to `keysLen`.
         * @param status    Input/output error code. Should not be set directly by the
         *        custom selector, which should use `FormattingContext::setSelectorError()`
         *        to signal errors. The custom selector may pass `status` to other ICU functions
         *        that can signal errors using this mechanism.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        virtual void selectKey(FormattedPlaceholder&& toFormat,
                               FunctionOptions&& options,
                               const UnicodeString* keys,
                               int32_t keysLen,
                               UnicodeString* prefs,
                               int32_t& prefsLen,
                               UErrorCode& status) const = 0;
        // Note: This takes array arguments because the internal MessageFormat code has to
        // call this method, and can't include any code that constructs std::vectors.
        virtual ~Selector();
    }; // class Selector

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT2_FUNCTION_REGISTRY_H

#endif // U_HIDE_DEPRECATED_API
// eof
