/********************************************************************
 * COPYRIGHT:
 * Copyright (C) 2001 IBM, Inc.   All Rights Reserved.
 *
 ********************************************************************/
/********************************************************************************
*
* File CALLCOLL.C
*
* Modification History:
*        Name                     Description
*     Andy Heninger             First Version
*
*********************************************************************************
*/

//
//  This program tests string collation and sort key generation performance.
//      Three APIs can be teste: ICU C , Unix strcoll, strxfrm and Windows LCMapString
//      A file of names is required as input, one per line.  It must be in utf-16 format, and
//      include a byte order mark.  Either LE or BE format is OK.
//
//      Usage:
//         collperf options...
//            -file file_name            utf-16 format file of names to sort/search
//            -locale name               ICU locale to use.  Default is en_US
//            -langid 0x1234             Windows Language ID number.  Default 0x409 (en_US)
//                                          see http://msdn.microsoft.com/library/psdk/winbase/nls_8xo3.htm
//            -win                       Run test using Windows native services.  (ICU is default)
//            -unix                      Run test using Unix strxfrm, strcoll services.
//            -uselen                    Use API with string lengths.  Default is null-terminated strings
//            -usekeys                   Run tests using sortkeys rather than strcoll
//            -loop nnnn                 Loopcount for test.  Adjust for reasonable total running time.
//            -terse                     Terse numbers-only output.  Intended for use by scripts.
//            -help                      Display this message.
//            -qsort                     Quicksort timing test
//            -binsearch                 Binary Search timing test
//            -keygen                    Sort Key Generation timing test


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <locale.h>
#include <errno.h>

#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/uloc.h>
#include <unicode/ustring.h>
#include <unicode/ures.h>
#include <unicode/uchar.h>
#include <unicode/ucnv.h>

#ifdef WIN32
#include <windows.h>
#else
//
//  Stubs for Windows API functions when building on UNIXes.
//
typedef int DWORD;
inline int CompareStringW(DWORD, DWORD, UChar *, int, UChar *, int) {return 0;};
#include <sys/time.h>
unsigned long timeGetTime() {
    struct timeval t;
    gettimeofday(&t, 0);
    unsigned long val = t.tv_sec * 1000;  // Let it overflow.  Who cares.
    val += t.tv_usec / 1000;
    return val;
};
inline int LCMapStringW(DWORD, DWORD, UChar *, int, UChar *, int) {return 0;};
const int LCMAP_SORTKEY = 0;
#define MAKELCID(a,b) 0
const int SORT_DEFAULT = 0;
#endif



//
//  Command line option variables
//     These global variables are set according to the options specified
//     on the command line by the user.
char * opt_fName      = "american.txt";
char * opt_locale     = "en_US";
int    opt_langid     = 0x409;      // English, US
UBool  opt_help       = FALSE;
int    opt_loopCount  = 1;
UBool  opt_terse      = FALSE;
UBool  opt_qsort      = FALSE;
UBool  opt_binsearch  = FALSE;
UBool  opt_icu        = TRUE;
UBool  opt_win        = FALSE;      // Run with Windows native functions.
UBool  opt_unix       = FALSE;      // Run with UNIX strcoll, strxfrm functions.
UBool  opt_uselen     = FALSE;
UBool  opt_usekeys    = FALSE;
UBool  opt_norm       = FALSE;
UBool  opt_keygen     = FALSE;



//
//   Definitions for the command line options
//
struct OptSpec {
    const char *name;
    enum {FLAG, NUM, STRING} type;
    void *pVar;
};

