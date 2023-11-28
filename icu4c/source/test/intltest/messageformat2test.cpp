// © 2016 and later: Unicode, Inc. and others.

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "messageformat2test.h"

using namespace icu::message2;

/*
  TODO: Tests need to be unified in a single format that
  both ICU4C and ICU4J can use, rather than being embedded in code.

  Tests are included in their current state to give a sense of
  how much test coverage has been achieved. Most of the testing is
  of the parser/serializer; the formatter needs to be tested more
  thoroughly.
*/

/*
Tests reflect the syntax specified in

  https://github.com/unicode-org/message-format-wg/commits/main/spec/message.abnf

as of the following commit from 2023-05-09:
  https://github.com/unicode-org/message-format-wg/commit/194f6efcec5bf396df36a19bd6fa78d1fa2e0867

*/

static const int32_t numValidTestCases = 25;
TestResult validTestCases[] = {
    {"{hello {|4.2| :number}}", "hello 4.2"},
    {"{hello {|4.2| :number minimumFractionDigits=2}}", "hello 4.20"},
    {"{hello {|4.2| :number minimumFractionDigits = 2}}", "hello 4.20"},
    {"{hello {|4.2| :number minimumFractionDigits= 2}}", "hello 4.20"},
    {"{hello {|4.2| :number minimumFractionDigits =2}}", "hello 4.20"},
    {"{hello {|4.2| :number minimumFractionDigits=2  }}", "hello 4.20"},
    {"{hello {|4.2| :number minimumFractionDigits=2 bar=3}}", "hello 4.20"},
    {"{hello {|4.2| :number minimumFractionDigits=2 bar=3  }}", "hello 4.20"},
    {"{hello {|4.2| :number minimumFractionDigits=|2|}}", "hello 4.20"},
    {"{content -tag}", "content -tag"},
    {"{}", ""},
    // tests for escape sequences in literals
    {"{{|hel\\\\lo|}}", "hel\\lo"},
    {"{{|hel\\|lo|}}", "hel|lo"},
    {"{{|hel\\|\\\\lo|}}", "hel|\\lo"},
    // tests for text escape sequences
    {"{hel\\{lo}", "hel{lo"},
    {"{hel\\}lo}", "hel}lo"},
    {"{hel\\\\lo}", "hel\\lo"},
    {"{hel\\{\\\\lo}", "hel{\\lo"},
    {"{hel\\{\\}lo}", "hel{}lo"},
    // tests for ':' in unquoted literals
    {"match {|foo| :select} when o:ne {one} when * {other}", "other"},
    {"match {|foo| :select} when one: {one} when * {other}", "other"},
    {"let $foo = {|42| :number option=a:b} {bar {$foo}}", "bar 42"},
    {"let $foo = {|42| :number option=a:b:c} {bar {$foo}}", "bar 42"},
    // tests for newlines in literals and text
    {"{hello {|wo\nrld|}}", "hello wo\nrld"},
    {"{hello wo\nrld}", "hello wo\nrld"}
};


static const int32_t numResolutionErrors = 6;
TestResultError jsonTestCasesResolutionError[] = {
    {"let $foo = {$bar} match {$foo :plural} when one {one} when * {other}", "other", U_UNRESOLVED_VARIABLE_ERROR},
    {"let $foo = {$bar} match {$foo :plural} when one {one} when * {other}", "other", U_UNRESOLVED_VARIABLE_ERROR},
    {"let $bar = {$none :plural} match {$foo :select} when one {one} when * {{$bar}}", "{$none}", U_UNRESOLVED_VARIABLE_ERROR},
    {"{{|content| +tag}}", "{|content|}", U_UNKNOWN_FUNCTION_ERROR},
    {"{{|content| -tag}}", "{|content|}", U_UNKNOWN_FUNCTION_ERROR},
    {"{{|content| +tag} {|content| -tag}}", "{|content|} {|content|}", U_UNKNOWN_FUNCTION_ERROR},
    {"{content {|foo| +markup}}", "content {|foo|}", U_UNKNOWN_FUNCTION_ERROR}
};

static const int32_t numReservedErrors = 34;
UnicodeString reservedErrors[] = {
    // tests for reserved syntax
    "{hello {|4.2| @number}}",
    "{hello {|4.2| @n|um|ber}}",
    "{hello {|4.2| &num|be|r}}",
    "{hello {|4.2| ?num|be||r|s}}",
    "{hello {|foo| !number}}",
    "{hello {|foo| *number}}",
    "{hello {#number}}",
    "{{<tag}}",
    "let $bar = {$none ~plural} match {$foo :select} when * {{$bar}}",
    // tests for reserved syntax with escaped chars
    "{hello {|4.2| @num\\\\ber}}",
    "{hello {|4.2| @num\\{be\\|r}}",
    "{hello {|4.2| @num\\\\\\}ber}}",
    // tests for reserved syntax
    "{hello {|4.2| @}}",
    "{hello {|4.2| #}}",
    "{hello {|4.2| *}}",
    "{hello {|4.2| ^abc|123||5|\\\\}}",
    "{hello {|4.2| ^ abc|123||5|\\\\}}",
    "{hello {|4.2| ^ abc|123||5|\\\\ \\|def |3.14||2|}}",
    // tests for reserved syntax with trailing whitespace
    "{hello {|4.2| ? }}",
    "{hello {|4.2| @xyzz }}",
    "{hello {|4.2| !xyzz   }}",
    "{hello {$foo ~xyzz }}",
    "{hello {$x   <xyzz   }}",
    "{{@xyzz }}",
    "{{  !xyzz   }}",
    "{{~xyzz }}",
    "{{ <xyzz   }}",
    // tests for reserved syntax with space-separated sequences
    "{hello {|4.2| @xy z z }}",
    "{hello {|4.2| *num \\\\ b er}}",
    "{hello {|4.2| %num \\\\ b |3.14| r    }}",
    "{hello {|4.2|    #num xx \\\\ b |3.14| r  }}",
    "{hello {$foo    #num x \\\\ abcde |3.14| r  }}",
    "{hello {$foo    >num x \\\\ abcde |aaa||3.14||42| r  }}",
    "{hello {$foo    >num x \\\\ abcde |aaa||3.14| |42| r  }}",
    0
};

