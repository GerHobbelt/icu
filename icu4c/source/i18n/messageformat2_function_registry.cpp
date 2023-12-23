// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/dtptngen.h"
#include "unicode/messageformat2.h"
#include "unicode/messageformat2_formatting_context.h"
#include "unicode/numberformatter.h"
#include "unicode/smpdtfmt.h"
#include "messageformat2_context.h"
#include "messageformat2_function_registry_internal.h"
#include "messageformat2_macros.h"
#include "uvector.h" // U_ASSERT

U_NAMESPACE_BEGIN

namespace message2 {

// Function registry implementation

Formatter::~Formatter() {}
Selector::~Selector() {}
FormatterFactory::~FormatterFactory() {}
SelectorFactory::~SelectorFactory() {}

FunctionRegistry FunctionRegistry::Builder::build() {
    return FunctionRegistry(std::move(formatters), std::move(selectors));
}

FunctionRegistry::Builder& FunctionRegistry::Builder::setSelector(const FunctionName& selectorName, SelectorFactory* selectorFactory) noexcept {
    selectors[selectorName] = std::shared_ptr<SelectorFactory>(selectorFactory);
    return *this;
}

FunctionRegistry::Builder& FunctionRegistry::Builder::setFormatter(const FunctionName& formatterName, FormatterFactory* formatterFactory) noexcept {
    formatters[formatterName] = std::shared_ptr<FormatterFactory>(formatterFactory);
    return *this;
}

FunctionRegistry::Builder::~Builder() {}

// This method is not const since it returns a non-const alias to a value in the map
std::shared_ptr<FormatterFactory> FunctionRegistry::getFormatter(const FunctionName& formatterName) {
    // Caller must check for null
    if (!hasFormatter(formatterName)) {
	return nullptr;
    }
    return formatters.at(formatterName);
}

const std::shared_ptr<SelectorFactory> FunctionRegistry::getSelector(const FunctionName& selectorName) const {
    // Caller must check for null
    if (!hasSelector(selectorName)) {
	return nullptr;
    }
    return selectors.at(selectorName);
}

bool FunctionRegistry::hasFormatter(const FunctionName& f) const {
    return formatters.count(f) > 0;
}

bool FunctionRegistry::hasSelector(const FunctionName& s) const {
    return selectors.count(s) > 0;
}

void FunctionRegistry::checkFormatter(const char* s) const {
#ifdef _DEBUG
    U_ASSERT(hasFormatter(FunctionName(UnicodeString(s))));
#else
   (void) s;
#endif
}

void FunctionRegistry::checkSelector(const char* s) const {
#ifdef _DEBUG
    U_ASSERT(hasSelector(FunctionName(UnicodeString(s))));
#else
    (void) s;
#endif
}

// Debugging
void FunctionRegistry::checkStandard() const {
    checkFormatter("datetime");
    checkFormatter("number");
    checkFormatter("identity");
    checkSelector("plural");
    checkSelector("selectordinal");
    checkSelector("select");
    checkSelector("gender");
}

// Formatter/selector helpers

// Converts `s` to an int64 value if possible, returning false
// if it can't be parsed
static bool tryStringToNumber(const UnicodeString& s, int64_t& result) {
    UErrorCode localErrorCode = U_ZERO_ERROR;
    // Try to parse string as int

    LocalPointer<NumberFormat> numberFormat(NumberFormat::createInstance(localErrorCode));
    if (U_FAILURE(localErrorCode)) {
        return false;
    }
    numberFormat->setParseIntegerOnly(true);
    Formattable asNumber;
    numberFormat->parse(s, asNumber, localErrorCode);
    if (U_SUCCESS(localErrorCode)) {
        result = asNumber.getInt64(localErrorCode);
        if (U_SUCCESS(localErrorCode)) {
            return true;
        }
    }
    return false;
}

// Converts `s` to a double, indicating failure via `errorCode`
static void strToDouble(const UnicodeString& s, const Locale& loc, double& result, UErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    LocalPointer<NumberFormat> numberFormat(NumberFormat::createInstance(loc, errorCode));
    CHECK_ERROR(errorCode);
    Formattable asNumber;
    numberFormat->parse(s, asNumber, errorCode);
    CHECK_ERROR(errorCode);
    result = asNumber.getDouble(errorCode);
}

// Converts `optionValue` to an int64 value if possible, returning false
// if it can't be parsed
bool tryFormattableAsNumber(const Formattable& optionValue, int64_t& result) {
    UErrorCode localErrorCode = U_ZERO_ERROR;
    if (optionValue.isNumeric()) {
        result = optionValue.getInt64(localErrorCode);
        if (U_SUCCESS(localErrorCode)) {
            return true;
        }
    } else {
        if (tryStringToNumber(optionValue.getString(), result)) {
            return true;
        }
    }
    return false;
}

FunctionRegistry::FunctionRegistry(FormatterMap&& f, SelectorMap&& s) : formatters(std::move(f)), selectors(std::move(s)) {}

FunctionRegistry& FunctionRegistry::operator=(FunctionRegistry&& other) noexcept {
    formatters = std::move(other.formatters);
    selectors = std::move(other.selectors);

    return *this;
}

FunctionRegistry::FunctionRegistry(FunctionRegistry&& other) noexcept : formatters(std::move(other.formatters)), selectors(std::move(other.selectors)) {}

FunctionRegistry::~FunctionRegistry() {}

// Specific formatter implementations

// --------- Number

number::LocalizedNumberFormatter formatterForOptions(Locale locale, const FormattingContext& context, UErrorCode& status) {
    number::UnlocalizedNumberFormatter nf;
    if (U_SUCCESS(status)) {
        UnicodeString skeleton;
        if (context.getStringOption(UnicodeString("skeleton"), skeleton)) {
            nf = number::NumberFormatter::forSkeleton(skeleton, status);
        } else {
            int64_t minFractionDigits = 0;
            context.getInt64Option(UnicodeString("minimumFractionDigits"), minFractionDigits);
            nf = number::NumberFormatter::with().precision(number::Precision::minFraction((int32_t) minFractionDigits));
        }
    }
    return number::LocalizedNumberFormatter(nf.locale(locale));
}

Formatter* StandardFunctions::NumberFactory::createFormatter(const Locale& locale, UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);