OptSpec opts[] = {
    {"-file",   OptSpec::STRING, &opt_fName},
    {"-locale", OptSpec::STRING, &opt_locale},
    {"-langid", OptSpec::NUM,    &opt_langid},
    {"-qsort",  OptSpec::FLAG,   &opt_qsort},
    {"-binsearch",  OptSpec::FLAG,   &opt_binsearch},
    {"-win",    OptSpec::FLAG,   &opt_win},
    {"-unix",   OptSpec::FLAG,   &opt_unix},
    {"-uselen", OptSpec::FLAG,   &opt_uselen},
    {"-usekeys",OptSpec::FLAG,   &opt_usekeys},
    {"-norm",   OptSpec::FLAG,   &opt_norm},
    {"-keygen", OptSpec::FLAG,   &opt_keygen},
    {"-loop",   OptSpec::NUM,    &opt_loopCount},
    {"-terse",  OptSpec::FLAG,   &opt_terse},
    {"-help",   OptSpec::FLAG,   &opt_help},
    {"-?",      OptSpec::FLAG,   &opt_help},
    {0, OptSpec::FLAG, 0}
};


//---------------------------------------------------------------------------
//
//  Global variables pointing to and describing the test file
//
//---------------------------------------------------------------------------

//
//   struct Line
//
//      Each line from the source file (containing a name, presumably) gets
//      one of these structs.
//
struct  Line {
    UChar     *name;
    int        len;
    char      *winSortKey;
    char      *icuSortKey;
    char      *unixSortKey;
    char      *unixName;
};



Line          *gFileLines;           // Ptr to array of Line structs, one per line in the file.
int            gNumFileLines;
UCollator     *gCol;
DWORD          gWinLCID;

Line          **gSortedLines;
Line          **gRandomLines;
int            gCount;



//---------------------------------------------------------------------------
//
//  ProcessOptions()    Function to read the command line options.
//
//---------------------------------------------------------------------------
UBool ProcessOptions(int argc, const char **argv, OptSpec opts[])
{
    int         i;
    int         argNum;
    const char  *pArgName;
    OptSpec    *pOpt;

    for (argNum=1; argNum<argc; argNum++) {
        pArgName = argv[argNum];
        for (pOpt = opts;  pOpt->name != 0; pOpt++) {
            if (strcmp(pOpt->name, pArgName) == 0) {
                switch (pOpt->type) {
                case OptSpec::FLAG:
                    *(UBool *)(pOpt->pVar) = TRUE;
                    break;
                case OptSpec::STRING:
                    argNum ++;
                    if (argNum >= argc) {
                        fprintf(stderr, "value expected for \"%s\" option.\n", pOpt->name);
                        return FALSE;
                    }
                    *(const char **)(pOpt->pVar)  = argv[argNum];
                    break;
                case OptSpec::NUM:
                    argNum ++;
                    if (argNum >= argc) {
                        fprintf(stderr, "value expected for \"%s\" option.\n", pOpt->name);
                        return FALSE;
                    }
                    char *endp;
                    i = strtol(argv[argNum], &endp, 0);
                    if (endp == argv[argNum]) {
                        fprintf(stderr, "integer value expected for \"%s\" option.\n", pOpt->name);
                        return FALSE;
                    }
                    *(int *)(pOpt->pVar) = i;
                }
                break;
            }
        }
        if (pOpt->name == 0)
        {
            fprintf(stderr, "Unrecognized option \"%s\"\n", pArgName);
            return FALSE;
        }
    }
return TRUE;
}

//---------------------------------------------------------------------------------------
//
//   Comparison functions for use by qsort.
//
//       Six flavors, ICU or Windows, SortKey or String Compare, Strings with length
//           or null terminated.
//
//---------------------------------------------------------------------------------------
int ICUstrcmpK(const void *a, const void *b) {
    gCount++;
    int t = strcmp((*(Line **)a)->icuSortKey, (*(Line **)b)->icuSortKey);
    return t;
}


int ICUstrcmpL(const void *a, const void *b) {
    gCount++;
    UCollationResult t;
    t = ucol_strcoll(gCol, (*(Line **)a)->name, (*(Line **)a)->len, (*(Line **)b)->name, (*(Line **)b)->len);
    if (t == UCOL_LESS) return -1;
    if (t == UCOL_GREATER) return +1;
    return 0;
}


