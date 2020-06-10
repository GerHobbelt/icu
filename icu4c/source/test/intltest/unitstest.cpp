// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include <cmath>
#include <iostream>

#include "charstr.h"
#include "cmemory.h"
#include "filestrm.h"
#include "intltest.h"
#include "number_decimalquantity.h"
#include "putilimp.h"
#include "unicode/ctest.h"
#include "unicode/measunit.h"
#include "unicode/unistr.h"
#include "unicode/unum.h"
#include "unicode/ures.h"
#include "unitconverter.h"
#include "unitsdata.h"
#include "unitsrouter.h"
#include "uparse.h"
#include "uresimp.h"

struct UnitConversionTestCase {
    const StringPiece source;
    const StringPiece target;
    const double inputValue;
    const double expectedValue;
};

using ::icu::number::impl::DecimalQuantity;
using namespace ::icu::units;

class UnitsTest : public IntlTest {
  public:
    UnitsTest() {}

    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par = NULL);

    void testUnitConstantFreshness();
    void testConversionCapability();
    void testConversions();
    void testPreferences();
    void testSiPrefixes();
    void testMass();
    void testTemperature();
    void testArea();
};

extern IntlTest *createUnitsTest() { return new UnitsTest(); }

void UnitsTest::runIndexedTest(int32_t index, UBool exec, const char *&name, char * /*par*/) {
    if (exec) {
        logln("TestSuite UnitsTest: ");
    }
    TESTCASE_AUTO_BEGIN;
    TESTCASE_AUTO(testUnitConstantFreshness);
    TESTCASE_AUTO(testConversionCapability);
    TESTCASE_AUTO(testConversions);
    TESTCASE_AUTO(testPreferences);
    TESTCASE_AUTO(testSiPrefixes);
    TESTCASE_AUTO(testMass);
    TESTCASE_AUTO(testTemperature);
    TESTCASE_AUTO(testArea);
    TESTCASE_AUTO_END;
}

// Tests the hard-coded constants in the code against constants that appear in
// units.txt.
void UnitsTest::testUnitConstantFreshness() {
    IcuTestErrorCode status(*this, "testUnitConstantFreshness");
    LocalUResourceBundlePointer unitsBundle(ures_openDirect(NULL, "units", status));
    LocalUResourceBundlePointer unitConstants(
        ures_getByKey(unitsBundle.getAlias(), "unitConstants", NULL, status));

    while (ures_hasNext(unitConstants.getAlias())) {
        int32_t len;
        const char *constant = NULL;
        ures_getNextString(unitConstants.getAlias(), &len, &constant, status);

        Factor factor;
        addSingleFactorConstant(constant, 2, POSITIVE, factor, status);
        if (status.errDataIfFailureAndReset(
                "addSingleFactorConstant(<%s>, ...).\n\n"
                "If U_INVALID_FORMAT_ERROR, please check that \"icu4c/source/i18n/unitconverter.cpp\" "
                "has all constants? Is \"%s\" a new constant?\n",
                constant, constant)) {
            continue;
        }
        // TODO(units,hugovdm): implement some symbolic maths to evaluate the
        // values of these constants? Counter-argument: constant values don't
        // change, and the data-driven unit tests generally take care of
        // validating the precision of conversions, if they have enough
        // coverage.
    }
}

void UnitsTest::testConversionCapability() {
    struct TestCase {
        const char *const source;
        const char *const target;
        const UnitsConvertibilityState expectedState;
    } testCases[]{
        {"meter", "foot", CONVERTIBLE},                                         //
        {"kilometer", "foot", CONVERTIBLE},                                     //
        {"hectare", "square-foot", CONVERTIBLE},                                //
        {"kilometer-per-second", "second-per-meter", RECIPROCAL},               //
        {"square-meter", "square-foot", CONVERTIBLE},                           //
        {"kilometer-per-second", "foot-per-second", CONVERTIBLE},               //
        {"square-hectare", "pow4-foot", CONVERTIBLE},                           //
        {"square-kilometer-per-second", "second-per-square-meter", RECIPROCAL}, //
    };

    for (const auto &testCase : testCases) {
        UErrorCode status = U_ZERO_ERROR;

        MeasureUnit source = MeasureUnit::forIdentifier(testCase.source, status);
        MeasureUnit target = MeasureUnit::forIdentifier(testCase.target, status);

        ConversionRates conversionRates(status);
        auto convertibility = checkConvertibility(source, target, conversionRates, status);

        assertEquals(UnicodeString("Conversion Capability: ") + testCase.source + " to " +
                         testCase.target,
                     testCase.expectedState, convertibility);
    }
}

