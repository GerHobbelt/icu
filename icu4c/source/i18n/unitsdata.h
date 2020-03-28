// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING
#ifndef __GETUNITSDATA_H__
#define __GETUNITSDATA_H__

#include "charstr.h"
#include "cmemory.h"
#include "unicode/measunit.h"
#include "unicode/stringpiece.h"
#include "unicode/utypes.h"

U_NAMESPACE_BEGIN

// Encapsulates "convertUnits" information from units resources, specifying how
// to convert from one unit to another.
class U_I18N_API ConversionRateInfo {
  public:
    ConversionRateInfo(){};
    ConversionRateInfo(StringPiece sourceUnit, StringPiece baseUnit, StringPiece factor,
                       StringPiece offset, UErrorCode &status)
        : sourceUnit(), baseUnit(), factor(), offset() {
        this->sourceUnit.append(sourceUnit, status);
        this->baseUnit.append(baseUnit, status);
        this->factor.append(factor, status);
        this->offset.append(offset, status);
    };
    CharString sourceUnit;
    CharString baseUnit; // FIXME/WIP: baseUnit
    CharString factor;
    CharString offset;
    bool reciprocal = false;
};

/**
 * Collects and returns ConversionRateInfo needed to convert from source to
 * baseUnit.
 *
 * If source and target are not compatible for conversion, status will be set to
 * U_ILLEGAL_ARGUMENT_ERROR.
 *
 * @param source The source unit (the unit type converted from).
 * @param target The target unit (the unit type converted to).
 * @param baseCompoundUnit Output parameter: if not NULL, it will be set to the
 * compound base unit type used as pivot for converting from source to target.
 * @param status Receives status.
 */
MaybeStackVector<ConversionRateInfo> U_I18N_API getConversionRatesInfo(MeasureUnit source,
                                                                       MeasureUnit target,
                                                                       MeasureUnit *baseCompoundUnit,
                                                                       UErrorCode &status);

U_NAMESPACE_END

#endif //__GETUNITSDATA_H__

#endif /* #if !UCONFIG_NO_FORMATTING */