int ICUstrcmp(const void *a, const void *b) {
    gCount++;
    UCollationResult t;
    t = ucol_strcoll(gCol, (*(Line **)a)->name, -1, (*(Line **)b)->name, -1);
    if (t == UCOL_LESS) return -1;
    if (t == UCOL_GREATER) return +1;
    return 0;
}


int Winstrcmp(const void *a, const void *b) {
    gCount++;
    int t;
    t = CompareStringW(gWinLCID, 0, (*(Line **)a)->name, -1, (*(Line **)b)->name, -1);
    return t-2;
}


int UNIXstrcmp(const void *a, const void *b) {
    gCount++;
    int t;
    t = strcoll((*(Line **)a)->unixName, (*(Line **)b)->unixName);
    return t;
}


int WinstrcmpL(const void *a, const void *b) {
    gCount++;
    int t;
    t = CompareStringW(gWinLCID, 0, (*(Line **)a)->name, (*(Line **)a)->len, (*(Line **)b)->name, (*(Line **)b)->len);
    return t-2;
}


int WinstrcmpK(const void *a, const void *b) {
    gCount++;
    int t = strcmp((*(Line **)a)->winSortKey, (*(Line **)b)->winSortKey);
    return t;
}


//---------------------------------------------------------------------------------------
//
//   Function for sorting the names (lines) into a random order.
//      Order is based on a hash of the  ICU Sort key for the lines
//      The randomized order is used as input for the sorting timing tests.
//
//---------------------------------------------------------------------------------------
int ICURandomCmp(const void *a, const void *b) {
    char  *ask = (*(Line **)a)->icuSortKey;
    char  *bsk = (*(Line **)b)->icuSortKey;
    int   aVal = 0;
    int   bVal = 0;
    int   retVal;
    while (*ask != 0) {
        aVal += aVal*37 + *ask++;
    }
    while (*bsk != 0) {
        bVal += bVal*37 + *bsk++;
    }
    retVal = -1;
    if (aVal == bVal) {
        retVal = 0;
    }
    else if (aVal > bVal) {
        retVal = 1;
    }
    return retVal;
}

//---------------------------------------------------------------------------------------
//
//   doKeyGen()     Key Generation Timing Test
//
//---------------------------------------------------------------------------------------
void doKeyGen()
{
    int  line;
    int  loops;
    int  t;
    int  len=-1;

    // Adjust loop count to compensate for file size.   Should be order n
    double dLoopCount = double(opt_loopCount) * (1000. /  double(gNumFileLines));
    int adj_loopCount = int(dLoopCount);
    if (adj_loopCount < 1) adj_loopCount = 1;


    unsigned long startTime = timeGetTime();

    if (opt_win) {
        for (loops=0; loops<adj_loopCount; loops++) {
            for (line=0; line < gNumFileLines; line++) {
                if (opt_uselen) {
                    len = gFileLines[line].len;
                }
                t=LCMapStringW(gWinLCID, LCMAP_SORTKEY,
                    gFileLines[line].name, len,
                    (unsigned short *)gFileLines[line].winSortKey, 5000);    // TODO  something with length.
            }
        }
    }
    else if (opt_icu)
    {
        for (loops=0; loops<adj_loopCount; loops++) {
            for (line=0; line < gNumFileLines; line++) {
                if (opt_uselen) {
                    len = gFileLines[line].len;
                }
                t = ucol_getSortKey(gCol, gFileLines[line].name, len, (unsigned char *)gFileLines[line].icuSortKey, 5000);
            }
        }
    }
    else if (opt_unix)
    {
        for (loops=0; loops<adj_loopCount; loops++) {
            for (line=0; line < gNumFileLines; line++) {
                t = strxfrm(gFileLines[line].unixSortKey, gFileLines[line].unixName, 5000);
            }
        }
    }

    unsigned long elapsedTime = timeGetTime() - startTime;
    int ns = (int)(float(1000000) * (float)elapsedTime / (float)(adj_loopCount*gNumFileLines));

    if (opt_terse == FALSE) {
        printf("Sort Key Generation:  total # of keys = %d\n", loops*gNumFileLines);
        printf("Sort Key Generation:  time per key = %d ns\n", ns);
    }
    else {
        printf("%d,  ", ns);
    }

    int   totalKeyLen = 0;
    int   totalChars  = 0;
    for (line=0; line<gNumFileLines; line++) {
        totalChars += u_strlen(gFileLines[line].name);
        if (opt_win) {
            totalKeyLen += strlen(gFileLines[line].winSortKey);
        }
        else if (opt_icu) {
            totalKeyLen += strlen(gFileLines[line].icuSortKey);
        }
        else if (opt_unix) {
            totalKeyLen += strlen(gFileLines[line].unixSortKey);
        }

    }
    if (opt_terse == FALSE) {
        printf("Key Length / character = %f\n", (float)totalKeyLen / (float)totalChars);
    } else {
        printf("%f, ", (float)totalKeyLen / (float)totalChars);
    }
}