void UnitsTest::testSiPrefixes() {
    IcuTestErrorCode status(*this, "Units testSiPrefixes");
    // Test Cases
    struct TestCase {
        const char *source;
        const char *target;
        const double inputValue;
        const double expectedValue;
    } testCases[]{
        {"gram", "kilogram", 1.0, 0.001},            //
        {"milligram", "kilogram", 1.0, 0.000001},    //
        {"microgram", "kilogram", 1.0, 0.000000001}, //
        {"megagram", "gram", 1.0, 1000000},          //
        {"megagram", "kilogram", 1.0, 1000},         //
        {"gigabyte", "byte", 1.0, 1000000000},       //
        // TODO: Fix `watt` probelms.
        // {"megawatt", "watt", 1.0, 1000000},          //
        // {"megawatt", "kilowatt", 1.0, 1000},         //
    };

    for (const auto &testCase : testCases) {
        UErrorCode status = U_ZERO_ERROR;

        MeasureUnit source = MeasureUnit::forIdentifier(testCase.source, status);
        MeasureUnit target = MeasureUnit::forIdentifier(testCase.target, status);

        ConversionRates conversionRates(status);
        UnitConverter converter(source, target, conversionRates, status);

        assertEqualsNear(UnicodeString("testSiPrefixes: ") + testCase.source + " to " + testCase.target,
                         testCase.expectedValue, converter.convert(testCase.inputValue),
                         0.0001 * testCase.expectedValue);
    }
}

void UnitsTest::testMass() {
    IcuTestErrorCode status(*this, "Units testMass");

    // Test Cases
    struct TestCase {
        const char *source;
        const char *target;
        const double inputValue;
        const double expectedValue;
    } testCases[]{
        {"gram", "kilogram", 1.0, 0.001},      //
        {"pound", "kilogram", 1.0, 0.453592},  //
        {"pound", "kilogram", 2.0, 0.907185},  //
        {"ounce", "pound", 16.0, 1.0},         //
        {"ounce", "kilogram", 16.0, 0.453592}, //
        {"ton", "pound", 1.0, 2000},           //
        {"stone", "pound", 1.0, 14},           //
        {"stone", "kilogram", 1.0, 6.35029}    //
    };

    for (const auto &testCase : testCases) {
        UErrorCode status = U_ZERO_ERROR;

        MeasureUnit source = MeasureUnit::forIdentifier(testCase.source, status);
        MeasureUnit target = MeasureUnit::forIdentifier(testCase.target, status);

        ConversionRates conversionRates(status);
        UnitConverter converter(source, target, conversionRates, status);

        assertEqualsNear(UnicodeString("testMass: ") + testCase.source + " to " + testCase.target,
                         testCase.expectedValue, converter.convert(testCase.inputValue),
                         0.0001 * testCase.expectedValue);
    }
}