static const int32_t numMatches = 15;
UnicodeString matches[] = {
    // multiple scrutinees, with or without whitespace
    "match {$foo :select} {$bar :select} when one * {one} when * * {other}",
    "match {$foo :select} {$bar :select}when one * {one} when * * {other}",
    "match {$foo :select}{$bar :select} when one * {one} when * * {other}",
    "match {$foo :select}{$bar :select}when one * {one} when * * {other}",
    "match{$foo :select} {$bar :select} when one * {one} when * * {other}",
    "match{$foo :select} {$bar :select}when one * {one} when * * {other}",
    "match{$foo :select}{$bar :select} when one * {one} when * * {other}",
    "match{$foo :select}{$bar :select}when one * {one} when * * {other}",
    // multiple variants, with or without whitespace
    "match {$foo :select} {$bar :select} when one * {one} when * * {other}",
    "match {$foo :select} {$bar :select} when one * {one}when * * {other}",
    "match {$foo :select} {$bar :select}when one * {one} when * * {other}",
    "match {$foo :select} {$bar :select}when one * {one}when * * {other}",
    // one or multiple keys, with or without whitespace before pattern
    "match {$foo :select} {$bar :select} when one *{one} when * * {foo}",
    "match {$foo :select} {$bar :select} when one * {one} when * * {foo}",
    "match {$foo :select} {$bar :select} when one *  {one} when * * {foo}"
};

static const int32_t numSyntaxTests = 22;
// These patterns are tested to ensure they parse without a syntax error
UnicodeString syntaxTests[] = {
    "{hello {|foo| :number   }}",
    // zero, one or multiple options, with or without whitespace before '}'
    "{{:foo}}",
    "{{:foo }}",
    "{{:foo   }}",
    "{{:foo k=v}}",
    "{{:foo k=v   }}",
    "{{:foo k1=v1   k2=v2}}",
    "{{:foo k1=v1   k2=v2   }}",
    // literals or variables followed by space, with or without an annotation following
    "{{|3.14| }}",
    "{{|3.14|    }}",
    "{{|3.14|    :foo}}",
    "{{|3.14|    :foo   }}",
    "{{$bar }}",
    "{{$bar    }}",
    "{{$bar    :foo}}",
    "{{$bar    :foo   }}",
    // Trailing whitespace at end of message should be accepted
    "match {$foo :select} {$bar :select} when one * {one} when * * {other}   ",
    "{hi} ",
    // Variable names can contain '-' or ':'
    "{{$bar:foo}}",
    "{{$bar-foo}}",
    // Name shadowing is allowed
    "let $foo = {|hello|} let $foo = {$foo} {{$foo}}",
    // Unquoted literal -- should work
    "{good {placeholder}}",
    0
};

void
TestMessageFormat2::runIndexedTest(int32_t index, UBool exec,
                                  const char* &name, char* /*par*/) {
    TESTCASE_AUTO_BEGIN;
    TESTCASE_AUTO(featureTests);
    TESTCASE_AUTO(messageFormat1Tests);
    TESTCASE_AUTO(testAPICustomFunctions);
    TESTCASE_AUTO(testCustomFunctions);
    TESTCASE_AUTO(testBuiltInFunctions);
    TESTCASE_AUTO(testDataModelErrors);
    TESTCASE_AUTO(testResolutionErrors);
    TESTCASE_AUTO(testAPI);
    TESTCASE_AUTO(testAPISimple);
    TESTCASE_AUTO(testVariousPatterns);
    TESTCASE_AUTO(testInvalidPatterns);
    TESTCASE_AUTO_END;
}