//---------------------------------------------------------------------------------------
//
//    doBinarySearch()    Binary Search timing test.  Each name from the list
//                        is looked up in the full sorted list of names.
//
//---------------------------------------------------------------------------------------
void doBinarySearch()
{

    gCount = 0;
    int  line;
    int  loops;

    // Adjust loop count to compensate for file size.   Should be order n (lookups) * log n  (compares/lookup)
    // Accurate timings do not depend on this being perfect.  The correction is just to try to
    //   get total running times of about the right order, so the that user doesn't need to
    //   manually adjust the loop count for every different file size.
    double dLoopCount = double(opt_loopCount) * 3000. / (log10(gNumFileLines) * double(gNumFileLines));
    if (opt_usekeys) dLoopCount *= 5;
    int adj_loopCount = int(dLoopCount);
    if (adj_loopCount < 1) adj_loopCount = 1;

    unsigned long startTime = timeGetTime();

    if (opt_icu )
    {
        UCollationResult  r;
        for (loops=0; loops<adj_loopCount; loops++) {

            for (line=0; line < gNumFileLines; line++) {
                int lineLen  = -1;
                int guessLen = -1;
                if (opt_uselen) {
                    lineLen = (gSortedLines[line])->len;
                }
                int hi      = gNumFileLines-1;
                int lo      = 0;
                int  guess = -1;
                for (;;) {
                    int newGuess = (hi + lo) / 2;
                    if (newGuess == guess)
                        break;
                    guess = newGuess;
                    if (opt_usekeys) {
                        int ri = strcmp((gSortedLines[line])->icuSortKey, (gSortedLines[guess])->icuSortKey);
                        gCount++;
                        r=UCOL_GREATER; if(ri<0) {r=UCOL_LESS;} else if (ri==0) {r=UCOL_EQUAL;}
                    }
                    else
                    {
                        if (opt_uselen) {
                            guessLen = (gSortedLines[guess])->len;
                        }
                        r = ucol_strcoll(gCol, (gSortedLines[line])->name, lineLen, (gSortedLines[guess])->name, guessLen);
                        gCount++;
                    }
                    if (r== UCOL_EQUAL)
                        break;
                    if (r == UCOL_LESS)
                        hi = guess;
                    else
                        lo   = guess;
                }
            }
        }
    }

    if (opt_win)
    {
        int r;
        for (loops=0; loops<adj_loopCount; loops++) {

            for (line=0; line < gNumFileLines; line++) {
                int lineLen  = -1;
                int guessLen = -1;
                if (opt_uselen) {
                    lineLen = (gSortedLines[line])->len;
                }
                int hi   = gNumFileLines-1;
                int lo   = 0;
                int  guess = -1;
                for (;;) {
                    int newGuess = (hi + lo) / 2;
                    if (newGuess == guess)
                        break;
                    guess = newGuess;
                    if (opt_usekeys) {
                        r = strcmp((gSortedLines[line])->winSortKey, (gSortedLines[guess])->winSortKey);
                        gCount++;
                        r+=2;
                    }
                    else
                    {
                        if (opt_uselen) {
                            guessLen = (gSortedLines[guess])->len;
                        }
                        r = CompareStringW(gWinLCID, 0, (gSortedLines[line])->name, lineLen, (gSortedLines[guess])->name, guessLen);
                        if (r == 0) {
                            fprintf(stderr, "Error returned from Windows CompareStringW.\n");
                            exit(-1);
                        }
                        gCount++;
                    }
                    if (r== 2)   //  strings ==
                        break;
                    if (r == 1)  //  line < guess
                        hi = guess;
                    else         //  line > guess
                        lo   = guess;
                }
            }
        }
    }

    if (opt_unix)
    {
        int r;
        for (loops=0; loops<adj_loopCount; loops++) {

            for (line=0; line < gNumFileLines; line++) {
                int hi   = gNumFileLines-1;
                int lo   = 0;
                int  guess = -1;
                for (;;) {
                    int newGuess = (hi + lo) / 2;
                    if (newGuess == guess)
                        break;
                    guess = newGuess;
                    if (opt_usekeys) {
                        r = strcmp((gSortedLines[line])->unixSortKey, (gSortedLines[guess])->unixSortKey);
                        gCount++;
                    }
                    else
                    {
                        r = strcoll((gSortedLines[line])->unixName, (gSortedLines[guess])->unixName);
                        errno = 0;
                        if (errno != 0) {
                            fprintf(stderr, "Error %d returned from strcoll.\n", errno);
                            exit(-1);
                        }
                        gCount++;
                    }
                    if (r == 0)   //  strings ==
                        break;
                    if (r < 0)  //  line < guess
                        hi = guess;
                    else         //  line > guess
                        lo   = guess;
                }
            }
        }
    }

    unsigned long elapsedTime = timeGetTime() - startTime;
    int ns = (int)(float(1000000) * (float)elapsedTime / (float)gCount);
    if (opt_terse == FALSE) {
        printf("binary search:  total # of string compares = %d\n", gCount);
        printf("binary search:  time per compare = %d ns\n", ns);
    } else {
        printf("%d, ", ns);
    }

}