void UnitsTest::testTemperature() {
    IcuTestErrorCode status(*this, "Units testTemperature");
    // Test Cases
    struct TestCase {
        const char *source;
        const char *target;
        const double inputValue;
        const double expectedValue;
    } testCases[]{
        {"celsius", "fahrenheit", 0.0, 32.0},   //
        {"celsius", "fahrenheit", 10.0, 50.0},  //
        {"fahrenheit", "celsius", 32.0, 0.0},   //
        {"fahrenheit", "celsius", 89.6, 32},    //
        {"kelvin", "fahrenheit", 0.0, -459.67}, //
        {"kelvin", "fahrenheit", 300, 80.33},   //
        {"kelvin", "celsius", 0.0, -273.15},    //
        {"kelvin", "celsius", 300.0, 26.85}     //
    };

    for (const auto &testCase : testCases) {
        UErrorCode status = U_ZERO_ERROR;

        MeasureUnit source = MeasureUnit::forIdentifier(testCase.source, status);
        MeasureUnit target = MeasureUnit::forIdentifier(testCase.target, status);

        ConversionRates conversionRates(status);
        UnitConverter converter(source, target, conversionRates, status);

        assertEqualsNear(UnicodeString("testTemperature: ") + testCase.source + " to " + testCase.target,
                         testCase.expectedValue, converter.convert(testCase.inputValue),
                         0.0001 * uprv_fabs(testCase.expectedValue));
    }
}

void UnitsTest::testArea() {
    IcuTestErrorCode status(*this, "Units Area");

    // Test Cases
    struct TestCase {
        const char *source;
        const char *target;
        const double inputValue;
        const double expectedValue;
    } testCases[]{
        {"square-meter", "square-yard", 10.0, 11.9599},     //
        {"hectare", "square-yard", 1.0, 11959.9},           //
        {"square-mile", "square-foot", 0.0001, 2787.84},    //
        {"hectare", "square-yard", 1.0, 11959.9},           //
        {"hectare", "square-meter", 1.0, 10000},            //
        {"hectare", "square-meter", 0.0, 0.0},              //
        {"square-mile", "square-foot", 0.0001, 2787.84},    //
        {"square-yard", "square-foot", 10, 90},             //
        {"square-yard", "square-foot", 0, 0},               //
        {"square-yard", "square-foot", 0.000001, 0.000009}, //
        {"square-mile", "square-foot", 0.0, 0.0},           //
    };

    for (const auto &testCase : testCases) {
        UErrorCode status = U_ZERO_ERROR;

        MeasureUnit source = MeasureUnit::forIdentifier(testCase.source, status);
        MeasureUnit target = MeasureUnit::forIdentifier(testCase.target, status);

        ConversionRates conversionRates(status);
        UnitConverter converter(source, target, conversionRates, status);

        assertEqualsNear(UnicodeString("testArea: ") + testCase.source + " to " + testCase.target,
                         testCase.expectedValue, converter.convert(testCase.inputValue),
                         0.0001 * testCase.expectedValue);
    }
}

/**
 * Trims whitespace (spaces only) off of the specified string.
 * @param field is two pointers pointing at the start and end of the string.
 * @return A StringPiece with initial and final space characters trimmed off.
 */
StringPiece trimField(char *(&field)[2]) {
    char *start = field[0];
    while (start < field[1] && (start[0]) == ' ') {
        start++;
    }
    int32_t length = (int32_t)(field[1] - start);
    while (length > 0 && (start[length - 1]) == ' ') {
        length--;
    }
    return StringPiece(start, length);
}

// Used for passing context to unitsTestDataLineFn via u_parseDelimitedFile.
struct UnitsTestContext {
    // Provides access to UnitsTest methods like logln.
    UnitsTest *unitsTest;
    // Conversion rates: does not take ownership.
    ConversionRates *conversionRates;
};

/**
 * Deals with a single data-driven unit test for unit conversions.
 *
 * This is a UParseLineFn as required by u_parseDelimitedFile, intended for
 * parsing unitsTest.txt.
 *
 * @param context Must point at a UnitsTestContext struct.
 * @param fields A list of pointer-pairs, each pair pointing at the start and
 * end of each field. End pointers are important because these are *not*
 * null-terminated strings. (Interpreted as a null-terminated string,
 * fields[0][0] points at the whole line.)
 * @param fieldCount The number of fields (pointer pairs) passed to the fields
 * parameter.
 * @param pErrorCode Receives status.
 */
