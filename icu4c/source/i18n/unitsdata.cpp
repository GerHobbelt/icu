// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include <utility>

#include "cmemory.h"
#include "cstring.h"
#include "number_decimalquantity.h"
#include "resource.h"
#include "unitsdata.h"
#include "uresimp.h"

U_NAMESPACE_BEGIN

namespace {

using icu::number::impl::DecimalQuantity;

/**
 * A ResourceSink that collects conversion rate information.
 *
 * This class is for use by ures_getAllItemsWithFallback. Example code for
 * collecting conversion info for "mile" and "foot" into conversionInfoOutput:
 *
 *     UErrorCode status = U_ZERO_ERROR;
 *     ures_getByKey(unitsBundle, "convertUnits", &fillIn, &status);
 *     MaybeStackVector<ConversionRateInfo> conversionInfoOutput;
 *     ConversionRateDataSink convertSink(conversionInfoOutput);
 *     ures_getAllItemsWithFallback(fillIn, "mile", convertSink, status);
 *     ures_getAllItemsWithFallback(fillIn, "foot", convertSink, status);
 */
class ConversionRateDataSink : public ResourceSink {
  public:
    /**
     * Constructor.
     * @param out The vector to which ConversionRateInfo instances are to be
     * added.
     */
    explicit ConversionRateDataSink(MaybeStackVector<ConversionRateInfo> &out) : outVector(out) {}

    /**
     * Adds the conversion rate information found in value to the output vector.
     *
     * Each call to put() collects a ConversionRateInfo instance for the
     * specified source unit identifier into the vector passed to the
     * constructor, but only if an identical instance isn't already present.
     *
     * @param source The source unit identifier.
     * @param value A resource containing conversion rate info (the base unit
     * and factor, and possibly an offset).
     * @param noFallback Ignored.
     * @param status The standard ICU error code output parameter.
     */
    void put(const char *source, ResourceValue &value, UBool /*noFallback*/, UErrorCode &status) {
        if (U_FAILURE(status)) return;
        ResourceTable conversionRateTable = value.getTable(status);
        if (U_FAILURE(status)) return;

        // Collect base unit, factor and offset from the resource.
        int32_t lenSource = uprv_strlen(source);
        const UChar *baseUnit = NULL, *factor = NULL, *offset = NULL;
        int32_t lenBaseUnit, lenFactor, lenOffset;
        const char *key;
        for (int32_t i = 0; conversionRateTable.getKeyAndValue(i, key, value); ++i) {
            if (uprv_strcmp(key, "target") == 0) {
                baseUnit = value.getString(lenBaseUnit, status);
            } else if (uprv_strcmp(key, "factor") == 0) {
                factor = value.getString(lenFactor, status);
            } else if (uprv_strcmp(key, "offset") == 0) {
                offset = value.getString(lenOffset, status);
            }
        }
        if (U_FAILURE(status)) return;
        if (baseUnit == NULL || factor == NULL) {
            // We could not find a usable conversion rate.
            status = U_MISSING_RESOURCE_ERROR;
            return;
        }

        // Check if we already have the conversion rate in question.
        //
        // TODO(review): We could do this skip-check *before* we fetch
        // baseUnit/factor/offset based only on the source unit, but only if
        // we're certain we'll never get two different baseUnits for a given
        // source. This should be the case, since convertUnit entries in CLDR's
        // units.xml should all point at a defined base unit for the unit
        // category. I should make this code more efficient after
        // double-checking we're fine with relying on such a detail from the
        // CLDR spec?
        fLastBaseUnit.clear();
        fLastBaseUnit.appendInvariantChars(baseUnit, lenBaseUnit, status);
        if (U_FAILURE(status)) return;
        for (int32_t i = 0, len = outVector.length(); i < len; i++) {
            if (strcmp(outVector[i]->sourceUnit.data(), source) == 0 &&
                strcmp(outVector[i]->baseUnit.data(), fLastBaseUnit.data()) == 0) {
                return;
            }
        }

        // We don't have this ConversionRateInfo yet: add it.
        ConversionRateInfo *cr = outVector.emplaceBack();
        if (!cr) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        } else {
            cr->sourceUnit.append(source, lenSource, status);
            cr->baseUnit.append(fLastBaseUnit.data(), fLastBaseUnit.length(), status);
            cr->factor.appendInvariantChars(factor, lenFactor, status);
            if (offset != NULL) cr->offset.appendInvariantChars(offset, lenOffset, status);
        }
    }

