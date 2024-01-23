// © 2023 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
package com.ibm.icu.dev.test.format;

import java.io.*;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.Locale;

import com.ibm.icu.dev.test.TestFmwk;
import com.ibm.icu.dev.test.TestUtil;
import com.ibm.icu.text.PersonName;
import com.ibm.icu.text.PersonNameFormatter;
import com.ibm.icu.text.SimplePersonName;
import com.ibm.icu.dev.test.rbbi.RBBITstUtils;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import junitparams.JUnitParamsRunner;
import junitparams.Parameters;

/**
 * This is a test designed to parse the files generated by GeneratePersonNameTestData.java in
 * the CLDR project.
 */
@RunWith(JUnitParamsRunner.class)
public class PersonNameConsistencyTest extends TestFmwk {
    private static final String DATA_PATH = TestUtil.DATA_PATH + "cldr/personNameTest/";

    static private Collection<String> FILENAMES_TO_SKIP =
         Arrays.asList("gaa.txt", "syr.txt", "lij.txt");

    static private Collection<String> FILENAMES_TO_SKIP_FOR_17028 =
        Arrays.asList("yue_Hans.txt", "to.txt", "gl.txt", "ie.txt" );

    static List<String> readTestCases() throws Exception {
        List<String> tests = new ArrayList<>();
        InputStream catalogFileStream = TestUtil.class.getResourceAsStream(DATA_PATH + "catalog.txt");
        LineNumberReader catalogFile = new LineNumberReader(new InputStreamReader(catalogFileStream));
        String filename = null;
        while ((filename = catalogFile.readLine()) != null) {
            if (filename.startsWith("#")) { // comment line, skip without logging
                continue;
            }
            if (!filename.endsWith(".txt")) {
                logln("Skipping " + filename + "...");
                continue;
            }
            tests.add(filename);
        }
        return tests;
    }

    private boolean shouldSkipTest(String filename, String errorMsg) {
        if (FILENAMES_TO_SKIP.contains(filename)) {
            return true;
        }
        if (FILENAMES_TO_SKIP_FOR_17028.contains(filename) &&
                logKnownIssue("ICU-17028", errorMsg)) {
            return true;
        }
        if (filename.equals("my.txt") && RBBITstUtils.skipDictionaryTest()) {
            return true;
        }
        return false;
    }

    @Test
    @Parameters(method = "readTestCases")
    public void TestPersonNames(String filename) throws IOException {
        LineNumberReader in = new LineNumberReader(new InputStreamReader(TestUtil.class.getResourceAsStream(DATA_PATH + filename)));
        String line = null;
        PersonNameTester tester = new PersonNameTester(filename);

        int errors = 0;
        try {
            while ((line = in.readLine()) != null) {
                tester.processLine(line, in.getLineNumber());
            }
            errors = tester.getErrorCount();
            System.out.println(filename + " had " + errors + " errors");
        } catch (Exception e) {
            String errorMsg = e.toString() + " " + e.getMessage();
            if (shouldSkipTest(filename, errorMsg)) {
                System.out.println("Test throw exception on " + filename + ": " + errorMsg);
                return;
            }
        }
        if (errors != 0) {
            String errorMsg = "ERROR: Testing against '" + filename + "' contains " + errors + " errors.";
            if (shouldSkipTest(filename, errorMsg)) {
                  System.out.println("Test failure on " + filename + ": " + errorMsg);
                  return;
            }
            errln(errorMsg);
        }
    }

    private static class PersonNameTester {
        SimplePersonName name = null;
        SimplePersonName.Builder nameBuilder = null;
        String expectedResult = null;
        Locale formatterLocale = null;
        int errorCount = 0;

        public PersonNameTester(String testFileName) {
            formatterLocale = Locale.forLanguageTag(testFileName.substring(0, testFileName.length() - ".txt".length()).replace('_', '-'));
        }

        public void processLine(String line, int lineNumber) {
            if (line == null || line.isEmpty() || line.startsWith("#")) {
                return;
            }

            String[] lineFields = line.split(";");
            String opcode = lineFields[0].trim();
            String[] parameters = Arrays.copyOfRange(lineFields,1, lineFields.length);

            processCommand(opcode, parameters, lineNumber);
        }

        public int getErrorCount() {
            return errorCount;
        }