void unitsTestDataLineFn(void *context, char *fields[][2], int32_t fieldCount, UErrorCode *pErrorCode) {
    if (U_FAILURE(*pErrorCode)) {
        return;
    }
    UnitsTestContext *ctx = (UnitsTestContext *)context;
    UnitsTest *unitsTest = ctx->unitsTest;
    (void)fieldCount; // unused UParseLineFn variable
    IcuTestErrorCode status(*unitsTest, "unitsTestDatalineFn");

    StringPiece quantity = trimField(fields[0]);
    StringPiece x = trimField(fields[1]);
    StringPiece y = trimField(fields[2]);
    StringPiece commentConversionFormula = trimField(fields[3]);
    StringPiece utf8Expected = trimField(fields[4]);

    UNumberFormat *nf = unum_open(UNUM_DEFAULT, NULL, -1, "en_US", NULL, status);
    if (status.errIfFailureAndReset("unum_open failed")) {
        return;
    }
    UnicodeString uExpected = UnicodeString::fromUTF8(utf8Expected);
    double expected = unum_parseDouble(nf, uExpected.getBuffer(), uExpected.length(), 0, status);
    unum_close(nf);
    if (status.errIfFailureAndReset("unum_parseDouble(\"%s\") failed", utf8Expected)) {
        return;
    }

    MeasureUnit sourceUnit = MeasureUnit::forIdentifier(x, status);
    if (status.errIfFailureAndReset("forIdentifier(\"%.*s\")", x.length(), x.data())) {
        return;
    }

    MeasureUnit targetUnit = MeasureUnit::forIdentifier(y, status);
    if (status.errIfFailureAndReset("forIdentifier(\"%.*s\")", y.length(), y.data())) {
        return;
    }

    unitsTest->logln("Quantity (Category): \"%.*s\", "
                     "Expected value of \"1000 %.*s in %.*s\": %f, "
                     "commentConversionFormula: \"%.*s\", ",
                     quantity.length(), quantity.data(), x.length(), x.data(), y.length(), y.data(),
                     expected, commentConversionFormula.length(), commentConversionFormula.data());

    // Convertibility:
    auto convertibility = checkConvertibility(sourceUnit, targetUnit, *ctx->conversionRates, status);
    if (status.errIfFailureAndReset("checkConvertibility(<%s>, <%s>, ...)", sourceUnit.getIdentifier(),
                                    targetUnit.getIdentifier())) {
        return;
    }
    CharString msg;
    msg.append("convertible: ", status)
        .append(sourceUnit.getIdentifier(), status)
        .append(" -> ", status)
        .append(targetUnit.getIdentifier(), status);
    if (status.errIfFailureAndReset("msg construction")) {
        return;
    }
    unitsTest->assertNotEquals(msg.data(), UNCONVERTIBLE, convertibility);

    // Conversion:
    UnitConverter converter(sourceUnit, targetUnit, *ctx->conversionRates, status);
    if (status.errIfFailureAndReset("constructor: UnitConverter(<%s>, <%s>, status)",
                                    sourceUnit.getIdentifier(), targetUnit.getIdentifier())) {
        return;
    }
    double got = converter.convert(1000);
    msg.clear();
    msg.append("Converting 1000 ", status).append(x, status).append(" to ", status).append(y, status);
    unitsTest->assertEqualsNear(msg.data(), expected, got, 0.0001 * expected);
}

/**
 * Runs data-driven unit tests for unit conversion. It looks for the test cases
 * in source/test/testdata/units/unitsTest.txt, which originates in CLDR.
 */