    /**
     * Returns the MeasureUnit that was the conversion base unit of the most
     * recent call to put() - typically meaning the most recent call to
     * ures_getAllItemsWithFallback().
     */
    MeasureUnit getLastBaseUnit(UErrorCode &status) {
        return MeasureUnit::forIdentifier(fLastBaseUnit.data(), status);
    }

  private:
    MaybeStackVector<ConversionRateInfo> &outVector;

    // TODO(review): felt like a hack: provides easy access to the most recent
    // baseUnit. This hack is another point making me wonder if doing this
    // ResourceSink thing is worthwhile. Functional style is not more verbose,
    // and IMHO more readable than this object-based approach where the output
    // seems/feels like a side-effect.
    CharString fLastBaseUnit;
};

// WIP/FIXME: partially due to my skepticism of using the ResourceSink design
// for units resources, this class is currently unused code (dead?) -
// collectUnitPrefs() has all the features we need whereas this doesn't handle
// fallback to usage="default" and region="001" yet. If we want to fix that
// here, this code will get quite a bit more complicated.
class UnitPreferencesSink : public ResourceSink {
  public:
    explicit UnitPreferencesSink(MaybeStackVector<UnitPreference> &out) : outVector(out) {}

    void put(const char *key, ResourceValue &value, UBool /*noFallback*/, UErrorCode &status) {
        if (U_FAILURE(status)) { return; }
        int32_t prefLen;
        ResourceArray unitPrefs = value.getArray(status);
        if (U_FAILURE(status)) { return; }
        prefLen = unitPrefs.getSize();
        for (int32_t i = 0; unitPrefs.getValue(i, value); i++) {
            UnitPreference *up = outVector.emplaceBack();
            if (!up) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            ResourceTable unitPref = value.getTable(status);
            if (U_FAILURE(status)) { return; }
            for (int32_t i = 0; unitPref.getKeyAndValue(i, key, value); ++i) {
                if (uprv_strcmp(key, "unit") == 0) {
                    int32_t length;
                    const UChar *u = value.getString(length, status);
                    up->unit.appendInvariantChars(u, length, status);
                } else if (uprv_strcmp(key, "geq") == 0) {
                    int32_t length;
                    const UChar *g = value.getString(length, status);
                    CharString geq;
                    geq.appendInvariantChars(g, length, status);
                    DecimalQuantity dq;
                    dq.setToDecNumber(geq.data(), status);
                    up->geq = dq.toDouble();
                } else if (uprv_strcmp(key, "skeleton") == 0) {
                    int32_t length;
                    const UChar *s = value.getString(length, status);
                    up->skeleton.appendInvariantChars(s, length, status);
                }
            }
        }
    }

  private:
    MaybeStackVector<UnitPreference> &outVector;
};

/**
 * Collects unit preference information from a set of preferences.
 * @param usageData This should be a resource bundle containing a vector of
 * preferences - i.e. the unitPreferenceData tree resources already narrowed
 * down to a particular usage and region (example:
 * "unitPreferenceData/length/road/GB").
 */
