// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
 *******************************************************************************
 *
 *   Copyright (C) 1999-2013, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  gendata.cpp
 *
 *   created on: 11/03/2000
 *   created by: Eric R. Mader
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "unicode/utypes.h"
#include "unicode/unistr.h"
#include "unicode/uscript.h"
#include "unicode/ubidi.h"
#include "unicode/ustring.h"

#include "layout/LETypes.h"
#include "layout/LEScripts.h"
#include "layout/LayoutEngine.h"

#include "PortableFontInstance.h"
#include "SimpleFontInstance.h"

#include "xmlparser.h"

#include "letsutil.h"
#include "letest.h"

U_NAMESPACE_USE

static LEErrorCode overallStatus = LE_NO_ERROR;
struct TestInput
{
    const char *fontName;
    LEUnicode  *text;
    le_int32    textLength;
    le_int32    scriptCode;
    le_bool     rightToLeft;
};

/* Returns the path to icu/source/test/testdata/ */
const char *getSourceTestData() {
    const char *srcDataDir = NULL;
#ifdef U_TOPSRCDIR
    srcDataDir =  U_TOPSRCDIR  U_FILE_SEP_STRING "test" U_FILE_SEP_STRING "testdata" U_FILE_SEP_STRING;
#else
    srcDataDir = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING"test"U_FILE_SEP_STRING"testdata"U_FILE_SEP_STRING;
    FILE *f = fopen(".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING"test"U_FILE_SEP_STRING"testdata"U_FILE_SEP_STRING"rbbitst.txt", "r");

    if (f != NULL) {
        /* We're in icu/source/test/letest/ */
        fclose(f);
    } else {
        /* We're in icu/source/test/letest/(Debug|Release) */
        srcDataDir = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING"test"U_FILE_SEP_STRING"testdata"U_FILE_SEP_STRING;
    }
#endif

    return srcDataDir;
}

const char *getPath(char buffer[2048], const char *filename) {
    const char *testDataDirectory = getSourceTestData();

    strcpy(buffer, testDataDirectory);
    strcat(buffer, filename);

    return buffer;
}

/* 
 * FIXME: should use the output file name and the current date.
 */
const char *header =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "\n"
    "<!--\n"
    "  Copyright (c) 1999-%4.4d International Business Machines\n"
    "  Corporation and others. All rights reserved.\n"
    "\n"
    "  WARNING: THIS FILE IS MACHINE GENERATED. DO NOT HAND EDIT IT\n"
    "  UNLESS YOU REALLY KNOW WHAT YOU'RE DOING.\n"
    "\n"
    "  file name:    letest.xml\n"
    "  generated on: %s\n"
    "  generated by: gendata.cpp\n"
    "-->\n"
    "\n"
    "<layout-tests>\n";

void dumpLongs(FILE *file, const char *tag, le_int32 *longs, le_int32 count) {
    char lineBuffer[8 * 12 + 2];
    le_int32 bufp = 0;

    fprintf(file, "        <%s>\n", tag);

    for (int i = 0; i < count; i += 1) {
        if (i % 8 == 0 && bufp != 0) {
            fprintf(file, "            %s\n", lineBuffer);
            bufp = 0;
        }

        bufp += sprintf(&lineBuffer[bufp], "0x%8.8X, ", longs[i]);
    }

    if (bufp != 0) {
        lineBuffer[bufp - 2] = '\0';
        fprintf(file, "            %s\n", lineBuffer);
    }

    fprintf(file, "        </%s>\n\n", tag);
}

void dumpFloats(FILE *file, const char *tag, float *floats, le_int32 count) {
    char lineBuffer[8 * 16 + 2];
    le_int32 bufp = 0;

    fprintf(file, "        <%s>\n", tag);

    for (int i = 0; i < count; i += 1) {
        if (i % 8 == 0 && bufp != 0) {
            fprintf(file, "            %s\n", lineBuffer);
            bufp = 0;
        }

        bufp += sprintf(&lineBuffer[bufp], "%f, ", floats[i]);
    }

    if (bufp != 0) {
        lineBuffer[bufp - 2] = '\0';
        fprintf(file, "            %s\n", lineBuffer);
    }

    fprintf(file, "        </%s>\n", tag);
}

int main(int argc, char *argv[])
{
    UErrorCode status = U_ZERO_ERROR;
    const char *gendataFile = "gendata.xml";
    FILE *outputFile = fopen(argv[1], "w");
    if(argc>2) {
      gendataFile = argv[2];
    }
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    const char *tmFormat = "%m/%d/%Y %I:%M:%S %p %Z";
    char  tmString[64];
    le_uint32 count = 0;
    strftime(tmString, 64, tmFormat, local);
    fprintf(outputFile, header, local->tm_year + 1900, tmString);

    UXMLParser  *parser = UXMLParser::createParser(status);
    UXMLElement *root   = parser->parseFile(gendataFile, status);

    if (root == NULL) {
      printf("Error: Could not open %s\n", gendataFile);
        delete parser;
        return -1;
    } else if(U_FAILURE(status)) {
      printf("Error reading %s: %s\n", gendataFile, u_errorName(status));
      return -2;
    } else {
      printf("Reading %s\n", gendataFile);
    }

    UnicodeString test_case        = UNICODE_STRING_SIMPLE("test-case");
    UnicodeString test_text        = UNICODE_STRING_SIMPLE("test-text");
    UnicodeString test_font        = UNICODE_STRING_SIMPLE("test-font");

    // test-case attributes
    UnicodeString id_attr     = UNICODE_STRING_SIMPLE("id");
    UnicodeString script_attr = UNICODE_STRING_SIMPLE("script");
    UnicodeString lang_attr   = UNICODE_STRING_SIMPLE("lang");

    // test-font attributes
    UnicodeString name_attr   = UNICODE_STRING_SIMPLE("name");

    const UXMLElement *testCase;
    int32_t tc = 0;

    while((testCase = root->nextChildElement(tc)) != NULL) {
        if (testCase->getTagName().compare(test_case) == 0) {
            char *id = getCString(testCase->getAttribute(id_attr));
            char *script = getCString(testCase->getAttribute(script_attr));
            char *lang   = getCString(testCase->getAttribute(lang_attr));
            ++count;
            printf("\n ID %s\n", id);
            LEFontInstance *font = NULL;
            const UXMLElement *element;
            int32_t ec = 0;
            int32_t charCount = 0;
            int32_t typoFlags = LayoutEngine::kTypoFlagKern | LayoutEngine::kTypoFlagLiga; // kerning + ligatures...
            UScriptCode scriptCode;
            le_int32 languageCode = -1;
            UnicodeString text;
            int32_t glyphCount = 0;
            LEErrorCode leStatus = LE_NO_ERROR;
            LayoutEngine *engine = NULL;
            LEGlyphID *glyphs    = NULL;
            le_int32  *indices   = NULL;
            float     *positions = NULL;

            uscript_getCode(script, &scriptCode, 1, &status);
            if (LE_FAILURE(status)) {
                printf("Error: invalid script name: %s.\n", script);
                goto free_c_strings;
            }

            if (lang != NULL) {
                languageCode = getLanguageCode(lang);

                if (languageCode < 0) {
                    printf("Error: invalid language name: %s.\n", lang);
                    goto free_c_strings;
                }

                fprintf(outputFile, "    <test-case id=\"%s\" script=\"%s\" lang=\"%s\">\n", id, script, lang);
            } else {
                fprintf(outputFile, "    <test-case id=\"%s\" script=\"%s\">\n", id, script);
            }

            while((element = testCase->nextChildElement(ec)) != NULL) {
                UnicodeString tag = element->getTagName();

                // TODO: make sure that each element is only used once.
                if (tag.compare(test_font) == 0) {
                    char *fontName  = getCString(element->getAttribute(name_attr));
                    const char *version = NULL;
                    char buf[2048];
                    PortableFontInstance *pfi = new PortableFontInstance(getPath(buf,fontName), 12, leStatus);
                    
                    if (LE_FAILURE(leStatus)) {
                      printf("Error: could not open font: %s (path: %s)\n", fontName, buf);
                        freeCString(fontName);
                        goto free_c_strings;
                    }

                    printf(" Generating: %s, %s, %s, %s\n", id, script, lang, fontName);

                    version = pfi->getNameString(NAME_VERSION_STRING, PLATFORM_MACINTOSH, MACINTOSH_ROMAN, MACINTOSH_ENGLISH);

                    // The standard recommends that the Macintosh Roman/English name string be present, but
                    // if it's not, try the Microsoft Unicode/English string.
                    if (version == NULL) {
                        const LEUnicode16 *uversion = pfi->getUnicodeNameString(NAME_VERSION_STRING, PLATFORM_MICROSOFT, MICROSOFT_UNICODE_BMP, MICROSOFT_ENGLISH);

                        if (uversion != NULL) {
                          char uversion_utf8[300];
                          UErrorCode status2 = U_ZERO_ERROR;
                          u_strToUTF8(uversion_utf8, 300, NULL, uversion, -1, &status2);
                          if(U_FAILURE(status2)) {
                            uversion_utf8[0]=0;
                          }
                            fprintf(outputFile, "        <test-font name=\"%s\" version=\"%s\" checksum=\"0x%8.8X\" rchecksum=\"0x%8.8X\"/>\n\n",
                                    fontName, uversion_utf8, pfi->getFontChecksum(), pfi->getRawChecksum());

                            pfi->deleteNameString(uversion);
                        } else {
                          fprintf(outputFile, "        <test-font name=\"%s\" version=\"unknown-0x%8.8X\" checksum=\"0x%8.8X\" rchecksum=\"0x%8.8X\"/>\n\n",
                                  fontName, pfi->getFontChecksum(), pfi->getFontChecksum(), pfi->getRawChecksum());
                        }
                    } else {
                        fprintf(outputFile, "        <test-font name=\"%s\" version=\"%s\" checksum=\"0x%8.8X\" rchecksum=\"0x%8.8X\"/>\n\n",
                                fontName, version, pfi->getFontChecksum(), pfi->getRawChecksum());

                        pfi->deleteNameString(version);
                    }
                    fflush(outputFile);

                    freeCString(fontName);

                    font = pfi;
                } else if (tag.compare(test_text) == 0) {
                    char *utf8 = NULL;

                    text = element->getText(true);
                    charCount = text.length();

                    utf8 = getUTF8String(&text);
                    fprintf(outputFile, "        <test-text>%s</test-text>\n\n", utf8);
                    fflush(outputFile);
                    freeCString(utf8);
                } else {
                    // an unknown tag...
                    char *cTag = getCString(&tag);

                    printf("Test %s: unknown element with tag \"%s\"\n", id, cTag);
                    freeCString(cTag);
                }
            }

            if (font == NULL) {
                LEErrorCode fontStatus = LE_NO_ERROR;

                font = new SimpleFontInstance(12, fontStatus);
                typoFlags |= 0x80000000L;  // use CharSubstitutionFilter...
            }

            engine = LayoutEngine::layoutEngineFactory(font, scriptCode, languageCode, typoFlags, leStatus);

            if (LE_FAILURE(leStatus)) {
                printf("Error for test %s: could not create a LayoutEngine.\n", id);
                goto delete_font;
            }

            glyphCount = engine->layoutChars(text.getBuffer(), 0, charCount, charCount, getRTL(text), 0, 0, leStatus);

            glyphs    = NEW_ARRAY(LEGlyphID, glyphCount);
            indices   = NEW_ARRAY(le_int32, glyphCount);
            positions = NEW_ARRAY(float, glyphCount * 2 + 2);

            engine->getGlyphs(glyphs, leStatus);
            engine->getCharIndices(indices, leStatus);
            engine->getGlyphPositions(positions, leStatus);

            if(LE_FAILURE(leStatus)) {
              fprintf(stderr,"ERROR: LO returned error: %s\n", u_errorName((UErrorCode)leStatus));
              overallStatus = leStatus;
              fprintf(outputFile, "<!-- ERROR: %d -->\n", leStatus);
              fflush(outputFile);
              leStatus = LE_NO_ERROR;
            } else {
              dumpLongs(outputFile, "result-glyphs", (le_int32 *) glyphs, glyphCount);
              
              dumpLongs(outputFile, "result-indices", indices, glyphCount);
              
              dumpFloats(outputFile, "result-positions", positions, glyphCount * 2 + 2);
              fflush(outputFile);

            }

            DELETE_ARRAY(positions);
            DELETE_ARRAY(indices);
            DELETE_ARRAY(glyphs);

            delete engine;

delete_font:
            fprintf(outputFile, "    </test-case>\n\n");
            fflush(outputFile);

            delete font;

free_c_strings:
            freeCString(lang);
            freeCString(script);
            freeCString(id);
        }
    }

    delete root;
    delete parser;

    fprintf(outputFile, "</layout-tests>\n");

    if(count==0) {
      fprintf(stderr, "No cases processed!\n");
      return 1;
    }


    if(LE_FAILURE(overallStatus)) {
      fprintf(outputFile, "<!-- !!! FAILED. %d -->\n", overallStatus);
      fprintf(stderr, "!!! FAILED. %d\n", overallStatus);
      fclose(outputFile);
      return 0;
      //      return 1;
    } else {
      printf("Generated.\n");
      fclose(outputFile);
      return 0;
    }
}