    Formatter* result = new Number(locale);
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return nullptr;
    }
    return result;
}

static void notANumber(FormattingContext& context) {
    context.setOutput(UnicodeString("NaN"));
}

static void stringAsNumber(Locale locale, const number::LocalizedNumberFormatter& nf, FormattingContext& context, UnicodeString s, int64_t offset, UErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    double numberValue;
    UErrorCode localErrorCode = U_ZERO_ERROR;
    strToDouble(s, locale, numberValue, localErrorCode);
    if (U_FAILURE(localErrorCode)) {
        notANumber(context);
        return;
    }
    UErrorCode savedStatus = errorCode;
    number::FormattedNumber result = nf.formatDouble(numberValue - offset, errorCode);
    // Ignore U_USING_DEFAULT_WARNING
    if (errorCode == U_USING_DEFAULT_WARNING) {
        errorCode = savedStatus;
    }
    context.setOutput(std::move(result));
}

void StandardFunctions::Number::format(FormattingContext& context, UErrorCode& errorCode) const {
    CHECK_ERROR(errorCode);

    // No argument => return "NaN"
    if (!context.hasFormattableInput()) {
        return notANumber(context);
    }

    int64_t offset = 0;
    context.getInt64Option(UnicodeString("offset"), offset);

    number::LocalizedNumberFormatter realFormatter;
    if (context.optionsCount() == 0) {
        realFormatter = number::LocalizedNumberFormatter(icuFormatter);
    } else {
        realFormatter = formatterForOptions(locale, context, errorCode);
    }

    if (context.hasStringOutput()) {
        stringAsNumber(locale, realFormatter, context, context.getStringOutput(), offset, errorCode);
        return;
    } else if (context.hasNumberOutput()) {
        // Nothing to do
        return;
    }

    number::FormattedNumber numberResult;
    // Already checked that input is present
    const Formattable& toFormat = context.getFormattableInput();
    switch (toFormat.getType()) {
    case Formattable::Type::kDouble: {
        numberResult = realFormatter.formatDouble(toFormat.getDouble() - offset, errorCode);
        break;
    }
    case Formattable::Type::kLong: {
        numberResult = realFormatter.formatInt(toFormat.getLong() - offset, errorCode);
        break;
    }
    case Formattable::Type::kInt64: {
        numberResult = realFormatter.formatInt(toFormat.getInt64() - offset, errorCode);
        break;
    }
    case Formattable::Type::kString: {
        // Try to parse the string as a number
        stringAsNumber(locale, realFormatter, context, toFormat.getString(), offset, errorCode);
        return;
    }
    default: {
        // Other types can't be parsed as a number
        notANumber(context);
        return;
    }
    }

    context.setOutput(std::move(numberResult));
}

StandardFunctions::Number::~Number() {}
StandardFunctions::NumberFactory::~NumberFactory() {}

// --------- PluralFactory