void UnitsTest::testConversions() {
    const char *filename = "unitsTest.txt";
    const int32_t kNumFields = 5;
    char *fields[kNumFields][2];

    IcuTestErrorCode errorCode(*this, "UnitsTest::testConversions");
    const char *sourceTestDataPath = getSourceTestData(errorCode);
    if (errorCode.errIfFailureAndReset("unable to find the source/test/testdata "
                                       "folder (getSourceTestData())")) {
        return;
    }

    CharString path(sourceTestDataPath, errorCode);
    path.appendPathPart("units", errorCode);
    path.appendPathPart(filename, errorCode);

    ConversionRates rates(errorCode);
    UnitsTestContext ctx = {this, &rates};
    u_parseDelimitedFile(path.data(), ';', fields, kNumFields, unitsTestDataLineFn, &ctx, errorCode);
    if (errorCode.errIfFailureAndReset("error parsing %s: %s\n", path.data(), u_errorName(errorCode))) {
        return;
    }
}

/**
 * This class represents the output fields from unitPreferencesTest.txt. Please
 * see the documentation at the top of that file for details.
 *
 * For "mixed units" output, there are more (repeated) output fields. The last
 * output unit has the expected output specified as both a rational fraction and
 * a decimal fraction. This class ignores rational fractions, and expects to
 * find a decimal fraction for each output unit.
 */
class ExpectedOutput {
  public:
    // Counts number of units in the output. When this is more than one, we have
    // "mixed units" in the expected output.
    int _compoundCount = 0;

    // Counts how many fields were skipped: we expect to skip only one per
    // output unit type (the rational fraction).
    int _skippedFields = 0;

    // The expected output units: more than one for "mixed units".
    MeasureUnit _measureUnits[3];

    // The amounts of each of the output units.
    double _amounts[3];

    /**
     * Parse an expected output field from the test data file.
     *
     * @param output may be a string representation of an integer, a rational
     * fraction, a decimal fraction, or it may be a unit identifier. Whitespace
     * should already be trimmed. This function ignores rational fractions,
     * saving only decimal fractions and their unit identifiers.
     * @return true if the field was successfully parsed, false if parsing
     * failed.
     */
    void parseOutputField(StringPiece output, UErrorCode &errorCode) {
        if (U_FAILURE(errorCode)) return;
        DecimalQuantity dqOutputD;

        dqOutputD.setToDecNumber(output, errorCode);
        if (U_SUCCESS(errorCode)) {
            _amounts[_compoundCount] = dqOutputD.toDouble();
            return;
        } else if (errorCode == U_DECIMAL_NUMBER_SYNTAX_ERROR) {
            // Not a decimal fraction, it might be a rational fraction or a unit
            // identifier: continue.
            errorCode = U_ZERO_ERROR;
        } else {
            // Unexpected error, so we propagate it.
            return;
        }

        _measureUnits[_compoundCount] = MeasureUnit::forIdentifier(output, errorCode);
        if (U_SUCCESS(errorCode)) {
            _compoundCount++;
            _skippedFields = 0;
            return;
        }
        _skippedFields++;
        if (_skippedFields < 2) {
            // We are happy skipping one field per output unit: we want to skip
            // rational fraction fields like "11 / 10".
            errorCode = U_ZERO_ERROR;
            return;
        } else {
            // Propagate the error.
            return;
        }
    }

    /**
     * Produces an output string for debug purposes.
     */
    std::string toDebugString() {
        std::string result;
        for (int i = 0; i < _compoundCount; i++) {
            result += std::to_string(_amounts[i]);
            result += " ";
            result += _measureUnits[i].getIdentifier();
            result += " ";
        }
        return result;
    }
};

// Checks a vector of Measure instances against ExpectedOutput.
void checkOutput(UnitsTest *unitsTest, const char *msg, ExpectedOutput expected,
                 const MaybeStackVector<Measure> &actual, double precision) {
    IcuTestErrorCode status(*unitsTest, "checkOutput");

    CharString testMessage("Test case \"", status);
    testMessage.append(msg, status);
    testMessage.append("\": expected output: ", status);
    testMessage.append(expected.toDebugString().c_str(), status);
    testMessage.append(", obtained output:", status);
    for (int i = 0; i < actual.length(); i++) {
        testMessage.append(" ", status);
        testMessage.append(std::to_string(actual[i]->getNumber().getDouble(status)), status);
        testMessage.append(" ", status);
        testMessage.appendInvariantChars(actual[i]->getUnit().getIdentifier(), status);
    }
    if (!unitsTest->assertEquals(testMessage.data(), expected._compoundCount, actual.length())) {
        return;
    };
    for (int i = 0; i < actual.length(); i++) {
        double permittedDiff = precision * expected._amounts[i];
        if (permittedDiff == 0) {
            // If 0 is expected, still permit a small delta.
            // TODO: revisit this experimentally chosen value:
            permittedDiff = 0.00000001;
        }
        unitsTest->assertEqualsNear(testMessage.data(), expected._amounts[i],
                                    actual[i]->getNumber().getDouble(status), permittedDiff);
    }
}