//---------------------------------------------------------------------------------------
//
//   doQSort()    The quick sort timing test.  Uses the C library qsort function.
//
//---------------------------------------------------------------------------------------
void doQSort() {
    int i;
    Line **sortBuf = new Line *[gNumFileLines];

    // Adjust loop count to compensate for file size.   QSort should be n log(n)
    double dLoopCount = double(opt_loopCount) * 3000. / (log10(gNumFileLines) * double(gNumFileLines));
    if (opt_usekeys) dLoopCount *= 5;
    int adj_loopCount = int(dLoopCount);
    if (adj_loopCount < 1) adj_loopCount = 1;


    gCount = 0;
    unsigned long startTime = timeGetTime();
    if (opt_win && opt_usekeys) {
        for (i=0; i<opt_loopCount; i++) {
            memcpy(sortBuf, gRandomLines, gNumFileLines * sizeof(Line *));
            qsort(sortBuf, gNumFileLines, sizeof(Line *), WinstrcmpK);
        }
    }

    else if (opt_win && opt_uselen) {
        for (i=0; i<adj_loopCount; i++) {
            memcpy(sortBuf, gRandomLines, gNumFileLines * sizeof(Line *));
            qsort(sortBuf, gNumFileLines, sizeof(Line *), WinstrcmpL);
        }
    }


    else if (opt_win && !opt_uselen) {
        for (i=0; i<adj_loopCount; i++) {
            memcpy(sortBuf, gRandomLines, gNumFileLines * sizeof(Line *));
            qsort(sortBuf, gNumFileLines, sizeof(Line *), Winstrcmp);
        }
    }

    else if (opt_icu && opt_usekeys) {
        for (i=0; i<adj_loopCount; i++) {
            memcpy(sortBuf, gRandomLines, gNumFileLines * sizeof(Line *));
            qsort(sortBuf, gNumFileLines, sizeof(Line *), ICUstrcmpK);
        }
    }

    else if (opt_icu && opt_uselen) {
        for (i=0; i<adj_loopCount; i++) {
            memcpy(sortBuf, gRandomLines, gNumFileLines * sizeof(Line *));
            qsort(sortBuf, gNumFileLines, sizeof(Line *), ICUstrcmpL);
        }
    }


    else if (opt_icu && !opt_uselen) {
        for (i=0; i<adj_loopCount; i++) {
            memcpy(sortBuf, gRandomLines, gNumFileLines * sizeof(Line *));
            qsort(sortBuf, gNumFileLines, sizeof(Line *), ICUstrcmp);
        }
    }

    else if (opt_unix && !opt_usekeys) {
        for (i=0; i<adj_loopCount; i++) {
            memcpy(sortBuf, gRandomLines, gNumFileLines * sizeof(Line *));
            qsort(sortBuf, gNumFileLines, sizeof(Line *), UNIXstrcmp);
        }
    }

    unsigned long elapsedTime = timeGetTime() - startTime;
    int ns = (int)(float(1000000) * (float)elapsedTime / (float)gCount);
    if (opt_terse == FALSE) {
        printf("qsort:  total # of string compares = %d\n", gCount);
        printf("qsort:  time per compare = %d ns\n", ns);
    } else {
        printf("%d, ", ns);
    }
};


