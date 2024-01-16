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
#include "unicode/messageformat2_formatting_context.h"
#include "unicode/numberformatter.h"
#include "unicode/unistr.h"
#include "unicode/upluralrules.h"

U_NAMESPACE_BEGIN

class Hashtable;

namespace message2 {

    class Formatter;
    class Selector;

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
         * @param result A reference that will be initialized to the selector factory registered
         *               under `selectorName` if the return value is true. If the return value is
         *               false, this reference is undefined.
         * @return True if and only if a selector factory was registered under `selectorName`.
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
            LocalPointer<FormatterMap> formatters;
            LocalPointer<SelectorMap> selectors;

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
         * Move constructor.
         * The source FunctionRegistry will be left in a valid but undefined state.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        FunctionRegistry(FunctionRegistry&&) noexcept;
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

        LocalPointer<FormatterMap> formatters;
        LocalPointer<SelectorMap> selectors;
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
         * @param context Formatting context; captures the unnamed function argument,
         *        current output, named options, and output. See the `FormattingContext`
         *        documentation for more details.
         * @param toFormat Formatted value; see `message2::FormattedValue` for details.
         *        Passed by move.
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
        virtual FormattedValue format(FormattingContext& context, FormattedValue&& toFormat, UErrorCode& status) const = 0;
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
         * @param context Formatting context; captures named options.
         *        See the `FormattingContext` documentation for more details.
         * @param toFormat The unnamed function argument; passed by move.
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
        virtual void selectKey(FormattingContext& context,
                               FormattedValue&& toFormat,
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