// Example for design doc -- version without null and error checks
void TestMessageFormat2::testAPISimple() {
    IcuTestErrorCode errorCode1(*this, "testAPI");
    UErrorCode errorCode = (UErrorCode) errorCode1;
    UParseError parseError;
    Locale locale = "en_US";

    // Since this is the example used in the
    // design doc, it elides null checks and error checks.
    // To be used in the test suite, it should include those checks
    // Null checks and error checks elided
    MessageFormatter::Builder* builder = MessageFormatter::builder(errorCode);
    MessageFormatter* mf = builder->setPattern(u"{Hello, {$userName}!}")
        .build(parseError, errorCode);

    MessageArguments::Builder argsBuilder = MessageArguments::Builder();
    argsBuilder.add("userName", "John");
    MessageArguments args = argsBuilder.build();

    UnicodeString result;
    result = mf->formatToString(args, errorCode);
    assertEquals("testAPI", result, "Hello, John!");

    delete mf;
    mf = builder->setPattern("{Today is {$today :datetime skeleton=yMMMdEEE}.}")
        .setLocale(locale)
        .build(parseError, errorCode);

    Calendar* cal(Calendar::createInstance(errorCode));
    // Sunday, October 28, 2136 8:39:12 AM PST
    cal->set(2136, Calendar::OCTOBER, 28, 8, 39, 12);
    UDate date = cal->getTime(errorCode);

    argsBuilder.addDate("today", date);
    args = argsBuilder.build();
    result = mf->formatToString(args, errorCode);
    assertEquals("testAPI", "Today is Sun, Oct 28, 2136.", result);

    argsBuilder.addInt64("photoCount", 12);
    argsBuilder.add("userGender", "feminine");
    argsBuilder.add("userName", "Maria");
    args = argsBuilder.build();

    delete mf;
    mf = builder->setPattern("match {$photoCount :plural} {$userGender :select}\n\
                     when 1 masculine {{$userName} added a new photo to his album.}\n \
                     when 1 feminine {{$userName} added a new photo to her album.}\n \
                     when 1 * {{$userName} added a new photo to their album.}\n \
                     when * masculine {{$userName} added {$photoCount} photos to his album.}\n \
                     when * feminine {{$userName} added {$photoCount} photos to her album.}\n \
                     when * * {{$userName} added {$photoCount} photos to their album.}")
        .setLocale(locale)
        .build(parseError, errorCode);
    result = mf->formatToString(args, errorCode);
    assertEquals("testAPI", "Maria added 12 photos to her album.", result);

    delete builder;
    delete cal;
    delete mf;
}

