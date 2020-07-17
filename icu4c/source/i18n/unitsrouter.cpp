// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include <stdio.h>
#include <utility>

#include "cmemory.h"
#include "cstring.h"
#include "number_decimalquantity.h"
#include "resource.h"
#include "unitconverter.h" // for extractCompoundBaseUnit
#include "unitsdata.h"     // for getUnitCategory
#include "unitsrouter.h"
#include "uresimp.h"

U_NAMESPACE_BEGIN
namespace units {

UnitsRouter::UnitsRouter(MeasureUnit inputUnit, StringPiece region, StringPiece usage,
                         UErrorCode &status) {
    // TODO: do we want to pass in ConversionRates and UnitPreferences instead
    // of loading in each UnitsRouter instance? (Or make global?)
    ConversionRates conversionRates(status);
    UnitPreferences prefs(status);

    MeasureUnit baseUnit = extractCompoundBaseUnit(inputUnit, conversionRates, status);
    CharString category = getUnitCategory(baseUnit.getIdentifier(), status);

    const UnitPreference *const *unitPreferences;
    int32_t preferencesCount;
    prefs.getPreferencesFor(category.data(), usage, region, unitPreferences, preferencesCount, status);

    for (int i = 0; i < preferencesCount; ++i) {
        const auto &preference = *unitPreferences[i];

        MeasureUnit complexTargetUnit = MeasureUnit::forIdentifier(preference.unit.data(), status);
        if (U_FAILURE(status)) {
            return;
        }

        UnicodeString precision = preference.skeleton;

        // For now, we only have "precision-increment" in Units Preferences skeleton.
        // Therefore, we check if the skeleton starts with "precision-increment" and force the program to
        // fail otherwise.
        // NOTE:
        //  It is allowed to have an empty precision.
        if (precision.length() != 0) {
            if (precision.startsWith(u"precision-increment", 0, 19)) break;

            status = U_INTERNAL_PROGRAM_ERROR;
            return;
        }

        outputUnits_.emplaceBackAndCheckErrorCode(status, complexTargetUnit);
        converterPreferences_.emplaceBackAndCheckErrorCode(status, inputUnit, complexTargetUnit,
                                                           preference.geq, preference.skeleton,
                                                           conversionRates, status);

        if (U_FAILURE(status)) {
            return;
        }
    }
}

RouteResult UnitsRouter::route(double quantity, UErrorCode &status) const {
    for (int i = 0, n = converterPreferences_.length(); i < n; i++) {
        const auto &converterPreference = *converterPreferences_[i];

        if (converterPreference.converter.greaterThanOrEqual(quantity, converterPreference.limit)) {
            return RouteResult{
                converterPreference.converter.convert(quantity, status), //
                converterPreference.precision                            //
            };
        }
    }

    // In case of the `quantity` does not fit in any converter limit, use the last converter.
    const auto &lastConverterPreference = (*converterPreferences_[converterPreferences_.length() - 1]);
    return RouteResult{
        lastConverterPreference.converter.convert(quantity, status), //
        lastConverterPreference.precision                            //
    };
}

const MaybeStackVector<MeasureUnit> *UnitsRouter::getOutputUnits() const {
    // TODO: consider pulling this from converterPreferences_ and dropping
    // outputUnits_?
    return &outputUnits_;
}

} // namespace units
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