        private void processCommand(String opcode, String[] parameters, int lineNumber) {
            if (opcode.equals("enum")) {
                processEnumLine();
            } else if (opcode.equals("name")) {
                processNameLine(parameters, lineNumber);
            } else if (opcode.equals("expectedResult")) {
                processExpectedResultLine(parameters, lineNumber);
            } else if (opcode.equals("parameters")) {
                processParametersLine(parameters, lineNumber);
            } else if (opcode.equals("endName")) {
                processEndNameLine();
            } else {
                System.err.println("Unknown command '" + opcode + "' at line " + lineNumber);
            }
        }

        private void processEnumLine() {
            // this test isn't actually going to do anything with "enum" lines
        }

        private void processNameLine(String[] parameters, int lineNumber) {
            if (checkState(name == null, "name", lineNumber)
                    && checkNumParams(parameters, 2, "name", lineNumber)) {
                if (nameBuilder == null) {
                    nameBuilder = SimplePersonName.builder();
                }

                String fieldName = parameters[0].trim();
                String fieldValue = parameters[1].trim();

                if (fieldName.equals("locale")) {
                    nameBuilder.setLocale(Locale.forLanguageTag(fieldValue.replace("_", "-")));
                } else {
                    String[] fieldNamePieces = fieldName.split("-");
                    PersonName.NameField nameField = PersonName.NameField.forString(fieldNamePieces[0]);
                    List<PersonName.FieldModifier> fieldModifiers = new ArrayList<>();
                    for (int i = 1; i < fieldNamePieces.length; i++) {
                        fieldModifiers.add(PersonName.FieldModifier.forString(fieldNamePieces[i]));
                    }
                    nameBuilder.addField(nameField, fieldModifiers, fieldValue);
                }
            }
        }

        private void processExpectedResultLine(String[] parameters, int lineNumber) {
            if (checkState(name != null || nameBuilder != null, "expectedResult", lineNumber)
                    && checkNumParams(parameters, 1, "expectedResult", lineNumber)) {
                if (name == null) {
                    name = nameBuilder.build();
                    nameBuilder = null;
                }
                expectedResult = parameters[0].trim();
            }
        }

        private void processParametersLine(String[] parameters, int lineNumber) {
            if (checkState(name != null && expectedResult != null, "parameters", lineNumber)
                    && checkNumParams(parameters, 4, "parameters", lineNumber)) {
                String orderStr = parameters[0].trim();
                String lengthStr = parameters[1].trim();
                String usageStr = parameters[2].trim();
                String formalityStr = parameters[3].trim();

                PersonNameFormatter.Builder builder = PersonNameFormatter.builder();
                builder.setLocale(formatterLocale);
                if (orderStr.equals("sorting")) {
                    builder.setDisplayOrder(PersonNameFormatter.DisplayOrder.SORTING);
                } else if (orderStr.equals("givenFirst")) {
                    builder.setDisplayOrder(PersonNameFormatter.DisplayOrder.FORCE_GIVEN_FIRST);
                } else if (orderStr.equals("surnameFirst")) {
                    builder.setDisplayOrder(PersonNameFormatter.DisplayOrder.FORCE_SURNAME_FIRST);
                }
                builder.setLength(PersonNameFormatter.Length.valueOf(lengthStr.toUpperCase()));
                builder.setUsage(PersonNameFormatter.Usage.valueOf(usageStr.toUpperCase()));
                builder.setFormality(PersonNameFormatter.Formality.valueOf(formalityStr.toUpperCase()));

                PersonNameFormatter formatter = builder.build();
                String actualResult = formatter.formatToString(name);

                checkResult(actualResult, lineNumber);
            }
        }

        private void processEndNameLine() {
            name = null;
            expectedResult = null;
            nameBuilder = null;
        }

        private boolean checkNumParams(String[] parameters, int expectedLength, String opcode, int lineNumber) {
            boolean result = parameters.length == expectedLength;
            if (!result) {
                reportError("'" + opcode + "' line doesn't have " + expectedLength + " parameters", lineNumber);
            }
            return result;
        }

        private boolean checkState(boolean state, String opcode, int lineNumber) {
            if (!state) {
                reportError("Misplaced '" + opcode + "' line", lineNumber);
            }
            return state;
        }

        private boolean checkResult(String actualResult, int lineNumber) {
            boolean result = expectedResult.equals(actualResult);
            if (!result) {
                reportError("Expected '" + expectedResult + "', got '" + actualResult + "'", lineNumber);
            }
            return result;
        }

        private void reportError(String error, int lineNumber) {
            System.out.println("    " + error + " at line " + lineNumber);
            ++errorCount;
        }
    }
}