/**
 * Runs a single data-driven unit test for unit preferences.
 *
 * This is a UParseLineFn as required by u_parseDelimitedFile, intended for
 * parsing unitPreferencesTest.txt.
 */
void unitPreferencesTestDataLineFn(void *context, char *fields[][2], int32_t fieldCount,
                                   UErrorCode *pErrorCode) {
    if (U_FAILURE(*pErrorCode)) return;
    UnitsTest *unitsTest = (UnitsTest *)context;
    IcuTestErrorCode status(*unitsTest, "unitPreferencesTestDatalineFn");

    if (!unitsTest->assertTrue(u"unitPreferencesTestDataLineFn expects 9 fields for simple and 11 "
                               u"fields for compound. Other field counts not yet supported. ",
                               fieldCount == 9 || fieldCount == 11)) {
        return;
    }

    StringPiece quantity = trimField(fields[0]);
    StringPiece usage = trimField(fields[1]);
    StringPiece region = trimField(fields[2]);
    // Unused // StringPiece inputR = trimField(fields[3]);
    StringPiece inputD = trimField(fields[4]);
    StringPiece inputUnit = trimField(fields[5]);
    ExpectedOutput expected;
    for (int i = 6; i < fieldCount; i++) {
        expected.parseOutputField(trimField(fields[i]), status);
    }
    if (status.errIfFailureAndReset("parsing unitPreferencesTestData.txt test case: %s", fields[0][0])) {
        return;
    }

    DecimalQuantity dqInputD;
    dqInputD.setToDecNumber(inputD, status);
    if (status.errIfFailureAndReset("parsing decimal quantity: \"%.*s\"", inputD.length(),
                                    inputD.data())) {
        return;
    }
    double inputAmount = dqInputD.toDouble();

    MeasureUnit inputMeasureUnit = MeasureUnit::forIdentifier(inputUnit, status);
    if (status.errIfFailureAndReset("forIdentifier(\"%.*s\")", inputUnit.length(), inputUnit.data())) {
        return;
    }

    unitsTest->logln("Quantity (Category): \"%.*s\", Usage: \"%.*s\", Region: \"%.*s\", "
                     "Input: \"%f %s\", Expected Output: %s",
                     quantity.length(), quantity.data(), usage.length(), usage.data(), region.length(),
                     region.data(), inputAmount, inputMeasureUnit.getIdentifier(),
                     expected.toDebugString().c_str());

    if (U_FAILURE(status)) {
        return;
    }
    UnitsRouter router(inputMeasureUnit, region, usage, status);
    if (status.errIfFailureAndReset("UnitsRouter(<%s>, \"%.*s\", \"%.*s\", status)",
                                    inputMeasureUnit.getIdentifier(), region.length(), region.data(),
                                    usage.length(), usage.data())) {
        return;
    }

    CharString msg(quantity, status);
    msg.append(" ", status);
    msg.append(usage, status);
    msg.append(" ", status);
    msg.append(region, status);
    msg.append(" ", status);
    msg.append(inputD, status);
    msg.append(" ", status);
    msg.append(inputMeasureUnit.getIdentifier(), status);
    if (status.errIfFailureAndReset("Failure before router.route")) {
        return;
    }
    MaybeStackVector<Measure> result = router.route(inputAmount, status);
    if (status.errIfFailureAndReset("router.route(inputAmount, ...)")) {
        return;
    }
    // TODO: revisit this experimentally chosen precision:
    checkOutput(unitsTest, msg.data(), expected, result, 0.0000000001);
}