// Design doc example, with more details
void TestMessageFormat2::testAPI() {
    IcuTestErrorCode errorCode(*this, "testAPI");
    TestCase::Builder testBuilder;

    // Pattern: "{Hello, {$userName}!}"
    TestCase test(testBuilder.setName("testAPI")
                  .setPattern("{Hello, {$userName}!}")
                  .setArgument("userName", "John")
                  .setExpected("Hello, John!")
                  .setLocale("en_US")
                  .build());
    TestUtils::runTestCase(*this, test, errorCode);

    // Pattern: "{Today is {$today ..."
    LocalPointer<Calendar> cal(Calendar::createInstance(errorCode));
    // Sunday, October 28, 2136 8:39:12 AM PST
    cal->set(2136, Calendar::OCTOBER, 28, 8, 39, 12);
    UDate date = cal->getTime(errorCode);

    test = testBuilder.setName("testAPI")
        .setPattern("{Today is {$today :datetime skeleton=yMMMdEEE}.}")
        .setDateArgument("today", date)
        .setExpected("Today is Sun, Oct 28, 2136.")
        .setLocale("en_US")
        .build();
    TestUtils::runTestCase(*this, test, errorCode);

    // Pattern matching - plural
    UnicodeString pattern = "match {$photoCount :select} {$userGender :select}\n\
                     when 1 masculine {{$userName} added a new photo to his album.}\n \
                     when 1 feminine {{$userName} added a new photo to her album.}\n \
                     when 1 * {{$userName} added a new photo to their album.}\n \
                     when * masculine {{$userName} added {$photoCount} photos to his album.}\n \
                     when * feminine {{$userName} added {$photoCount} photos to her album.}\n \
                     when * * {{$userName} added {$photoCount} photos to their album.}";


    int64_t photoCount = 12;
    test = testBuilder.setName("testAPI")
        .setPattern(pattern)
        .setArgument("photoCount", photoCount)
        .setArgument("userGender", "feminine")
        .setArgument("userName", "Maria")
        .setExpected("Maria added 12 photos to her album.")
        .setLocale("en_US")
        .build();
    TestUtils::runTestCase(*this, test, errorCode);

    // Built-in functions
    pattern = "match {$photoCount :plural} {$userGender :select}\n\
                     when 1 masculine {{$userName} added a new photo to his album.}\n \
                     when 1 feminine {{$userName} added a new photo to her album.}\n \
                     when 1 * {{$userName} added a new photo to their album.}\n \
                     when * masculine {{$userName} added {$photoCount} photos to his album.}\n \
                     when * feminine {{$userName} added {$photoCount} photos to her album.}\n \
                     when * * {{$userName} added {$photoCount} photos to their album.}";

    photoCount = 1;
    test = testBuilder.setName("testAPI")
        .setPattern(pattern)
        .setArgument("photoCount", photoCount)
        .setArgument("userGender", "feminine")
        .setArgument("userName", "Maria")
        .setExpected("Maria added a new photo to her album.")
        .setLocale("en_US")
        .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

// Custom functions example from the ICU4C API design doc
// Note: error/null checks are omitted
void TestMessageFormat2::testAPICustomFunctions() {
    IcuTestErrorCode errorCode1(*this, "testAPICustomFunctions");
    UErrorCode errorCode = (UErrorCode) errorCode1;
    UParseError parseError;
    Locale locale = "en_US";

    // Set up custom function registry
    FunctionRegistry::Builder* builder = FunctionRegistry::builder(errorCode);
    // Note that this doesn't use `setDefaultFormatterNameForType()`; not implemented yet
    FunctionRegistry* functionRegistry =
        builder->setFormatter(data_model::FunctionName("person"), new PersonNameFormatterFactory(), errorCode)
               .build(errorCode);

    Person* person = new Person(UnicodeString("Mr."), UnicodeString("John"), UnicodeString("Doe"));

    MessageArguments::Builder argsBuilder;
    argsBuilder.addObject("name", person);
    MessageArguments arguments = argsBuilder.build();

    MessageFormatter::Builder* mfBuilder = MessageFormatter::builder(errorCode);
    UnicodeString result;
    // This fails, because we did not provide a function registry:
    MessageFormatter* mf = mfBuilder->setPattern("{Hello {$name :person formality=informal}}")
                                    .setLocale(locale)
                                    .build(parseError, errorCode);
    result = mf->formatToString(arguments, errorCode);
    assertEquals("testAPICustomFunctions", U_UNKNOWN_FUNCTION_ERROR, errorCode);

    errorCode = U_ZERO_ERROR;
    mfBuilder->setFunctionRegistry(functionRegistry)
              .setLocale(locale);

    delete mf;
    mf = mfBuilder->setPattern("{Hello {$name :person formality=informal}}")
                    .build(parseError, errorCode);
    result = mf->formatToString(arguments, errorCode);
    assertEquals("testAPICustomFunctions", "Hello John", result);

    delete mf;
    mf = mfBuilder->setPattern("{Hello {$name :person formality=formal}}")
                    .build(parseError, errorCode);
    result = mf->formatToString(arguments, errorCode);
    assertEquals("testAPICustomFunctions", "Hello Mr. Doe", result);

    delete mf;
    mf = mfBuilder->setPattern("{Hello {$name :person formality=formal length=long}}")
                    .build(parseError, errorCode);
    result = mf->formatToString(arguments, errorCode);
    assertEquals("testAPICustomFunctions", "Hello Mr. John Doe", result);

    delete builder;
    delete functionRegistry;
    delete person;
    delete mf;
    delete mfBuilder;
}

void TestMessageFormat2::testValidPatterns(const TestResult* patterns, int32_t len, IcuTestErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    TestCase::Builder testBuilder;
    testBuilder.setName("testOtherJsonPatterns");

    for (int32_t i = 0; i < len - 1; i++) {
        TestUtils::runTestCase(*this, testBuilder.setPattern(patterns[i].pattern)
                               .setExpected(patterns[i].output)
                               .setExpectSuccess()
                               .build(), errorCode);
    }
}

void TestMessageFormat2::testResolutionErrors(IcuTestErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    TestCase::Builder testBuilder;
    testBuilder.setName("testResolutionErrorPattern");

    for (int32_t i = 0; i < numResolutionErrors - 1; i++) {
        TestUtils::runTestCase(*this, testBuilder.setPattern(jsonTestCasesResolutionError[i].pattern)
                          .setExpected(jsonTestCasesResolutionError[i].output)
                          .setExpectedError(jsonTestCasesResolutionError[i].expected)
                          .build(), errorCode);
    }
}

void TestMessageFormat2::testNoSyntaxErrors(const UnicodeString* patterns, int32_t len, IcuTestErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    TestCase::Builder testBuilder;
    testBuilder.setName("testReservedErrorPattern");

    for (int32_t i = 0; i < len - 1; i++) {
        TestUtils::runTestCase(*this, testBuilder.setPattern(patterns[i])
                          .setNoSyntaxError()
                          .build(), errorCode);
    } 
}

void TestMessageFormat2::testVariousPatterns() {
    IcuTestErrorCode errorCode(*this, "jsonTests");

    jsonTests(errorCode);
    testValidPatterns(validTestCases, numValidTestCases, errorCode);
    testResolutionErrors(errorCode);
    testNoSyntaxErrors(reservedErrors, numReservedErrors, errorCode);
    testNoSyntaxErrors(matches, numMatches, errorCode);
    testNoSyntaxErrors(syntaxTests, numSyntaxTests, errorCode);
}

/*
 Tests a single pattern, which is expected to be invalid.

 `testNum`: Test number (only used for diagnostic output)
 `s`: The pattern string.

 The error is assumed to be on line 0, offset `s.length()`.
*/
void TestMessageFormat2::testInvalidPattern(uint32_t testNum, const UnicodeString& s) {
    testInvalidPattern(testNum, s, s.length(), 0);
}

/*
 Tests a single pattern, which is expected to be invalid.

 `testNum`: Test number (only used for diagnostic output)
 `s`: The pattern string.

 The error is assumed to be on line 0, offset `expectedErrorOffset`.
*/
void TestMessageFormat2::testInvalidPattern(uint32_t testNum, const UnicodeString& s, uint32_t expectedErrorOffset) {
    testInvalidPattern(testNum, s, expectedErrorOffset, 0);
}

/*
 Tests a single pattern, which is expected to be invalid.

 `testNum`: Test number (only used for diagnostic output)
 `s`: The pattern string.
 `expectedErrorOffset`: The expected character offset for the parse error.

 The error is assumed to be on line `expectedErrorLine`, offset `expectedErrorOffset`.
*/
void TestMessageFormat2::testInvalidPattern(uint32_t testNum, const UnicodeString& s, uint32_t expectedErrorOffset, uint32_t expectedErrorLine) {
    IcuTestErrorCode errorCode(*this, "testInvalidPattern");
    char testName[50];
    snprintf(testName, sizeof(testName), "testInvalidPattern: %d", testNum);

    TestCase::Builder testBuilder;
    testBuilder.setName("testName");

    TestUtils::runTestCase(*this, testBuilder.setPattern(s)
                           .setExpectedError(U_SYNTAX_ERROR)
                           .setExpectedLineNumberAndOffset(expectedErrorLine, expectedErrorOffset)
                           .build(), errorCode);
}

/*
 Tests a single pattern, which is expected to cause the parser to
 emit a data model error

 `testNum`: Test number (only used for diagnostic output)
 `s`: The pattern string.
 `expectedErrorCode`: the error code expected to be returned by the formatter

  For now, the line and character numbers are not checked
*/
void TestMessageFormat2::testSemanticallyInvalidPattern(uint32_t testNum, const UnicodeString& s, UErrorCode expectedErrorCode) {
    IcuTestErrorCode errorCode(*this, "testInvalidPattern");

    char testName[50];
    snprintf(testName, sizeof(testName), "testSemanticallyInvalidPattern: %d", testNum);

    TestCase::Builder testBuilder;
    testBuilder.setName("testName").setPattern(s);
    testBuilder.setExpectedError(expectedErrorCode);

    TestUtils::runTestCase(*this, testBuilder.build(), errorCode);
}

/*
 Tests a single pattern, which is expected to cause the formatter
 to emit a resolution error, selection error, or
 formatting error

 `testNum`: Test number (only used for diagnostic output)
 `s`: The pattern string.
 `expectedErrorCode`: the error code expected to be returned by the formatter

 For now, the line and character numbers are not checked
*/
void TestMessageFormat2::testRuntimeErrorPattern(uint32_t testNum, const UnicodeString& s, UErrorCode expectedErrorCode) {
    IcuTestErrorCode errorCode(*this, "testInvalidPattern");
    char testName[50];
    snprintf(testName, sizeof(testName), "testInvalidPattern (errors): %u", testNum);

    TestCase::Builder testBuilder;
    TestUtils::runTestCase(*this, testBuilder.setName(testName)
                           .setPattern(s)
                           .setExpectedError(expectedErrorCode)
                           .build(), errorCode);
}

/*
 Tests a single pattern, which is expected to cause the formatter
 to emit a resolution error, selection error, or
 formatting error

 `testNum`: Test number (only used for diagnostic output)
 `s`: The pattern string.
 `expectedErrorCode`: the error code expected to be returned by the formatter

 For now, the line and character numbers are not checked
*/
void TestMessageFormat2::testRuntimeWarningPattern(uint32_t testNum, const UnicodeString& s, const UnicodeString& expectedResult, UErrorCode expectedErrorCode) {
    IcuTestErrorCode errorCode(*this, "testInvalidPattern");
    char testName[50];
    snprintf(testName, sizeof(testName), "testInvalidPattern (warnings): %u", testNum);

    TestCase::Builder testBuilder;
    TestUtils::runTestCase(*this, testBuilder.setName(testName)
                                .setPattern(s)
                                .setExpected(expectedResult)
                                .setExpectedError(expectedErrorCode)
                                .build(), errorCode);
}

void TestMessageFormat2::testDataModelErrors() {
    uint32_t i = 0;
    IcuTestErrorCode errorCode(*this, "testDataModelErrors");

    // The following tests are syntactically valid but should trigger a data model error

    // Examples taken from https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md

    // Variant key mismatch
    testSemanticallyInvalidPattern(++i, "match {$foo :plural} {$bar :plural} when one{one}", U_VARIANT_KEY_MISMATCH_ERROR);
    testSemanticallyInvalidPattern(++i, "match {$foo :plural} {$bar :plural} when one {one}", U_VARIANT_KEY_MISMATCH_ERROR);
    testSemanticallyInvalidPattern(++i, "match {$foo :plural} {$bar :plural} when one  {one}", U_VARIANT_KEY_MISMATCH_ERROR);

    testSemanticallyInvalidPattern(++i, "match {$foo :plural} when * * {foo}", U_VARIANT_KEY_MISMATCH_ERROR);
    testSemanticallyInvalidPattern(++i, "match {$one :plural}\n\
                             when 1 2 {Too many}\n\
                             when * {Otherwise}", U_VARIANT_KEY_MISMATCH_ERROR);
    testSemanticallyInvalidPattern(++i, "match {$one :plural} {$two :plural}\n\
                             when 1 2 {Two keys}\n\
                             when * {Missing a key}\n\
                             when * * {Otherwise}", U_VARIANT_KEY_MISMATCH_ERROR);

    // Non-exhaustive patterns
    testSemanticallyInvalidPattern(++i, "match {$one :plural}\n\
                                         when 1 {Value is one}\n\
                                         when 2 {Value is two}\n", U_NONEXHAUSTIVE_PATTERN_ERROR);
    testSemanticallyInvalidPattern(++i, "match {$one :plural} {$two :plural}\n\
                                         when 1 * {First is one}\n\
                                         when * 1 {Second is one}\n", U_NONEXHAUSTIVE_PATTERN_ERROR);

    // Duplicate option names
    testSemanticallyInvalidPattern(++i, "{{:foo a=1 b=2 a=1}}", U_DUPLICATE_OPTION_NAME_ERROR);
    testSemanticallyInvalidPattern(++i, "{{:foo a=1 a=1}}", U_DUPLICATE_OPTION_NAME_ERROR);
    testSemanticallyInvalidPattern(++i, "{{:foo a=1 a=2}}", U_DUPLICATE_OPTION_NAME_ERROR);
    testSemanticallyInvalidPattern(++i, "{{|x| :foo a=1 a=2}}", U_DUPLICATE_OPTION_NAME_ERROR);

    // Missing selector annotation
    testSemanticallyInvalidPattern(++i, "match {$one}\n\
                                         when 1 {Value is one}\n\
                                         when * {Value is not one}\n", U_MISSING_SELECTOR_ANNOTATION_ERROR);
    testSemanticallyInvalidPattern(++i, "let $one = {|The one|}\n\
                                         match {$one}\n\
                                         when 1 {Value is one}\n\
                                         when * {Value is not one}\n", U_MISSING_SELECTOR_ANNOTATION_ERROR);
    testSemanticallyInvalidPattern(++i, "match {|horse| ^private}\n\
                                         when 1 {The value is one.}\n          \
                                         when * {The value is not one.}\n", U_MISSING_SELECTOR_ANNOTATION_ERROR);
    testSemanticallyInvalidPattern(++i, "match {$foo !select} when |1| {one} when * {other}",
                                   U_MISSING_SELECTOR_ANNOTATION_ERROR);
    testSemanticallyInvalidPattern(++i, "match {$foo ^select} when |1| {one} when * {other}",
                                   U_MISSING_SELECTOR_ANNOTATION_ERROR);

    TestCase::Builder testBuilder;
    testBuilder.setName("testDataModelErrors");

    // This should *not* trigger a "missing selector annotation" error
    TestCase test = testBuilder.setPattern("let $one = {|The one| :select}\n\
                 match {$one}\n\
                 when 1 {Value is one}\n\
                 when * {Value is not one}")
                          .setExpected("Value is not one")
                          .setExpectSuccess()
                          .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("let $one = {|The one| :select}\n\
                 let $two = {$one}\n\
                 match {$two}\n\
                 when 1 {Value is one}\n\
                 when * {Value is not one}")
                          .setExpected("Value is not one")
                          .setExpectSuccess()
                          .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

void TestMessageFormat2::testResolutionErrors() {
    uint32_t i = 0;

    // The following tests are syntactically valid and free of data model errors,
    // but should trigger a resolution error

    // Unresolved variable
    testRuntimeWarningPattern(++i, "{{$oops}}", "{$oops}", U_UNRESOLVED_VARIABLE_ERROR);
    testRuntimeWarningPattern(++i, "let $x = {$forward} let $forward = {42} {{$x}}", "{$forward}", U_UNRESOLVED_VARIABLE_ERROR);

    // Unknown function
    testRuntimeWarningPattern(++i, "{The value is {horse :func}.}", "The value is {|horse|}.", U_UNKNOWN_FUNCTION_ERROR);
    testRuntimeWarningPattern(++i, "match {|horse| :func}\n\
                                         when 1 {The value is one.}\n\
                                         when * {The value is not one.}\n",
                              "The value is not one.", U_UNKNOWN_FUNCTION_ERROR);
    // Using formatter as selector
    // The fallback string will match the '*' variant
    testRuntimeWarningPattern(++i, "match {|horse| :number}\n\
                                         when 1 {The value is one.}\n\
                                         when * {The value is not one.}\n", "The value is not one.", U_SELECTOR_ERROR);

    // Using selector as formatter
    testRuntimeWarningPattern(++i, "match {|horse| :select}\n\
                                         when 1 {The value is one.}\n   \
                                         when * {{|horse| :select}}\n",
                              "{|horse|}", U_FORMATTING_ERROR);

    // Unsupported expressions
    testRuntimeErrorPattern(++i, "{The value is {@horse}.}", U_UNSUPPORTED_PROPERTY);
    testRuntimeErrorPattern(++i, "{hello {|4.2| @number}}", U_UNSUPPORTED_PROPERTY);
    testRuntimeErrorPattern(++i, "{{<tag}}", U_UNSUPPORTED_PROPERTY);
    testRuntimeErrorPattern(++i, "let $bar = {|42| ~plural} match {|horse| :select} when * {{$bar}}",
                            U_UNSUPPORTED_PROPERTY);

    // Selector error
    // Here, the plural selector returns "no match" so the * variant matches
    testRuntimeWarningPattern(++i, "match {|horse| :plural}\n\
                                  when 1 {The value is one.}\n\
                                  when * {The value is not one.}\n", "The value is not one.", U_SELECTOR_ERROR);
    testRuntimeWarningPattern(++i, "let $sel = {|horse| :plural}\n\
                                  match {$sel}\n\
                                  when 1 {The value is one.}\n\
                                  when * {The value is not one.}\n", "The value is not one.", U_SELECTOR_ERROR);
}

void TestMessageFormat2::testInvalidPatterns() {
/*
  These tests are mostly from the test suite created for the JavaScript implementation of MessageFormat v2:
  <p>Original JSON file
  <a href="https://github.com/messageformat/messageformat/blob/master/packages/mf2-messageformat/src/__fixtures/test-messages.json">here</a>.</p>
  Some have been modified or added to reflect syntax changes that post-date the JSON file.

 */
    uint32_t i = 0;

    // Unexpected end of input
    testInvalidPattern(++i, "let    ");
    testInvalidPattern(++i, "le");
    testInvalidPattern(++i, "let $foo");
    testInvalidPattern(++i, "let $foo =    ");
    testInvalidPattern(++i, "{{:fszzz");
    testInvalidPattern(++i, "{{:fszzz   ");
    testInvalidPattern(++i, "match {$foo} when |xyz");
    testInvalidPattern(++i, "{{:f aaa");
    testInvalidPattern(++i, "{missing end brace");
    testInvalidPattern(++i, "{missing end {$brace");

    // Error should be reported at character 0, not end of input
    testInvalidPattern(++i, "}{|xyz|", 0);
    testInvalidPattern(++i, "}", 0);

    // @xyz is a valid annotation (`reserved`) so the error should be at the end of input
    testInvalidPattern(++i, "{{@xyz");
    // Backslash followed by non-backslash followed by a '{' -- this should be an error
    // immediately after the first backslash
    testInvalidPattern(++i, "{{@\\y{}}", 4);

    // Reserved chars followed by a '|' that doesn't begin a valid literal -- this should be
    // an error at the first invalid char in the literal
    testInvalidPattern(++i, "{{@abc|\\z}}", 8);

    // Same pattern, but with a valid reserved-char following the erroneous reserved-escape
    // -- the offset should be the same as with the previous one
    testInvalidPattern(++i, "{{@\\y{p}}", 4);
    // Erroneous literal inside a reserved string -- the error should be at the first
    // erroneous literal char
    testInvalidPattern(++i, "{{@ab|\\z|cd}}", 7);

    // tests for reserved syntax with bad escaped chars
    // Single backslash - not allowed
    testInvalidPattern(++i, "{hello {|4.2| @num\\ber}}", 19);
    // Unescaped '{' -- not allowed
    testInvalidPattern(++i, "{hello {|4.2| @num{be\\|r}}", 18);
    // Unescaped '}' -- will be interpreted as the end of the reserved
    // string, and the error will be reported at the index of '|', which is
    // when the parser determines that "\|" isn't a valid text-escape
    testInvalidPattern(++i, "{hello {|4.2| @num}be\\|r}}", 22);
    // Unescaped '|' -- will be interpreted as the beginning of a literal
    // Error at end of input
    testInvalidPattern(++i, "{hello {|4.2| @num\\{be|r}}", 26);

    // Invalid escape sequence in a `text` -- the error should be at the character
    // following the backslash
    testInvalidPattern(++i, "{a\\qbc", 3);

    // Missing space after `when` -- the error should be immediately after the
    // `when` (not the error in the pattern)
    testInvalidPattern(++i, "match {|y|} when|y| {|||}", 16);

    // Missing spaces betwen keys in `when`-clause
    testInvalidPattern(++i, "match {|y|} when |foo|bar {a}", 22);
    testInvalidPattern(++i, "match {|y|} when |quux| |foo|bar {a}", 29);
    testInvalidPattern(++i, "match {|y|} when |quux| |foo||bar| {a}", 29);

    // Error parsing the first key -- the error should be there, not in the
    // also-erroneous third key
    testInvalidPattern(++i, "match {|y|} when |\\q| * @{! {z}", 19);

    // Error parsing the second key -- the error should be there, not in the
    // also-erroneous third key
    testInvalidPattern(++i, "match {|y|} when * @{! {z} |\\q|", 19);

    // Error parsing the last key -- the error should be there, not in the erroneous
    // pattern
    testInvalidPattern(++i, "match {|y|} when * |\\q| {\\z}", 21);

    // Selectors not starting with `match` -- error should be on character 1,
    // not the later erroneous key
    testInvalidPattern(++i, "m {|y|} when @{! {z}", 1);

    // Non-expression as scrutinee in pattern -- error should be at the first
    // non-expression, not the later non-expression
    testInvalidPattern(++i, "match {|y|} {\\|} {@} when * * * {a}", 13);

    // Non-key in variant -- error should be there, not in the next erroneous
    // variant
    testInvalidPattern(++i, "match {|y|} when $foo * {a} when * :bar {b}", 17);


    // Error should be within the first erroneous `text` or expression
    testInvalidPattern(++i, "{ foo {|bar|} \\q baz  ", 15);

    // ':' has to be followed by a function name -- the error should be at the first
    // whitespace character
    testInvalidPattern(++i, "{{:    }}", 3);

    // Expression not starting with a '{'
    testInvalidPattern(++i, "let $x = }|foo|}", 9);

    // Error should be at the first declaration not starting with a `let`
    testInvalidPattern(++i, "let $x = {|foo|} l $y = {|bar|} let $z {|quux|}", 18);

    // Missing '=' in `let` declaration
    testInvalidPattern(++i, "let $bar {|foo|} {{$bar}}", 9);

    // LHS of declaration doesn't start with a '$'
    testInvalidPattern(++i, "let bar = {|foo|} {{$bar}}", 4);

    // `let` RHS isn't an expression
    testInvalidPattern(++i, "let $bar = |foo| {{$bar}}", 11);

    // Non-expression
    testInvalidPattern(++i, "no braces", 0);
    testInvalidPattern(++i, "no braces {$foo}", 0);

    // Trailing characters that are not whitespace
    testInvalidPattern(++i, "{extra} content", 8);
    testInvalidPattern(++i, "match {|x|} when * {foo} extra", 25);

    // Empty expression
    testInvalidPattern(++i, "{empty { }}", 9);
    testInvalidPattern(++i, "match {} when * {foo}", 7);

    // ':' not preceding a function name
    testInvalidPattern(++i, "{bad {:}}", 7);

    // Missing '=' after option name
    testInvalidPattern(++i, "{no-equal {|42| :number m }}", 26);
    testInvalidPattern(++i, "{no-equal {|42| :number minimumFractionDigits 2}}", 46);
    testInvalidPattern(++i, "{bad {:placeholder option value}}", 26);

    // Extra '=' after option value
    testInvalidPattern(++i, "{hello {|4.2| :number min=2=3}}", 27),
    testInvalidPattern(++i, "{hello {|4.2| :number min=2max=3}}", 30),
    // Missing whitespace between valid options
    testInvalidPattern(++i, "{hello {|4.2| :number min=|a|max=3}}", 29),
    // Ill-formed RHS of option -- the error should be within the RHS,
    // not after parsing options
    testInvalidPattern(++i, "{hello {|4.2| :number min=|\\a|}}", 28),


    // Junk after annotation
    testInvalidPattern(++i, "{no-equal {|42| :number   {}", 26);

    // Missing RHS of option
    testInvalidPattern(++i, "{bad {:placeholder option=}}", 26);
    testInvalidPattern(++i, "{bad {:placeholder option}}", 25);

    // Annotation is not a function or reserved text
    testInvalidPattern(++i, "{bad {$placeholder option}}", 19);
    testInvalidPattern(++i, "{no {$placeholder end}", 18);

    // Missing whitespace before key in variant
    testInvalidPattern(++i, "match {|foo|} when*{foo}", 18);
    // Missing expression in selectors
    testInvalidPattern(++i, "match when * {foo}", 6);
    // Non-expression in selectors
    testInvalidPattern(++i, "match |x| when * {foo}", 6);

    // Missing RHS in variant
    testInvalidPattern(++i, "match {|x|} when * foo");

    // Text may include newlines; check that the missing closing '}' is
    // reported on the correct line
    testInvalidPattern(++i, "{hello wo\nrld", 3, 1);
    testInvalidPattern(++i, "{hello wo\nr\nl\ndddd", 4, 3);
    // Offset for end-of-input should be 0 here because the line begins
    // after the '\n', but there is no character after the '\n'
    testInvalidPattern(++i, "{hello wo\nr\nl\n", 0, 3);

    // Literals may include newlines; check that the missing closing '|' is
    // reported on the correct line
    testInvalidPattern(++i, "{hello {|wo\nrld}", 4, 1);
    testInvalidPattern(++i, "{hello {|wo\nr\nl\ndddd}", 5, 3);
    // Offset for end-of-input should be 0 here because the line begins
    // after the '\n', but there is no character after the '\n'
    testInvalidPattern(++i, "{hello {|wo\nr\nl\n", 0, 3);

    // Variable names can't start with a : or -
    testInvalidPattern(++i, "{{$:abc}}", 3);
    testInvalidPattern(++i, "{{$-abc}}", 3);

    // Missing space before annotation
    // Note that {{$bar:foo}} and {{$bar-foo}} are valid,
    // because variable names can contain a ':' or a '-'
    testInvalidPattern(++i, "{{$bar+foo}}", 6);
    testInvalidPattern(++i, "{{|3.14|:foo}}", 8);
    testInvalidPattern(++i, "{{|3.14|-foo}}", 8);
    testInvalidPattern(++i, "{{|3.14|+foo}}", 8);

    // Unquoted literals can't begin with a ':'
    testInvalidPattern(++i, "let $foo = {$bar} match {$foo} when :one {one} when * {other}", 36);
    testInvalidPattern(++i, "let $foo = {$bar :fun option=:a} {bar {$foo}}", 29);

}

#endif /* #if !UCONFIG_NO_FORMATTING */

