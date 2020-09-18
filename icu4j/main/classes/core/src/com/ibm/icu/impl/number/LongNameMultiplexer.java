// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html


package com.ibm.icu.impl.number;

import com.ibm.icu.number.NumberFormatter;
import com.ibm.icu.text.PluralRules;
import com.ibm.icu.util.ICUException;
import com.ibm.icu.util.MeasureUnit;
import com.ibm.icu.util.NoUnit;
import com.ibm.icu.util.ULocale;

import java.util.ArrayList;
import java.util.List;

public class LongNameMultiplexer implements MicroPropsGenerator {
    private final MicroPropsGenerator fParent;

    private List<MicroPropsGenerator> fHandlers;

    // Each MeasureUnit corresponds to the same-index MicroPropsGenerator
    // pointed to in fHandlers.
    private List<MeasureUnit> fMeasureUnits;

    public LongNameMultiplexer(MicroPropsGenerator fParent) {
        this.fParent = fParent;
    }

    // Produces a multiplexer for LongNameHandlers, one for each unit in
    // `units`. An individual unit might be a mixed unit.
    public static LongNameMultiplexer forMeasureUnits(ULocale locale,
                                                      List<MeasureUnit> units,
                                                      NumberFormatter.UnitWidth width,
                                                      PluralRules rules,
                                                      MicroPropsGenerator parent) {
        LongNameMultiplexer result = new LongNameMultiplexer(parent);

        assert (units.size() > 0);

        result.fMeasureUnits = new ArrayList<>();
        result.fHandlers = new ArrayList<>();


        for (int i = 0; i < units.size(); i++) {
            MeasureUnit unit = units.get(i);
            result.fMeasureUnits.add(unit);
            if (unit.getComplexity() == MeasureUnit.Complexity.MIXED) {
                MixedUnitLongNameHandler mlnh = MixedUnitLongNameHandler
                        .forMeasureUnit(locale, unit, width, rules, null);
                result.fHandlers.add(mlnh);
            } else {
                LongNameHandler lnh = LongNameHandler
                        .forMeasureUnit(locale, unit, NoUnit.BASE, width, rules, null );
                result.fHandlers.add(lnh);
            }
        }

        return result;
    }

    // The output unit must be provided via `micros.outputUnit`, it must match
    // one of the units provided to the factory function.
    @Override
    public MicroProps processQuantity(DecimalQuantity quantity) {

        // We call parent->processQuantity() from the Multiplexer, instead of
        // letting LongNameHandler handle it: we don't know which LongNameHandler to
        // call until we've called the parent!
        MicroProps micros = this.fParent.processQuantity(quantity);

        // Call the correct LongNameHandler based on outputUnit
        for (int i = 0; i < this.fHandlers.size(); i++) {
            if (fMeasureUnits.get(i).equals(micros.outputUnit)) {
                MicroPropsGenerator handler = fHandlers.get(i);

                if (handler instanceof MixedUnitLongNameHandler) {
                    return ((MixedUnitLongNameHandler) handler).processQuantityWithMicros(quantity, micros);
                }

                if (handler instanceof LongNameHandler) {
                    return ((LongNameHandler) handler).processQuantityWithMicros(quantity, micros);
                }

                throw new ICUException("FIXME(exception) - BAD HANDLER");
            }

        }

        throw new AssertionError
                (" We shouldn't receive any outputUnit for which we haven't already got a LongNameHandler");
    }
}
