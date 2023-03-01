// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/**
 * Copyright (c) 1999-2016, International Business Machines Corporation and
 * others. All Rights Reserved.
 *
 * Generator for source/i18n/collunsafe.h
 * see Makefile
 */

#include <stdio.h>
#include "unicode/uversion.h"
#include "unicode/uniset.h"
#include "collationroot.h"
#include "collationtailoring.h"

/**
 * Define the type of generator to use. Choose one.
 */
#define SERIALIZE 1   //< Default: use UnicodeSet.serialize() and a new internal c'tor
#define RANGES 0      //< Enumerate ranges (works, not as fast. No support in collationdatareader.cpp)
#define PATTERN 0     //< Generate a UnicodeSet pattern (depends on #11891 AND probably slower. No support in collationdatareader.cpp)

int main(int argc, const char *argv[]) {
    UErrorCode errorCode = U_ZERO_ERROR;

    // Get the unsafeBackwardsSet
    const CollationCacheEntry *rootEntry = CollationRoot::getRootCacheEntry(errorCode);
    if(U_FAILURE(errorCode)) {
      fprintf(stderr, "Err: %s getting root cache entry\n", u_errorName(errorCode));
      return 1;
    }
    const UVersionInfo &version = rootEntry->tailoring->version;
    const UnicodeSet *unsafeBackwardSet = rootEntry->tailoring->unsafeBackwardSet;
    char verString[20];
    u_versionToString(version, verString);
    fprintf(stderr, "Generating data for ICU %s, Collation %s\n", U_ICU_VERSION, verString);
    int32_t rangeCount = unsafeBackwardSet->getRangeCount();
    
#if SERIALIZE
    fprintf(stderr, ".. serializing\n");
    // UnicodeSet serialization
    
    UErrorCode preflightCode = U_ZERO_ERROR;
    // preflight
    int32_t serializedCount = unsafeBackwardSet->serialize(nullptr,0,preflightCode);
    if(U_FAILURE(preflightCode) && preflightCode != U_BUFFER_OVERFLOW_ERROR) {
      fprintf(stderr, "Err: %s preflighting unicode set\n", u_errorName(preflightCode));
      return 1;
    }
    uint16_t *serializedData = new uint16_t[serializedCount];
    // serialize
    unsafeBackwardSet->serialize(serializedData, serializedCount, errorCode);
    if(U_FAILURE(errorCode)) {
      delete [] serializedData;
      fprintf(stderr, "Err: %s serializing unicodeset\n", u_errorName(errorCode));
      return 1;
    }
#endif
    
#if PATTERN
    fprintf(stderr,".. pattern. (Note: collationdatareader.cpp does not support this form also see #11891)\n");
    // attempt to use pattern
    
    UnicodeString pattern;
    UnicodeSet set(*unsafeBackwardSet);
    set.compact();
    set.toPattern(pattern, false);

    if(U_SUCCESS(errorCode)) {
      // This fails (bug# ?) - which is why this method was abandoned.
      
      // UnicodeSet usA(pattern, errorCode);
      // fprintf(stderr, "\n%s:%d: err creating set A %s\n", __FILE__, __LINE__, u_errorName(errorCode));
      // return 1;
    }


    const UChar *buf = pattern.getBuffer();
    int32_t needed = pattern.length();

    // print
    {
      char buf2[2048];
      int32_t len2 = pattern.extract(0, pattern.length(), buf2, "utf-8");
      buf2[len2]=0;
      fprintf(stderr,"===\n%s\n===\n", buf2);
    }

    const UnicodeString unsafeBackwardPattern(false, buf, needed);
  if(U_SUCCESS(errorCode)) {
    //UnicodeSet us(unsafeBackwardPattern, errorCode);
    //    fprintf(stderr, "\n%s:%d: err creating set %s\n", __FILE__, __LINE__, u_errorName(errorCode));
  } else {
    fprintf(stderr, "Uset OK - \n");
  }
#endif


  // Generate the output file.

  printf("// collunsafe.h\n");
  printf("// %s\n", U_COPYRIGHT_STRING);
  printf("\n");
  printf("// To be included by collationdatareader.cpp, and generated by gencolusb.\n");
  printf("// Machine generated, do not edit.\n");
  printf("\n");
  printf("#ifndef COLLUNSAFE_H\n"
         "#define COLLUNSAFE_H\n"
         "\n"
         "#include \"unicode/utypes.h\"\n"
         "\n"
         "#define COLLUNSAFE_ICU_VERSION \"" U_ICU_VERSION "\"\n");
  printf("#define COLLUNSAFE_COLL_VERSION \"%s\"\n", verString);

  
  
#if PATTERN
  printf("#define COLLUNSAFE_PATTERN 1\n");
  printf("static const int32_t collunsafe_len = %d;\n", needed);
  printf("static const UChar collunsafe_pattern[collunsafe_len] = {\n");
  for(int i=0;i<needed;i++) {
    if( (i>0) && (i%8 == 0) ) {
      printf(" // %d\n", i);
    }
    printf("0x%04X", buf[i]); // TODO check
    if(i != (needed-1)) {
      printf(", ");
    }
    }
  printf(" //%d\n};\n", (needed-1));
#endif

#if RANGE
    fprintf(stderr, "COLLUNSAFE_RANGE - no code support in collationdatareader.cpp for this\n");
    printf("#define COLLUNSAFE_RANGE 1\n");
    printf("static const int32_t unsafe_rangeCount = %d;\n", rangeCount);
    printf("static const UChar32 unsafe_ranges[%d] = { \n", rangeCount*2);
    for(int32_t i=0;i<rangeCount;i++) {
      printf(" 0x%04X, 0x%04X, // %d\n",
             unsafeBackwardSet->getRangeStart(i),
             unsafeBackwardSet->getRangeEnd(i),
             i);
    }
    printf("};\n");
#endif

#if SERIALIZE
    printf("#define COLLUNSAFE_SERIALIZE 1\n");    
    printf("static const int32_t unsafe_serializedCount = %d;\n", serializedCount);
    printf("static const uint16_t unsafe_serializedData[%d] = { \n", serializedCount);
    for(int32_t i=0;i<serializedCount;i++) {
      if( (i>0) && (i%8 == 0) ) {
        printf(" // %d\n", i);
      }
      printf("0x%04X", serializedData[i]); // TODO check
      if(i != (serializedCount-1)) {
        printf(", ");
      }
    }  
    printf("};\n");
#endif
    
    printf("#endif\n");
    fflush(stderr);
    fflush(stdout);
    return(U_SUCCESS(errorCode)?0:1);
}