Selector* StandardFunctions::PluralFactory::createSelector(const Locale& locale, UErrorCode& errorCode) const {
    NULL_ON_ERROR(errorCode);

    // Look up plural rules by locale
    LocalPointer<PluralRules> rules(PluralRules::forLocale(locale, type, errorCode));
    NULL_ON_ERROR(errorCode);
    Selector* result = new Plural(locale, rules.orphan());
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return nullptr;
    }
    return result;
}

static void tryAsString(const Locale& locale, const UnicodeString& s, double& valToCheck, bool& noMatch) {
    // Try parsing the inputString as a double
    UErrorCode localErrorCode = U_ZERO_ERROR;
    strToDouble(s, locale, valToCheck, localErrorCode);
    // Invalid format error => value is not a number; no match
    if (U_FAILURE(localErrorCode)) {
        noMatch = true;
        return;
    }
    noMatch = false;
}

static void tryWithFormattable(const Locale& locale, const Formattable& value, double& valToCheck, bool& noMatch) {
    switch (value.getType()) {
        case Formattable::Type::kDouble: {
            valToCheck = value.getDouble();
            break;
        }
        case Formattable::Type::kLong: {
            valToCheck = (double) value.getLong();
            break;
        }
        case Formattable::Type::kInt64: {
            valToCheck = (double) value.getInt64();
            break;
        }
        case Formattable::Type::kString: {
            tryAsString(locale, value.getString(), valToCheck, noMatch);
            return;
        }
        default: {
            noMatch = true;
            return;
        }
    }
    noMatch = false;
}

void StandardFunctions::Plural::selectKey(FormattingContext& context,
                                          const UnicodeString* keys,
                                          int32_t keysLen,
                                          UnicodeString* prefs,
                                          int32_t& prefsLen,
					  UErrorCode& errorCode) const {
    CHECK_ERROR(errorCode);

    // No argument => return "NaN"
    if (!context.hasFormattableInput()) {
        context.setSelectorError(UnicodeString("plural"), errorCode);
        return;
    }

    int64_t offset = 0;
    context.getInt64Option(UnicodeString("offset"), offset);

    // Only doubles and integers can match
    double valToCheck;
    bool noMatch = true;

    bool isFormattedNumber = context.hasNumberOutput();
    bool isFormattedString = context.hasStringOutput();

    if (isFormattedString) {
        // Formatted string: try parsing it as a number
        tryAsString(locale, context.getStringOutput(), valToCheck, noMatch);
    } else {
        // Already checked that input is present
        tryWithFormattable(locale, context.getFormattableInput(), valToCheck, noMatch);
    }

    if (noMatch) {
        // Non-number => selector error
        context.setSelectorError(UnicodeString("plural"), errorCode);
        return;
    }

    // Generate the matches
    // -----------------------

    // First, check for an exact match
    prefsLen = 0;
    double keyAsDouble = 0;
    for (int32_t i = 0; i < keysLen; i++) {
        // Try parsing the key as a double
        UErrorCode localErrorCode = U_ZERO_ERROR;
        strToDouble(keys[i], locale, keyAsDouble, localErrorCode);
        if (U_SUCCESS(localErrorCode)) {
            if (valToCheck == keyAsDouble) {
		prefs[0] = keys[i];
                prefsLen = 1;
                break;
            }
        }
    }
    if (prefsLen > 0) {
        return;
    }

    // If there was no exact match, check for a match based on the plural category
    UnicodeString match;
    if (isFormattedNumber) {
        match = rules->select(context.getNumberOutput(), errorCode);
    } else {
        match = rules->select(valToCheck - offset);
    }
    CHECK_ERROR(errorCode);

    for (int32_t i = 0; i < keysLen; i ++) {
        if (match == keys[i]) {
            prefs[0] = keys[i];
            prefsLen = 1;
            break;
        }
    }
}

StandardFunctions::Plural::~Plural() {}
StandardFunctions::PluralFactory::~PluralFactory() {}

// --------- DateTimeFactory

static DateFormat::EStyle stringToStyle(UnicodeString option, UErrorCode& errorCode) {
    if (U_SUCCESS(errorCode)) {
        UnicodeString upper = option.toUpper();
        if (upper == UnicodeString("FULL")) {
            return DateFormat::EStyle::kFull;
        }
        if (upper == UnicodeString("LONG")) {
            return DateFormat::EStyle::kLong;
        }
        if (upper == UnicodeString("MEDIUM")) {
            return DateFormat::EStyle::kMedium;
        }
        if (upper == UnicodeString("SHORT")) {
            return DateFormat::EStyle::kShort;
        }
        if (upper.isEmpty() || upper == UnicodeString("DEFAULT")) {
            return DateFormat::EStyle::kDefault;
        }
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
    }
    return DateFormat::EStyle::kNone;
}