//----------------------------------------------------------------------------------------
//
//   UnixConvert   -- Convert the lines of the file to the encoding for UNIX
//                    Since it appears that Unicode support is going in the general
//                    direction of the use of UTF-8 locales, that is the approach
//                    that is used here.
//
//----------------------------------------------------------------------------------------
void  UnixConvert() {
    int    line;

    UConverter   *cvrtr;    // An ICU code page converter.
    UErrorCode    status = U_ZERO_ERROR;


    cvrtr = ucnv_open("utf-8", &status);    // we are just doing UTF-8 locales for now.
    if (U_FAILURE(status)) {
        fprintf(stderr, "ICU Converter open failed.: %d\n", &status);
        exit(-1);
    }

    for (line=0; line < gNumFileLines; line++) {
        int sizeNeeded = ucnv_fromUChars(cvrtr,
                                         0,            // ptr to target buffer.
                                         0,            // length of target buffer.
                                         gFileLines[line].name,
                                         -1,           //  source is null terminated
                                         &status);
        if (status != U_BUFFER_OVERFLOW_ERROR && status != U_ZERO_ERROR) {
            fprintf(stderr, "Conversion from Unicode, something is wrong.\n");
            exit(-1);
        }
        status = U_ZERO_ERROR;
        gFileLines[line].unixName = new char[sizeNeeded+1];
        sizeNeeded = ucnv_fromUChars(cvrtr,
                                         gFileLines[line].unixName, // ptr to target buffer.
                                         sizeNeeded+1, // length of target buffer.
                                         gFileLines[line].name,
                                         -1,           //  source is null terminated
                                         &status);
        if (U_FAILURE(status)) {
            fprintf(stderr, "ICU Conversion Failed.: %d\n", status);
            exit(-1);
        }
        gFileLines[line].unixName[sizeNeeded] = 0;
    };
    ucnv_close(cvrtr);
}