void collectUnitPrefs(UResourceBundle *usageData, MaybeStackVector<UnitPreference> &outVector,
                      UErrorCode &status) {
    if (U_FAILURE(status)) return;
    StackUResourceBundle prefBundle;

    int32_t numPrefs = ures_getSize(usageData);
    for (int32_t i = 0; i < numPrefs; i++) {
        ures_getByIndex(usageData, i, prefBundle.getAlias(), &status);

        // Add and populate a new UnitPreference
        int32_t strLen;

        // unit
        const UChar *unitIdent = ures_getStringByKey(prefBundle.getAlias(), "unit", &strLen, &status);
        if (U_FAILURE(status)) return;
        UnitPreference *up = outVector.emplaceBack();
        if (!up) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        up->unit.appendInvariantChars(unitIdent, strLen, status);

        // geq
        const UChar *geq = ures_getStringByKey(prefBundle.getAlias(), "geq", &strLen, &status);
        if (U_SUCCESS(status)) {
            // If we don't mind up->geq having a bad value when
            // U_FAILURE(status), we could extract a function and do a one-liner:
            // up->geq = UCharsToDouble(geq, status);
            CharString cGeq;
            cGeq.appendInvariantChars(geq, strLen, status);
            DecimalQuantity dq;
            dq.setToDecNumber(StringPiece(cGeq.data()), status);
            if (U_FAILURE(status)) return;
            up->geq = dq.toDouble();
        } else if (status == U_MISSING_RESOURCE_ERROR) {
            // We don't mind if geq is missing
            status = U_ZERO_ERROR;
        } else {
            return;
        }

        // skeleton
        const UChar *skel = ures_getStringByKey(prefBundle.getAlias(), "skeleton", &strLen, &status);
        if (U_SUCCESS(status)) {
            up->skeleton.appendInvariantChars(skel, strLen, status);
        } else if (status == U_MISSING_RESOURCE_ERROR) {
            // We don't mind if geq is missing
            status = U_ZERO_ERROR;
        } else {
            return;
        }
    }
}

/**
 * Collects conversion information for a "single unit" (a unit whose complexity
 * is UMEASURE_UNIT_SINGLE).
 *
 * This function currently only supports higher-dimensionality input units if
 * they map to "single unit" output units. This means it don't support
 * square-bar, one-per-bar, square-joule or one-per-joule. (Some unit types in
 * this class: volume, consumption, torque, force, pressure, speed,
 * acceleration, and more).
 *
 * TODO(hugovdm): maybe find and share (in documentation) a solid argument for
 * why these kinds of input units won't be needed with higher dimensionality? Or
 * start supporting them... Also: add unit tests demonstrating the
 * U_ILLEGAL_ARGUMENT_ERROR returned for such units.
 *
 * @param unit The input unit. Its complexity must be UMEASURE_UNIT_SINGLE, but
 * it may have a dimensionality != 1.
 * @param converUnitsBundle A UResourceBundle instance for the convertUnits
 * resource.
 * @param convertSink The ConversionRateDataSink through which
 * ConversionRateInfo instances are to be collected.
 * @param status The standard ICU error code output parameter.
 */
void processSingleUnit(const MeasureUnit &unit, const UResourceBundle *convertUnitsBundle,
                       ConversionRateDataSink &convertSink, UErrorCode &status) {
    if (U_FAILURE(status)) return;
    int32_t dimensionality = unit.getDimensionality(status);

    // Fetch the relevant entry in convertUnits.
    MeasureUnit simple = unit;
    if (dimensionality != 1 || simple.getSIPrefix(status) != UMEASURE_SI_PREFIX_ONE) {
        simple = unit.withDimensionality(1, status).withSIPrefix(UMEASURE_SI_PREFIX_ONE, status);
    }
    ures_getAllItemsWithFallback(convertUnitsBundle, simple.getIdentifier(), convertSink, status);
}

} // namespace

MaybeStackVector<ConversionRateInfo> U_I18N_API
getConversionRatesInfo(const MaybeStackVector<MeasureUnit> &units, UErrorCode &status) {
    MaybeStackVector<ConversionRateInfo> result;
    if (U_FAILURE(status)) return result;

    LocalUResourceBundlePointer unitsBundle(ures_openDirect(NULL, "units", &status));
    StackUResourceBundle convertUnitsBundle;
    ures_getByKey(unitsBundle.getAlias(), "convertUnits", convertUnitsBundle.getAlias(), &status);
    ConversionRateDataSink convertSink(result);

    for (int i = 0; i < units.length(); i++) {
        int32_t numSingleUnits;
        LocalArray<MeasureUnit> singleUnits = units[i]->splitToSingleUnits(numSingleUnits, status);

        for (int i = 0; i < numSingleUnits; i++) {
            processSingleUnit(singleUnits[i], convertUnitsBundle.getAlias(), convertSink, status);
        }
    }
    return result;
}