Formatter* StandardFunctions::DateTimeFactory::createFormatter(const Locale& locale, UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);

    Formatter* result = new DateTime(locale);
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return nullptr;
    }
    return result;
}

void StandardFunctions::DateTime::format(FormattingContext& context, UErrorCode& errorCode) const {
    CHECK_ERROR(errorCode);

    // Argument must be present;
    // also, if there is existing string output, that's
    // like passing in a string, so we return
    if (!context.hasFormattableInput() || context.hasStringOutput()) {
        context.setFormattingError(UnicodeString("datetime"), errorCode);
        return;
    }

    LocalPointer<DateFormat> df;

    UnicodeString opt;
    if (context.getStringOption(UnicodeString("skeleton"), opt)) {
        // Same as getInstanceForSkeleton(), see ICU 9029
        // Based on test/intltest/dtfmttst.cpp - TestPatterns()
        LocalPointer<DateTimePatternGenerator> generator(DateTimePatternGenerator::createInstance(locale, errorCode));
        UnicodeString pattern = generator->getBestPattern(opt, errorCode);
        df.adoptInstead(new SimpleDateFormat(pattern, locale, errorCode));
    } else {
        if (context.getStringOption(UnicodeString("pattern"), opt)) {
            df.adoptInstead(new SimpleDateFormat(opt, locale, errorCode));
        } else {
            DateFormat::EStyle dateStyle = DateFormat::NONE;
            if (context.getStringOption(UnicodeString("datestyle"), opt)) {
                dateStyle = stringToStyle(opt, errorCode);
            }
            DateFormat::EStyle timeStyle = DateFormat::NONE;
            if (context.getStringOption(UnicodeString("timestyle"), opt)) {
                timeStyle = stringToStyle(opt, errorCode);
            }
            if (dateStyle == DateFormat::NONE && timeStyle == DateFormat::NONE) {
                df.adoptInstead(defaultDateTimeInstance(locale, errorCode));
            } else {
                df.adoptInstead(DateFormat::createDateTimeInstance(dateStyle, timeStyle, locale));
                if (!df.isValid()) {
                    errorCode = U_MEMORY_ALLOCATION_ERROR;
                    return;
                }
            }
        }
    }

    CHECK_ERROR(errorCode);

    UnicodeString result;
    df->format(context.getFormattableInput(), result, 0, errorCode);
    context.setOutput(result);
}

StandardFunctions::DateTimeFactory::~DateTimeFactory() {}
StandardFunctions::DateTime::~DateTime() {}

// --------- TextFactory

Selector* StandardFunctions::TextFactory::createSelector(const Locale& locale, UErrorCode& errorCode) const {
    Selector* result = new TextSelector(locale);
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return nullptr;
    }
    return result;
}

void StandardFunctions::TextSelector::selectKey(FormattingContext& context,
                                                const UnicodeString* keys,
                                                int32_t keysLen,
                                                UnicodeString* prefs,
                                                int32_t& prefsLen,
						UErrorCode& errorCode) const {
    CHECK_ERROR(errorCode);

    // Just compares the key and value as strings

    // Argument must be present
    if (!context.hasFormattableInput()) {
        context.setSelectorError(UnicodeString("select"), errorCode);
        return;
    }

    prefsLen = 0;
    // Convert to string
    context.formatToString(locale, errorCode);
    CHECK_ERROR(errorCode);
    if (!context.hasStringOutput()) {
        return;
    }

    const UnicodeString& formattedValue = context.getStringOutput();

    for (int32_t i = 0; i < keysLen; i++) {
        if (keys[i] == formattedValue) {
	    prefs[0] = keys[i];
            prefsLen = 1;
            break;
        }
    }
}

StandardFunctions::TextFactory::~TextFactory() {}
StandardFunctions::TextSelector::~TextSelector() {}

// --------- IdentityFactory

Formatter* StandardFunctions::IdentityFactory::createFormatter(const Locale& locale, UErrorCode& errorCode) {
    Formatter* result = new Identity(locale);
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return nullptr;
    }
    return result;

}

void StandardFunctions::Identity::format(FormattingContext& context, UErrorCode& errorCode) const {
    CHECK_ERROR(errorCode);

    // Argument must be present
    if (!context.hasFormattableInput()) {
        context.setFormattingError(UnicodeString("text"), errorCode);
        return;
    }

    // Just returns the input value as a string
    context.formatToString(locale, errorCode);
}

StandardFunctions::IdentityFactory::~IdentityFactory() {}
StandardFunctions::Identity::~Identity() {}

} // namespace message2
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