//----------------------------------------------------------------------------------------
//
//    Main   --  process command line, read in and pre-process the test file,
//                 call other functions to do the actual tests.
//
//----------------------------------------------------------------------------------------
int main(int argc, const char** argv) {
    if (ProcessOptions(argc, argv, opts) != TRUE || opt_help) {
        printf("Usage:  strperf options...\n"
            "-file file_name            utf-16 format file of names\n"
            "-locale name               ICU locale to use.  Default is en_US\n"
            "-langid 0x1234             Windows Language ID number.  Default 0x409 (en_US)\n"
            "                              see http://msdn.microsoft.com/library/psdk/winbase/nls_8xo3.htm\n"
            "-win                       Run test using Windows native services.  (ICU is default)\n"
            "-unix                      Run test using Unix strxfrm, strcoll services.\n"
            "-uselen                    Use API with string lengths.  Default is null-terminated strings\n"
            "-usekeys                   Run tests using sortkeys rather than strcoll\n"
            "-loop nnnn                 Loopcount for test.  Adjust for reasonable total running time.\n"
            "-terse                     Terse numbers-only output.  Intended for use by scripts.\n"
            "-help                      Display this message.\n"
            "-qsort                     Quicksort timing test\n"
            "-binsearch                 Binary Search timing test\n"
            "-keygen                    Sort Key Generation timing test\n"
            );
        exit (1);
    }

    // Make sure that we've only got one API selected.
    if (opt_unix || opt_win) opt_icu = FALSE;
    if (opt_unix) opt_win = FALSE;

    //
    //  Set up an ICU collator
    //
    UErrorCode          status = U_ZERO_ERROR;

    gCol = ucol_open(opt_locale, &status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "Collator creation failed.: %d\n", status);
        return -1;
    }
    if (opt_norm) {
        ucol_setAttribute(gCol, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
    }


    //
    //  Set up a Windows LCID
    //
    gWinLCID = MAKELCID(opt_langid, SORT_DEFAULT);

    //
    //  Set the UNIX locale
    //
    if (opt_unix) {
        if (setlocale(LC_ALL, opt_locale) == 0) {
            fprintf(stderr, "setlocale(LC_ALL, %s) failed.\n", opt_locale);
            exit(-1);
        }
    }

    // Read in  the input file.
    //   File assumed to be utf-16.
    //   Lines go onto heap buffers.  Global index array to line starts is created.
    //   Lines themselves are null terminated.
    //
    FILE *f;
    f = fopen(opt_fName, "r");
    if (f == NULL) {
        fprintf(stderr, "Can not open file \"%s\"\n", opt_fName);
        exit(-1);
    }

    const int MAXLINES = 10000;
    gFileLines = new Line[MAXLINES];
    UChar buf[1024];
    int   column = 0;
    UBool  littleEndian = TRUE;
    UBool  sawBOM       = FALSE;
    for (;;) {
        UChar c;
        int  cL, cH;

        // Get next utf-16 UChar
        //
        if (littleEndian) {
            cL = fgetc(f);
            cH = fgetc(f);
        }
        else
        {
            cH = fgetc(f);
            cL = fgetc(f);
        }
        c  = cL  | (cH << 8);

        //
        //  Look for the byte order mark at the start of the file.
        //
        if (sawBOM == FALSE) {

            if (c == 0xfeff) {   // Little Endian BOM
                sawBOM = TRUE;
                continue;
            }
            if (c == 0xfffe) {  // Big endian BOM
                sawBOM = TRUE;
                littleEndian = FALSE;
                continue;
            }
            fprintf(stderr, "Error - no BOM in file.  File format must be UTF-16.\n");
            exit(-1);
        }

        // Watch for CR, LF, EOF; these finish off a line.
        if (c == 0xd) {
            continue;
        }

        if (cL == EOF || cH == EOF || c == 0x0a || c==0x2028) {  // Unipad inserts 2028 line separators!
            buf[column++] = 0;
            if (column > 1) {
                gFileLines[gNumFileLines].name  = new UChar[column];
                gFileLines[gNumFileLines].len   = column-1;
                memcpy(gFileLines[gNumFileLines].name, buf, column * sizeof(UChar));
                gNumFileLines++;
                column = 0;
                if (gNumFileLines >= MAXLINES) {
                    fprintf(stderr, "File too big.  Max number of lines is %d\n", MAXLINES);
                    exit(-1);
                }

            }
            if (c == 0xa || c == 0x2028)
                continue;
            else
                break;  // EOF
        }
        buf[column++] = c;
        if (column >= 1023)
        {
            static UBool warnFlag = TRUE;
            if (warnFlag) {
                fprintf(stderr, "Warning - file line longer than 1023 chars truncated.\n");
                warnFlag = FALSE;
            }
            column--;
        }
    }

    fclose(f);
    if (opt_terse == FALSE) {
        printf("file \"%s\", %d lines.\n", opt_fName, gNumFileLines);
    }


    // Convert the lines to the UNIX encoding.
    if (opt_unix) {
        UnixConvert();
    }

    //
    //  Pre-compute ICU sort keys for the lines of the file.
    //
    int line;
    int t;

    for (line=0; line<gNumFileLines; line++) {
         t = ucol_getSortKey(gCol, gFileLines[line].name, -1, (unsigned char *)buf, sizeof(buf));
         gFileLines[line].icuSortKey  = new char[t];

         if (t > sizeof(buf)) {
             t = ucol_getSortKey(gCol, gFileLines[line].name, -1, (unsigned char *)gFileLines[line].icuSortKey , t);
         }
         else
         {
             memcpy(gFileLines[line].icuSortKey, buf, t);
         }
    }



    //
    //  Pre-compute Windows sort keys for the lines of the file.
    //
    for (line=0; line<gNumFileLines; line++) {
         t=LCMapStringW(gWinLCID, LCMAP_SORTKEY, gFileLines[line].name, -1, buf, sizeof(buf));
         gFileLines[line].winSortKey  = new char[t];
         if (t > sizeof(buf)) {
             t = LCMapStringW(gWinLCID, LCMAP_SORTKEY, gFileLines[line].name, -1, (unsigned short *)(gFileLines[line].winSortKey), t);
         }
         else
         {
             memcpy(gFileLines[line].winSortKey, buf, t);
         }
    }

    //
    //  Pre-compute UNIX sort keys for the lines of the file.
    //
    if (opt_unix) {
        for (line=0; line<gNumFileLines; line++) {
            t=strxfrm((char *)buf,  gFileLines[line].unixName,  sizeof(buf));
            gFileLines[line].unixSortKey  = new char[t];
            if (t > sizeof(buf)) {
                t = strxfrm(gFileLines[line].unixSortKey,  gFileLines[line].unixName,  sizeof(buf));
            }
            else
            {
                memcpy(gFileLines[line].unixSortKey, buf, t);
            }
        }
    }



    //
    //  Pre-sort the lines.
    //
    int i;
    gSortedLines = new Line *[gNumFileLines];
    for (i=0; i<gNumFileLines; i++) {
        gSortedLines[i] = &gFileLines[i];
    }

    if (opt_win) {
        qsort(gSortedLines, gNumFileLines, sizeof(Line *), Winstrcmp);
    }
    else if (opt_unix) {
        qsort(gSortedLines, gNumFileLines, sizeof(Line *), UNIXstrcmp);
    }
    else   /* ICU */
    {
        qsort(gSortedLines, gNumFileLines, sizeof(Line *), ICUstrcmp);
    }


    //
    //  Make up a randomized order, will be used for sorting tests.
    //
    gRandomLines = new Line *[gNumFileLines];
    for (i=0; i<gNumFileLines; i++) {
        gRandomLines[i] = &gFileLines[i];
    }
    qsort(gRandomLines, gNumFileLines, sizeof(Line *), ICURandomCmp);




    //
    //  We've got the file read into memory.  Go do something with it.
    //

    if (opt_qsort)     doQSort();
    if (opt_binsearch) doBinarySearch();
    if (opt_keygen)    doKeyGen();

    return 0;

}