/**
 * Fetches the units data that would be needed for the given usage.
 *
 * @param inputUnit the unit for which input is expected. (NOTE/WIP: If this is
 * known to be a base unit already, we could strip some logic here.)
 */
void getUnitsData(const char *outputRegion, const char *usage, const MeasureUnit &inputUnit,
                  CharString &category, MeasureUnit &baseUnit,
                  MaybeStackVector<ConversionRateInfo> &conversionRates,
                  MaybeStackVector<UnitPreference> &unitPreferences, UErrorCode &status) {
    // This first fetches conversion info for the inputUnit, to find out the
    // base unit. Next it fetches the category and unit preferences for the
    // given usage and region. Finally it fetches conversion rates again, for
    // each of the units in the regional preferences for the given usage.

    // In this function we use LocalUResourceBundlePointers for resource bundles
    // that don't change, and StackUResourceBundles for structures we use as
    // fillin.
    LocalUResourceBundlePointer unitsBundle(ures_openDirect(NULL, "units", &status));
    StackUResourceBundle convertUnitsBundle;
    ConversionRateDataSink convertSink(conversionRates);

    // baseUnit
    MeasureUnit inputBase = inputUnit.withSIPrefix(UMEASURE_SI_PREFIX_ONE, status);
    ures_getByKey(unitsBundle.getAlias(), "convertUnits", convertUnitsBundle.getAlias(), &status);
    ures_getAllItemsWithFallback(convertUnitsBundle.getAlias(), inputBase.getIdentifier(), convertSink,
                                 status);
    if (U_FAILURE(status)) return;
    if (conversionRates.length() < 1) {
        // This is defensive programming, because this shouldn't happen: if
        // convertSink succeeds, there should be at least one item in
        // conversionRates.
        status = U_MISSING_RESOURCE_ERROR;
        return;
    }
    const char *baseIdentifier = conversionRates[0]->baseUnit.data();
    baseUnit = MeasureUnit::forIdentifier(baseIdentifier, status);

    // category
    LocalUResourceBundlePointer unitQuantities(
        ures_getByKey(unitsBundle.getAlias(), "unitQuantities", NULL, &status));
    int32_t categoryLength;
    const UChar *uCategory =
        ures_getStringByKey(unitQuantities.getAlias(), baseIdentifier, &categoryLength, &status);
    category.appendInvariantChars(uCategory, categoryLength, status);

    // Find the right unit preference bundle
    StackUResourceBundle stackBundle; // Reused as we climb the tree
    ures_getByKey(unitsBundle.getAlias(), "unitPreferenceData", stackBundle.getAlias(), &status);
    ures_getByKey(stackBundle.getAlias(), category.data(), stackBundle.getAlias(), &status);
    if (U_FAILURE(status)) { return; }
    ures_getByKey(stackBundle.getAlias(), usage, stackBundle.getAlias(), &status);
    if (status == U_MISSING_RESOURCE_ERROR) {
        // Requested usage does not exist, so we use "default".
        status = U_ZERO_ERROR;
        ures_getByKey(stackBundle.getAlias(), "default", stackBundle.getAlias(), &status);
    }
    ures_getByKey(stackBundle.getAlias(), outputRegion, stackBundle.getAlias(), &status);
    if (status == U_MISSING_RESOURCE_ERROR) {
        // Requested region does not exist, so we use "001".
        status = U_ZERO_ERROR;
        ures_getByKey(stackBundle.getAlias(), "001", stackBundle.getAlias(), &status);
    }

    // Collect all the preferences into unitPreferences
    collectUnitPrefs(stackBundle.getAlias(), unitPreferences, status);

    // Load ConversionRateInfo for each of the units in unitPreferences
    for (int32_t i = 0; i < unitPreferences.length(); i++) {
        MeasureUnit prefUnitBase = MeasureUnit::forIdentifier(unitPreferences[i]->unit.data(), status)
                                       .withSIPrefix(UMEASURE_SI_PREFIX_ONE, status);
        // convertSink will skip conversion rates we already have
        ures_getAllItemsWithFallback(convertUnitsBundle.getAlias(), prefUnitBase.getIdentifier(),
                                     convertSink, status);
    }
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