/**
 * Parses the format used by unitPreferencesTest.txt, calling lineFn for each
 * line.
 *
 * This is a modified version of u_parseDelimitedFile, customized for
 * unitPreferencesTest.txt, due to it having a variable number of fields per
 * line.
 */
void parsePreferencesTests(const char *filename, char delimiter, char *fields[][2],
                           int32_t maxFieldCount, UParseLineFn *lineFn, void *context,
                           UErrorCode *pErrorCode) {
    FileStream *file;
    char line[10000];
    char *start, *limit;
    int32_t i;

    if (U_FAILURE(*pErrorCode)) {
        return;
    }

    if (fields == NULL || lineFn == NULL || maxFieldCount <= 0) {
        *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    if (filename == NULL || *filename == 0 || (*filename == '-' && filename[1] == 0)) {
        filename = NULL;
        file = T_FileStream_stdin();
    } else {
        file = T_FileStream_open(filename, "r");
    }
    if (file == NULL) {
        *pErrorCode = U_FILE_ACCESS_ERROR;
        return;
    }

    while (T_FileStream_readLine(file, line, sizeof(line)) != NULL) {
        /* remove trailing newline characters */
        u_rtrim(line);

        start = line;
        *pErrorCode = U_ZERO_ERROR;

        /* skip this line if it is empty or a comment */
        if (*start == 0 || *start == '#') {
            continue;
        }

        /* remove in-line comments */
        limit = uprv_strchr(start, '#');
        if (limit != NULL) {
            /* get white space before the pound sign */
            while (limit > start && U_IS_INV_WHITESPACE(*(limit - 1))) {
                --limit;
            }

            /* truncate the line */
            *limit = 0;
        }

        /* skip lines with only whitespace */
        if (u_skipWhitespace(start)[0] == 0) {
            continue;
        }

        /* for each field, call the corresponding field function */
        for (i = 0; i < maxFieldCount; ++i) {
            /* set the limit pointer of this field */
            limit = start;
            while (*limit != delimiter && *limit != 0) {
                ++limit;
            }

            /* set the field start and limit in the fields array */
            fields[i][0] = start;
            fields[i][1] = limit;

            /* set start to the beginning of the next field, if any */
            start = limit;
            if (*start != 0) {
                ++start;
            } else {
                break;
            }
        }
        if (i == maxFieldCount) {
            *pErrorCode = U_PARSE_ERROR;
        }
        int fieldCount = i + 1;

        /* call the field function */
        lineFn(context, fields, fieldCount, pErrorCode);
        if (U_FAILURE(*pErrorCode)) {
            break;
        }
    }

    if (filename != NULL) {
        T_FileStream_close(file);
    }
}

/**
 * Runs data-driven unit tests for unit preferences.
 */
void UnitsTest::testPreferences() {
    const char *filename = "unitPreferencesTest.txt";
    const int32_t maxFields = 11;
    char *fields[maxFields][2];

    IcuTestErrorCode errorCode(*this, "UnitsTest::testPreferences");
    const char *sourceTestDataPath = getSourceTestData(errorCode);
    if (errorCode.errIfFailureAndReset("unable to find the source/test/testdata "
                                       "folder (getSourceTestData())")) {
        return;
    }

    CharString path(sourceTestDataPath, errorCode);
    path.appendPathPart("units", errorCode);
    path.appendPathPart(filename, errorCode);

    parsePreferencesTests(path.data(), ';', fields, maxFields, unitPreferencesTestDataLineFn, this,
                          errorCode);
    if (errorCode.errIfFailureAndReset("error parsing %s: %s\n", path.data(), u_errorName(errorCode))) {
        return;
    }
}

#endif /* #if !UCONFIG_NO_FORMATTING */
