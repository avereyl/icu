// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING
#ifndef __NUMBERFORMATTER_H__
#define __NUMBERFORMATTER_H__

#include "unicode/appendable.h"
#include "unicode/dcfmtsym.h"
#include "unicode/currunit.h"
#include "unicode/fieldpos.h"
#include "unicode/fpositer.h"
#include "unicode/measunit.h"
#include "unicode/unumberformatter.h"
#include "unicode/nounit.h"
#include "unicode/plurrule.h"
#include "unicode/ucurr.h"
#include "unicode/unum.h"
#include "unicode/uobject.h"

#ifndef U_HIDE_DRAFT_API

/**
 * \file
 * \brief C++ API: Library for localized number formatting introduced in ICU 60.
 *
 * This library was introduced in ICU 60 to simplify the process of formatting localized number strings.
 * Basic usage examples:
 *
 * <pre>
 * // Most basic usage:
 * NumberFormatter::withLocale(...).format(123).toString();  // 1,234 in en-US
 *
 * // Custom notation, unit, and rounding strategy:
 * NumberFormatter::with()
 *     .notation(Notation::compactShort())
 *     .unit(CurrencyUnit("EUR", status))
 *     .rounding(Rounder::maxDigits(2))
 *     .locale(...)
 *     .format(1234)
 *     .toString();  // €1.2K in en-US
 *
 * // Create a formatter in a singleton for use later:
 * static const LocalizedNumberFormatter formatter = NumberFormatter::withLocale(...)
 *     .unit(NoUnit::percent())
 *     .rounding(Rounder::fixedFraction(3));
 * formatter.format(5.9831).toString();  // 5.983% in en-US
 *
 * // Create a "template" in a singleton but without setting a locale until the call site:
 * static const UnlocalizedNumberFormatter template = NumberFormatter::with()
 *     .sign(UNumberSignDisplay::UNUM_SIGN_ALWAYS)
 *     .adoptUnit(MeasureUnit::createMeter(status))
 *     .unitWidth(UNumberUnitWidth::UNUM_UNIT_WIDTH_FULL_NAME);
 * template.locale(...).format(1234).toString();  // +1,234 meters in en-US
 * </pre>
 *
 * <p>
 * This API offers more features than DecimalFormat and is geared toward new users of ICU.
 *
 * <p>
 * NumberFormatter instances are immutable and thread safe. This means that invoking a configuration method has no
 * effect on the receiving instance; you must store and use the new number formatter instance it returns instead.
 *
 * <pre>
 * UnlocalizedNumberFormatter formatter = UnlocalizedNumberFormatter::with().notation(Notation::scientific());
 * formatter.rounding(Rounder.maxFraction(2)); // does nothing!
 * formatter.locale(Locale.getEnglish()).format(9.8765).toString(); // prints "9.8765E0", not "9.88E0"
 * </pre>
 *
 * <p>
 * This API is based on the <em>fluent</em> design pattern popularized by libraries such as Google's Guava. For
 * extensive details on the design of this API, read <a href="https://goo.gl/szi5VB">the design doc</a>.
 *
 * @author Shane Carr
 */

U_NAMESPACE_BEGIN

// Forward declarations:
class IFixedDecimal;

namespace numparse {
namespace impl {

// Forward declarations:
class NumberParserImpl;

}
}

namespace number {  // icu::number

// Forward declarations:
class UnlocalizedNumberFormatter;
class LocalizedNumberFormatter;
class FormattedNumber;
class Notation;
class ScientificNotation;
class Rounder;
class FractionRounder;
class CurrencyRounder;
class IncrementRounder;
class IntegerWidth;

namespace impl {

#ifndef U_HIDE_INTERNAL_API
/**
 * Datatype for minimum/maximum fraction digits. Must be able to hold kMaxIntFracSig.
 *
 * @internal
 */
typedef int16_t digits_t;

/**
 * Use a default threshold of 3. This means that the third time .format() is called, the data structures get built
 * using the "safe" code path. The first two calls to .format() will trigger the unsafe code path.
 *
 * @internal
 */
static constexpr int32_t DEFAULT_THRESHOLD = 3;
#endif  // U_HIDE_INTERNAL_API

// Forward declarations:
class Padder;
struct MacroProps;
struct MicroProps;
class DecimalQuantity;
struct UFormattedNumberData;
class NumberFormatterImpl;
struct ParsedPatternInfo;
class ScientificModifier;
class MultiplierProducer;
class MutablePatternModifier;
class LongNameHandler;
class ScientificHandler;
class CompactHandler;
class Modifier;
class NumberStringBuilder;
class AffixPatternProvider;
class NumberPropertyMapper;
struct DecimalFormatProperties;
class MultiplierChain;
class CurrencySymbols;
class GeneratorHelpers;

} // namespace impl

// Reserve extra names in case they are added as classes in the future:
typedef Notation CompactNotation;
typedef Notation SimpleNotation;

/**
 * A class that defines the notation style to be used when formatting numbers in NumberFormatter.
 *
 * @draft ICU 60
 */
class U_I18N_API Notation : public UMemory {
  public:
    /**
     * Print the number using scientific notation (also known as scientific form, standard index form, or standard form
     * in the UK). The format for scientific notation varies by locale; for example, many Western locales display the
     * number in the form "#E0", where the number is displayed with one digit before the decimal separator, zero or more
     * digits after the decimal separator, and the corresponding power of 10 displayed after the "E".
     *
     * <p>
     * Example outputs in <em>en-US</em> when printing 8.765E4 through 8.765E-3:
     *
     * <pre>
     * 8.765E4
     * 8.765E3
     * 8.765E2
     * 8.765E1
     * 8.765E0
     * 8.765E-1
     * 8.765E-2
     * 8.765E-3
     * 0E0
     * </pre>
     *
     * @return A ScientificNotation for chaining or passing to the NumberFormatter notation() setter.
     * @draft ICU 60
     */
    static ScientificNotation scientific();

    /**
     * Print the number using engineering notation, a variant of scientific notation in which the exponent must be
     * divisible by 3.
     *
     * <p>
     * Example outputs in <em>en-US</em> when printing 8.765E4 through 8.765E-3:
     *
     * <pre>
     * 87.65E3
     * 8.765E3
     * 876.5E0
     * 87.65E0
     * 8.765E0
     * 876.5E-3
     * 87.65E-3
     * 8.765E-3
     * 0E0
     * </pre>
     *
     * @return A ScientificNotation for chaining or passing to the NumberFormatter notation() setter.
     * @draft ICU 60
     */
    static ScientificNotation engineering();

    /**
     * Print the number using short-form compact notation.
     *
     * <p>
     * <em>Compact notation</em>, defined in Unicode Technical Standard #35 Part 3 Section 2.4.1, prints numbers with
     * localized prefixes or suffixes corresponding to different powers of ten. Compact notation is similar to
     * engineering notation in how it scales numbers.
     *
     * <p>
     * Compact notation is ideal for displaying large numbers (over ~1000) to humans while at the same time minimizing
     * screen real estate.
     *
     * <p>
     * In short form, the powers of ten are abbreviated. In <em>en-US</em>, the abbreviations are "K" for thousands, "M"
     * for millions, "B" for billions, and "T" for trillions. Example outputs in <em>en-US</em> when printing 8.765E7
     * through 8.765E0:
     *
     * <pre>
     * 88M
     * 8.8M
     * 876K
     * 88K
     * 8.8K
     * 876
     * 88
     * 8.8
     * </pre>
     *
     * <p>
     * When compact notation is specified without an explicit rounding strategy, numbers are rounded off to the closest
     * integer after scaling the number by the corresponding power of 10, but with a digit shown after the decimal
     * separator if there is only one digit before the decimal separator. The default compact notation rounding strategy
     * is equivalent to:
     *
     * <pre>
     * Rounder.integer().withMinDigits(2)
     * </pre>
     *
     * @return A CompactNotation for passing to the NumberFormatter notation() setter.
     * @draft ICU 60
     */
    static CompactNotation compactShort();

    /**
     * Print the number using long-form compact notation. For more information on compact notation, see
     * {@link #compactShort}.
     *
     * <p>
     * In long form, the powers of ten are spelled out fully. Example outputs in <em>en-US</em> when printing 8.765E7
     * through 8.765E0:
     *
     * <pre>
     * 88 million
     * 8.8 million
     * 876 thousand
     * 88 thousand
     * 8.8 thousand
     * 876
     * 88
     * 8.8
     * </pre>
     *
     * @return A CompactNotation for passing to the NumberFormatter notation() setter.
     * @draft ICU 60
     */
    static CompactNotation compactLong();

    /**
     * Print the number using simple notation without any scaling by powers of ten. This is the default behavior.
     *
     * <p>
     * Since this is the default behavior, this method needs to be called only when it is necessary to override a
     * previous setting.
     *
     * <p>
     * Example outputs in <em>en-US</em> when printing 8.765E7 through 8.765E0:
     *
     * <pre>
     * 87,650,000
     * 8,765,000
     * 876,500
     * 87,650
     * 8,765
     * 876.5
     * 87.65
     * 8.765
     * </pre>
     *
     * @return A SimpleNotation for passing to the NumberFormatter notation() setter.
     * @draft ICU 60
     */
    static SimpleNotation simple();

  private:
    enum NotationType {
        NTN_SCIENTIFIC, NTN_COMPACT, NTN_SIMPLE, NTN_ERROR
    } fType;

    union NotationUnion {
        // For NTN_SCIENTIFIC
        struct ScientificSettings {
            int8_t fEngineeringInterval;
            bool fRequireMinInt;
            impl::digits_t fMinExponentDigits;
            UNumberSignDisplay fExponentSignDisplay;
        } scientific;

        // For NTN_COMPACT
        UNumberCompactStyle compactStyle;

        // For NTN_ERROR
        UErrorCode errorCode;
    } fUnion;

    typedef NotationUnion::ScientificSettings ScientificSettings;

    Notation(const NotationType &type, const NotationUnion &union_) : fType(type), fUnion(union_) {}

    Notation(UErrorCode errorCode) : fType(NTN_ERROR) {
        fUnion.errorCode = errorCode;
    }

    Notation() : fType(NTN_SIMPLE), fUnion() {}

    UBool copyErrorTo(UErrorCode &status) const {
        if (fType == NTN_ERROR) {
            status = fUnion.errorCode;
            return TRUE;
        }
        return FALSE;
    }

    // To allow MacroProps to initialize empty instances:
    friend struct impl::MacroProps;
    friend class ScientificNotation;

    // To allow implementation to access internal types:
    friend class impl::NumberFormatterImpl;
    friend class impl::ScientificModifier;
    friend class impl::ScientificHandler;

    // To allow access to the skeleton generation code:
    friend class impl::GeneratorHelpers;
};

/**
 * A class that defines the scientific notation style to be used when formatting numbers in NumberFormatter.
 *
 * <p>
 * To create a ScientificNotation, use one of the factory methods in {@link Notation}.
 *
 * @draft ICU 60
 */
class U_I18N_API ScientificNotation : public Notation {
  public:
    /**
     * Sets the minimum number of digits to show in the exponent of scientific notation, padding with zeros if
     * necessary. Useful for fixed-width display.
     *
     * <p>
     * For example, with minExponentDigits=2, the number 123 will be printed as "1.23E02" in <em>en-US</em> instead of
     * the default "1.23E2".
     *
     * @param minExponentDigits
     *            The minimum number of digits to show in the exponent.
     * @return A ScientificNotation, for chaining.
     * @draft ICU 60
     */
    ScientificNotation withMinExponentDigits(int32_t minExponentDigits) const;

    /**
     * Sets whether to show the sign on positive and negative exponents in scientific notation. The default is AUTO,
     * showing the minus sign but not the plus sign.
     *
     * <p>
     * For example, with exponentSignDisplay=ALWAYS, the number 123 will be printed as "1.23E+2" in <em>en-US</em>
     * instead of the default "1.23E2".
     *
     * @param exponentSignDisplay
     *            The strategy for displaying the sign in the exponent.
     * @return A ScientificNotation, for chaining.
     * @draft ICU 60
     */
    ScientificNotation withExponentSignDisplay(UNumberSignDisplay exponentSignDisplay) const;

  private:
    // Inherit constructor
    using Notation::Notation;

    // Raw constructor for NumberPropertyMapper
    ScientificNotation(int8_t fEngineeringInterval, bool fRequireMinInt, impl::digits_t fMinExponentDigits,
                       UNumberSignDisplay fExponentSignDisplay);

    friend class Notation;

    // So that NumberPropertyMapper can create instances
    friend class impl::NumberPropertyMapper;
};

// Reserve extra names in case they are added as classes in the future:
typedef Rounder DigitRounder;

/**
 * A class that defines the rounding strategy to be used when formatting numbers in NumberFormatter.
 *
 * <p>
 * To create a Rounder, use one of the factory methods.
 *
 * @draft ICU 60
 */
class U_I18N_API Rounder : public UMemory {

  public:
    /**
     * Show all available digits to full precision.
     *
     * <p>
     * <strong>NOTE:</strong> When formatting a <em>double</em>, this method, along with {@link #minFraction} and
     * {@link #minDigits}, will trigger complex algorithm similar to <em>Dragon4</em> to determine the low-order digits
     * and the number of digits to display based on the value of the double. If the number of fraction places or
     * significant digits can be bounded, consider using {@link #maxFraction} or {@link #maxDigits} instead to maximize
     * performance. For more information, read the following blog post.
     *
     * <p>
     * http://www.serpentine.com/blog/2011/06/29/here-be-dragons-advances-in-problems-you-didnt-even-know-you-had/
     *
     * @return A Rounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     */
    static Rounder unlimited();

    /**
     * Show numbers rounded if necessary to the nearest integer.
     *
     * @return A FractionRounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     */
    static FractionRounder integer();

    /**
     * Show numbers rounded if necessary to a certain number of fraction places (numerals after the decimal separator).
     * Additionally, pad with zeros to ensure that this number of places are always shown.
     *
     * <p>
     * Example output with minMaxFractionPlaces = 3:
     *
     * <p>
     * 87,650.000<br>
     * 8,765.000<br>
     * 876.500<br>
     * 87.650<br>
     * 8.765<br>
     * 0.876<br>
     * 0.088<br>
     * 0.009<br>
     * 0.000 (zero)
     *
     * <p>
     * This method is equivalent to {@link #minMaxFraction} with both arguments equal.
     *
     * @param minMaxFractionPlaces
     *            The minimum and maximum number of numerals to display after the decimal separator (rounding if too
     *            long or padding with zeros if too short).
     * @return A FractionRounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     */
    static FractionRounder fixedFraction(int32_t minMaxFractionPlaces);

    /**
     * Always show at least a certain number of fraction places after the decimal separator, padding with zeros if
     * necessary. Do not perform rounding (display numbers to their full precision).
     *
     * <p>
     * <strong>NOTE:</strong> If you are formatting <em>doubles</em>, see the performance note in {@link #unlimited}.
     *
     * @param minFractionPlaces
     *            The minimum number of numerals to display after the decimal separator (padding with zeros if
     *            necessary).
     * @return A FractionRounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     */
    static FractionRounder minFraction(int32_t minFractionPlaces);

    /**
     * Show numbers rounded if necessary to a certain number of fraction places (numerals after the decimal separator).
     * Unlike the other fraction rounding strategies, this strategy does <em>not</em> pad zeros to the end of the
     * number.
     *
     * @param maxFractionPlaces
     *            The maximum number of numerals to display after the decimal mark (rounding if necessary).
     * @return A FractionRounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     */
    static FractionRounder maxFraction(int32_t maxFractionPlaces);

    /**
     * Show numbers rounded if necessary to a certain number of fraction places (numerals after the decimal separator);
     * in addition, always show at least a certain number of places after the decimal separator, padding with zeros if
     * necessary.
     *
     * @param minFractionPlaces
     *            The minimum number of numerals to display after the decimal separator (padding with zeros if
     *            necessary).
     * @param maxFractionPlaces
     *            The maximum number of numerals to display after the decimal separator (rounding if necessary).
     * @return A FractionRounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     */
    static FractionRounder minMaxFraction(int32_t minFractionPlaces, int32_t maxFractionPlaces);

    /**
     * Show numbers rounded if necessary to a certain number of significant digits or significant figures. Additionally,
     * pad with zeros to ensure that this number of significant digits/figures are always shown.
     *
     * <p>
     * This method is equivalent to {@link #minMaxDigits} with both arguments equal.
     *
     * @param minMaxSignificantDigits
     *            The minimum and maximum number of significant digits to display (rounding if too long or padding with
     *            zeros if too short).
     * @return A Rounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     */
    static DigitRounder fixedDigits(int32_t minMaxSignificantDigits);

    /**
     * Always show at least a certain number of significant digits/figures, padding with zeros if necessary. Do not
     * perform rounding (display numbers to their full precision).
     *
     * <p>
     * <strong>NOTE:</strong> If you are formatting <em>doubles</em>, see the performance note in {@link #unlimited}.
     *
     * @param minSignificantDigits
     *            The minimum number of significant digits to display (padding with zeros if too short).
     * @return A Rounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     */
    static DigitRounder minDigits(int32_t minSignificantDigits);

    /**
     * Show numbers rounded if necessary to a certain number of significant digits/figures.
     *
     * @param maxSignificantDigits
     *            The maximum number of significant digits to display (rounding if too long).
     * @return A Rounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     */
    static DigitRounder maxDigits(int32_t maxSignificantDigits);

    /**
     * Show numbers rounded if necessary to a certain number of significant digits/figures; in addition, always show at
     * least a certain number of significant digits, padding with zeros if necessary.
     *
     * @param minSignificantDigits
     *            The minimum number of significant digits to display (padding with zeros if necessary).
     * @param maxSignificantDigits
     *            The maximum number of significant digits to display (rounding if necessary).
     * @return A Rounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     */
    static DigitRounder minMaxDigits(int32_t minSignificantDigits, int32_t maxSignificantDigits);

    /**
     * Show numbers rounded if necessary to the closest multiple of a certain rounding increment. For example, if the
     * rounding increment is 0.5, then round 1.2 to 1 and round 1.3 to 1.5.
     *
     * <p>
     * In order to ensure that numbers are padded to the appropriate number of fraction places, call
     * withMinFraction() on the return value of this method.
     * For example, to round to the nearest 0.5 and always display 2 numerals after the
     * decimal separator (to display 1.2 as "1.00" and 1.3 as "1.50"), you can run:
     *
     * <pre>
     * Rounder::increment(0.5).withMinFraction(2)
     * </pre>
     *
     * @param roundingIncrement
     *            The increment to which to round numbers.
     * @return A Rounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     */
    static IncrementRounder increment(double roundingIncrement);

    /**
     * Show numbers rounded and padded according to the rules for the currency unit. The most common rounding settings
     * for currencies include <code>Rounder.fixedFraction(2)</code>, <code>Rounder.integer()</code>, and
     * <code>Rounder.increment(0.05)</code> for cash transactions ("nickel rounding").
     *
     * <p>
     * The exact rounding details will be resolved at runtime based on the currency unit specified in the
     * NumberFormatter chain. To round according to the rules for one currency while displaying the symbol for another
     * currency, the withCurrency() method can be called on the return value of this method.
     *
     * @param currencyUsage
     *            Either STANDARD (for digital transactions) or CASH (for transactions where the rounding increment may
     *            be limited by the available denominations of cash or coins).
     * @return A CurrencyRounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     */
    static CurrencyRounder currency(UCurrencyUsage currencyUsage);

    /**
     * Sets the rounding mode to use when picking the direction to round (up or down). Common values
     * include HALF_EVEN, HALF_UP, and FLOOR. The default is HALF_EVEN.
     *
     * @param roundingMode
     *            The RoundingMode to use.
     * @return A Rounder for passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     */
    Rounder withMode(UNumberFormatRoundingMode roundingMode) const;

  private:
    enum RounderType {
        RND_BOGUS,
        RND_NONE,
        RND_FRACTION,
        RND_SIGNIFICANT,
        RND_FRACTION_SIGNIFICANT,
        RND_INCREMENT,
        RND_CURRENCY,
        RND_PASS_THROUGH,
        RND_ERROR
    } fType;

    union RounderUnion {
        struct FractionSignificantSettings {
            // For RND_FRACTION, RND_SIGNIFICANT, and RND_FRACTION_SIGNIFICANT
            impl::digits_t fMinFrac;
            impl::digits_t fMaxFrac;
            impl::digits_t fMinSig;
            impl::digits_t fMaxSig;
        } fracSig;
        struct IncrementSettings {
            double fIncrement;
            impl::digits_t fMinFrac;
            impl::digits_t fMaxFrac;
        } increment; // For RND_INCREMENT
        UCurrencyUsage currencyUsage; // For RND_CURRENCY
        UErrorCode errorCode; // For RND_ERROR
    } fUnion;

    typedef RounderUnion::FractionSignificantSettings FractionSignificantSettings;
    typedef RounderUnion::IncrementSettings IncrementSettings;

    UNumberFormatRoundingMode fRoundingMode;

    Rounder(const RounderType &type, const RounderUnion &union_, UNumberFormatRoundingMode roundingMode)
            : fType(type), fUnion(union_), fRoundingMode(roundingMode) {}

    Rounder(UErrorCode errorCode) : fType(RND_ERROR) {
        fUnion.errorCode = errorCode;
    }

    Rounder() : fType(RND_BOGUS) {}

    bool isBogus() const {
        return fType == RND_BOGUS;
    }

    UBool copyErrorTo(UErrorCode &status) const {
        if (fType == RND_ERROR) {
            status = fUnion.errorCode;
            return TRUE;
        }
        return FALSE;
    }

    // On the parent type so that this method can be called internally on Rounder instances.
    Rounder withCurrency(const CurrencyUnit &currency, UErrorCode &status) const;

    /** NON-CONST: mutates the current instance. */
    void setLocaleData(const CurrencyUnit &currency, UErrorCode &status);

    void apply(impl::DecimalQuantity &value, UErrorCode &status) const;

    /** Version of {@link #apply} that obeys minInt constraints. Used for scientific notation compatibility mode. */
    void apply(impl::DecimalQuantity &value, int32_t minInt, UErrorCode status);

    /**
     * Rounding endpoint used by Engineering and Compact notation. Chooses the most appropriate multiplier (magnitude
     * adjustment), applies the adjustment, rounds, and returns the chosen multiplier.
     *
     * <p>
     * In most cases, this is simple. However, when rounding the number causes it to cross a multiplier boundary, we
     * need to re-do the rounding. For example, to display 999,999 in Engineering notation with 2 sigfigs, first you
     * guess the multiplier to be -3. However, then you end up getting 1000E3, which is not the correct output. You then
     * change your multiplier to be -6, and you get 1.0E6, which is correct.
     *
     * @param input The quantity to process.
     * @param producer Function to call to return a multiplier based on a magnitude.
     * @return The number of orders of magnitude the input was adjusted by this method.
     */
    int32_t
    chooseMultiplierAndApply(impl::DecimalQuantity &input, const impl::MultiplierProducer &producer,
                             UErrorCode &status);

    static FractionRounder constructFraction(int32_t minFrac, int32_t maxFrac);

    static Rounder constructSignificant(int32_t minSig, int32_t maxSig);

    static Rounder
    constructFractionSignificant(const FractionRounder &base, int32_t minSig, int32_t maxSig);

    static IncrementRounder constructIncrement(double increment, int32_t minFrac);

    static CurrencyRounder constructCurrency(UCurrencyUsage usage);

    static Rounder constructPassThrough();

    // To allow MacroProps/MicroProps to initialize bogus instances:
    friend struct impl::MacroProps;
    friend struct impl::MicroProps;

    // To allow NumberFormatterImpl to access isBogus() and other internal methods:
    friend class impl::NumberFormatterImpl;

    // To allow NumberPropertyMapper to create instances from DecimalFormatProperties:
    friend class impl::NumberPropertyMapper;

    // To give access to apply() and chooseMultiplierAndApply():
    friend class impl::MutablePatternModifier;
    friend class impl::LongNameHandler;
    friend class impl::ScientificHandler;
    friend class impl::CompactHandler;

    // To allow child classes to call private methods:
    friend class FractionRounder;
    friend class CurrencyRounder;
    friend class IncrementRounder;

    // To allow access to the skeleton generation code:
    friend class impl::GeneratorHelpers;
};

/**
 * A class that defines a rounding strategy based on a number of fraction places and optionally significant digits to be
 * used when formatting numbers in NumberFormatter.
 *
 * <p>
 * To create a FractionRounder, use one of the factory methods on Rounder.
 *
 * @draft ICU 60
 */
class U_I18N_API FractionRounder : public Rounder {
  public:
    /**
     * Ensure that no less than this number of significant digits are retained when rounding according to fraction
     * rules.
     *
     * <p>
     * For example, with integer rounding, the number 3.141 becomes "3". However, with minimum figures set to 2, 3.141
     * becomes "3.1" instead.
     *
     * <p>
     * This setting does not affect the number of trailing zeros. For example, 3.01 would print as "3", not "3.0".
     *
     * @param minSignificantDigits
     *            The number of significant figures to guarantee.
     * @return A Rounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     */
    Rounder withMinDigits(int32_t minSignificantDigits) const;

    /**
     * Ensure that no more than this number of significant digits are retained when rounding according to fraction
     * rules.
     *
     * <p>
     * For example, with integer rounding, the number 123.4 becomes "123". However, with maximum figures set to 2, 123.4
     * becomes "120" instead.
     *
     * <p>
     * This setting does not affect the number of trailing zeros. For example, with fixed fraction of 2, 123.4 would
     * become "120.00".
     *
     * @param maxSignificantDigits
     *            Round the number to no more than this number of significant figures.
     * @return A Rounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     */
    Rounder withMaxDigits(int32_t maxSignificantDigits) const;

  private:
    // Inherit constructor
    using Rounder::Rounder;

    // To allow parent class to call this class's constructor:
    friend class Rounder;
};

/**
 * A class that defines a rounding strategy parameterized by a currency to be used when formatting numbers in
 * NumberFormatter.
 *
 * <p>
 * To create a CurrencyRounder, use one of the factory methods on Rounder.
 *
 * @draft ICU 60
 */
class U_I18N_API CurrencyRounder : public Rounder {
  public:
    /**
      * Associates a currency with this rounding strategy.
      *
      * <p>
      * <strong>Calling this method is <em>not required</em></strong>, because the currency specified in unit()
      * is automatically applied to currency rounding strategies. However,
      * this method enables you to override that automatic association.
      *
      * <p>
      * This method also enables numbers to be formatted using currency rounding rules without explicitly using a
      * currency format.
      *
      * @param currency
      *            The currency to associate with this rounding strategy.
      * @return A Rounder for chaining or passing to the NumberFormatter rounding() setter.
      * @draft ICU 60
      */
    Rounder withCurrency(const CurrencyUnit &currency) const;

  private:
    // Inherit constructor
    using Rounder::Rounder;

    // To allow parent class to call this class's constructor:
    friend class Rounder;
};

/**
 * A class that defines a rounding strategy parameterized by a rounding increment to be used when formatting numbers in
 * NumberFormatter.
 *
 * <p>
 * To create an IncrementRounder, use one of the factory methods on Rounder.
 *
 * @draft ICU 60
 */
class U_I18N_API IncrementRounder : public Rounder {
  public:
    /**
     * Specifies the minimum number of fraction digits to render after the decimal separator, padding with zeros if
     * necessary.  By default, no trailing zeros are added.
     *
     * <p>
     * For example, if the rounding increment is 0.5 and minFrac is 2, then the resulting strings include "0.00",
     * "0.50", "1.00", and "1.50".
     *
     * <p>
     * Note: In ICU4J, this functionality is accomplished via the scale of the BigDecimal rounding increment.
     *
     * @param minFrac The minimum number of digits after the decimal separator.
     * @return A Rounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     */
    Rounder withMinFraction(int32_t minFrac) const;

  private:
    // Inherit constructor
    using Rounder::Rounder;

    // To allow parent class to call this class's constructor:
    friend class Rounder;
};

/**
 * A class that defines the strategy for padding and truncating integers before the decimal separator.
 *
 * <p>
 * To create an IntegerWidth, use one of the factory methods.
 *
 * @draft ICU 60
 * @see NumberFormatter
 */
class U_I18N_API IntegerWidth : public UMemory {
  public:
    /**
     * Pad numbers at the beginning with zeros to guarantee a certain number of numerals before the decimal separator.
     *
     * <p>
     * For example, with minInt=3, the number 55 will get printed as "055".
     *
     * @param minInt
     *            The minimum number of places before the decimal separator.
     * @return An IntegerWidth for chaining or passing to the NumberFormatter integerWidth() setter.
     * @draft ICU 60
     * @see NumberFormatter
     */
    static IntegerWidth zeroFillTo(int32_t minInt);

    /**
     * Truncate numbers exceeding a certain number of numerals before the decimal separator.
     *
     * For example, with maxInt=3, the number 1234 will get printed as "234".
     *
     * @param maxInt
     *            The maximum number of places before the decimal separator. maxInt == -1 means no
     *            truncation.
     * @return An IntegerWidth for passing to the NumberFormatter integerWidth() setter.
     * @draft ICU 60
     * @see NumberFormatter
     */
    IntegerWidth truncateAt(int32_t maxInt);

  private:
    union {
        struct {
            impl::digits_t fMinInt;
            impl::digits_t fMaxInt;
        } minMaxInt;
        UErrorCode errorCode;
    } fUnion;
    bool fHasError = false;

    IntegerWidth(impl::digits_t minInt, impl::digits_t maxInt);

    IntegerWidth(UErrorCode errorCode) { // NOLINT
        fUnion.errorCode = errorCode;
        fHasError = true;
    }

    IntegerWidth() { // NOLINT
        fUnion.minMaxInt.fMinInt = -1;
    }

    /** Returns the default instance. */
    static IntegerWidth standard() {
        return IntegerWidth::zeroFillTo(1);
    }

    bool isBogus() const {
        return !fHasError && fUnion.minMaxInt.fMinInt == -1;
    }

    UBool copyErrorTo(UErrorCode &status) const {
        if (fHasError) {
            status = fUnion.errorCode;
            return TRUE;
        }
        return FALSE;
    }

    void apply(impl::DecimalQuantity &quantity, UErrorCode &status) const;

    bool operator==(const IntegerWidth& other) const;

    // To allow MacroProps/MicroProps to initialize empty instances:
    friend struct impl::MacroProps;
    friend struct impl::MicroProps;

    // To allow NumberFormatterImpl to access isBogus() and perform other operations:
    friend class impl::NumberFormatterImpl;

    // To allow access to the skeleton generation code:
    friend class impl::GeneratorHelpers;
};

namespace impl {

// Do not enclose entire SymbolsWrapper with #ifndef U_HIDE_INTERNAL_API, needed for a protected field
/** @internal */
class U_I18N_API SymbolsWrapper : public UMemory {
  public:
    /** @internal */
    SymbolsWrapper() : fType(SYMPTR_NONE), fPtr{nullptr} {}

    /** @internal */
    SymbolsWrapper(const SymbolsWrapper &other);

    /** @internal */
    SymbolsWrapper(SymbolsWrapper&& src) U_NOEXCEPT;

    /** @internal */
    ~SymbolsWrapper();

    /** @internal */
    SymbolsWrapper &operator=(const SymbolsWrapper &other);

    /** @internal */
    SymbolsWrapper &operator=(SymbolsWrapper&& src) U_NOEXCEPT;

#ifndef U_HIDE_INTERNAL_API

    /**
     * The provided object is copied, but we do not adopt it.
     * @internal
     */
    void setTo(const DecimalFormatSymbols &dfs);

    /**
     * Adopt the provided object.
     * @internal
     */
    void setTo(const NumberingSystem *ns);

    /**
     * Whether the object is currently holding a DecimalFormatSymbols.
     * @internal
     */
    bool isDecimalFormatSymbols() const;

    /**
     * Whether the object is currently holding a NumberingSystem.
     * @internal
     */
    bool isNumberingSystem() const;

    /**
     * Get the DecimalFormatSymbols pointer. No ownership change.
     * @internal
     */
    const DecimalFormatSymbols *getDecimalFormatSymbols() const;

    /**
     * Get the NumberingSystem pointer. No ownership change.
     * @internal
     */
    const NumberingSystem *getNumberingSystem() const;

    /** @internal */
    UBool copyErrorTo(UErrorCode &status) const {
        if (fType == SYMPTR_DFS && fPtr.dfs == nullptr) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return TRUE;
        } else if (fType == SYMPTR_NS && fPtr.ns == nullptr) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return TRUE;
        }
        return FALSE;
    }

#endif  // U_HIDE_INTERNAL_API

  private:
    enum SymbolsPointerType {
        SYMPTR_NONE, SYMPTR_DFS, SYMPTR_NS
    } fType;

    union {
        const DecimalFormatSymbols *dfs;
        const NumberingSystem *ns;
    } fPtr;

    void doCopyFrom(const SymbolsWrapper &other);

    void doMoveFrom(SymbolsWrapper&& src);

    void doCleanup();
};

// Do not enclose entire Grouper with #ifndef U_HIDE_INTERNAL_API, needed for a protected field
/** @internal */
class U_I18N_API Grouper : public UMemory {
  public:
#ifndef U_HIDE_INTERNAL_API
    /** @internal */
    static Grouper forStrategy(UGroupingStrategy grouping);

    /**
     * Resolve the values in Properties to a Grouper object.
     * @internal
     */
    static Grouper forProperties(const DecimalFormatProperties& properties);

    // Future: static Grouper forProperties(DecimalFormatProperties& properties);

    /** @internal */
    Grouper(int16_t grouping1, int16_t grouping2, int16_t minGrouping, UGroupingStrategy strategy)
            : fGrouping1(grouping1),
              fGrouping2(grouping2),
              fMinGrouping(minGrouping),
              fStrategy(strategy) {}
#endif  // U_HIDE_INTERNAL_API

    /** @internal */
    int16_t getPrimary() const;

    /** @internal */
    int16_t getSecondary() const;

  private:
    /**
     * The grouping sizes, with the following special values:
     * <ul>
     * <li>-1 = no grouping
     * <li>-2 = needs locale data
     * <li>-4 = fall back to Western grouping if not in locale
     * </ul>
     */
    int16_t fGrouping1;
    int16_t fGrouping2;

    /**
     * The minimum grouping size, with the following special values:
     * <ul>
     * <li>-2 = needs locale data
     * <li>-3 = no less than 2
     * </ul>
     */
    int16_t fMinGrouping;

    /**
     * The UGroupingStrategy that was used to create this Grouper, or UNUM_GROUPING_COUNT if this
     * was not created from a UGroupingStrategy.
     */
    UGroupingStrategy fStrategy;

    Grouper() : fGrouping1(-3) {};

    bool isBogus() const {
        return fGrouping1 == -3;
    }

    /** NON-CONST: mutates the current instance. */
    void setLocaleData(const impl::ParsedPatternInfo &patternInfo, const Locale& locale);

    bool groupAtPosition(int32_t position, const impl::DecimalQuantity &value) const;

    // To allow MacroProps/MicroProps to initialize empty instances:
    friend struct MacroProps;
    friend struct MicroProps;

    // To allow NumberFormatterImpl to access isBogus() and perform other operations:
    friend class NumberFormatterImpl;

    // To allow NumberParserImpl to perform setLocaleData():
    friend class ::icu::numparse::impl::NumberParserImpl;

    // To allow access to the skeleton generation code:
    friend class impl::GeneratorHelpers;
};

// Do not enclose entire Padder with #ifndef U_HIDE_INTERNAL_API, needed for a protected field
/** @internal */
class U_I18N_API Padder : public UMemory {
  public:
#ifndef U_HIDE_INTERNAL_API
    /** @internal */
    static Padder none();

    /** @internal */
    static Padder codePoints(UChar32 cp, int32_t targetWidth, UNumberFormatPadPosition position);
#endif  // U_HIDE_INTERNAL_API

    /** @internal */
    static Padder forProperties(const DecimalFormatProperties& properties);

  private:
    UChar32 fWidth;  // -3 = error; -2 = bogus; -1 = no padding
    union {
        struct {
            int32_t fCp;
            UNumberFormatPadPosition fPosition;
        } padding;
        UErrorCode errorCode;
    } fUnion;

    Padder(UChar32 cp, int32_t width, UNumberFormatPadPosition position);

    Padder(int32_t width);

    Padder(UErrorCode errorCode) : fWidth(-3) { // NOLINT
        fUnion.errorCode = errorCode;
    }

    Padder() : fWidth(-2) {} // NOLINT

    bool isBogus() const {
        return fWidth == -2;
    }

    UBool copyErrorTo(UErrorCode &status) const {
        if (fWidth == -3) {
            status = fUnion.errorCode;
            return TRUE;
        }
        return FALSE;
    }

    bool isValid() const {
        return fWidth > 0;
    }

    int32_t padAndApply(const impl::Modifier &mod1, const impl::Modifier &mod2,
                        impl::NumberStringBuilder &string, int32_t leftIndex, int32_t rightIndex,
                        UErrorCode &status) const;

    // To allow MacroProps/MicroProps to initialize empty instances:
    friend struct MacroProps;
    friend struct MicroProps;

    // To allow NumberFormatterImpl to access isBogus() and perform other operations:
    friend class impl::NumberFormatterImpl;

    // To allow access to the skeleton generation code:
    friend class impl::GeneratorHelpers;
};

// Do not enclose entire MacroProps with #ifndef U_HIDE_INTERNAL_API, needed for a protected field
/** @internal */
class U_I18N_API Multiplier : public UMemory {
  public:
    /** @internal */
    static Multiplier magnitude(int32_t magnitudeMultiplier);

    /** @internal */
    static Multiplier integer(int32_t multiplier);

  private:
    int32_t magnitudeMultiplier;
    int32_t multiplier;

    Multiplier(int32_t magnitudeMultiplier, int32_t multiplier);

    Multiplier() : magnitudeMultiplier(0), multiplier(1) {}

    bool isValid() const {
        return magnitudeMultiplier != 0 || multiplier != 1;
    }

    // To allow MacroProps/MicroProps to initialize empty instances:
    friend struct MacroProps;
    friend struct MicroProps;

    // To allow NumberFormatterImpl to access isBogus() and perform other operations:
    friend class impl::NumberFormatterImpl;

    // To allow the helper class MultiplierChain access to private fields:
    friend class impl::MultiplierChain;

    // To allow access to the skeleton generation code:
    friend class impl::GeneratorHelpers;
};

/** @internal */
struct U_I18N_API MacroProps : public UMemory {
    /** @internal */
    Notation notation;

    /** @internal */
    MeasureUnit unit; // = NoUnit::base();

    /** @internal */
    MeasureUnit perUnit; // = NoUnit::base();

    /** @internal */
    Rounder rounder;  // = Rounder();  (bogus)

    /** @internal */
    Grouper grouper;  // = Grouper();  (bogus)

    /** @internal */
    Padder padder;    // = Padder();   (bogus)

    /** @internal */
    IntegerWidth integerWidth; // = IntegerWidth(); (bogus)

    /** @internal */
    SymbolsWrapper symbols;

    // UNUM_XYZ_COUNT denotes null (bogus) values.

    /** @internal */
    UNumberUnitWidth unitWidth = UNUM_UNIT_WIDTH_COUNT;

    /** @internal */
    UNumberSignDisplay sign = UNUM_SIGN_COUNT;

    /** @internal */
    UNumberDecimalSeparatorDisplay decimal = UNUM_DECIMAL_SEPARATOR_COUNT;

    Multiplier multiplier;  // = Multiplier();  (bogus)

    AffixPatternProvider* affixProvider = nullptr;  // no ownership

    /** @internal */
    PluralRules *rules = nullptr;  // no ownership

    /** @internal */
    CurrencySymbols *currencySymbols = nullptr;  // no ownership

    /** @internal */
    int32_t threshold = DEFAULT_THRESHOLD;

    /** @internal */
    Locale locale;

    // NOTE: Uses default copy and move constructors.

    /**
     * Check all members for errors.
     * @internal
     */
    bool copyErrorTo(UErrorCode &status) const {
        return notation.copyErrorTo(status) || rounder.copyErrorTo(status) ||
               padder.copyErrorTo(status) || integerWidth.copyErrorTo(status) ||
               symbols.copyErrorTo(status);
    }
};

} // namespace impl

/**
 * An abstract base class for specifying settings related to number formatting. This class is implemented by
 * {@link UnlocalizedNumberFormatter} and {@link LocalizedNumberFormatter}.
 */
template<typename Derived>
class U_I18N_API NumberFormatterSettings {
  public:
    /**
     * Specifies the notation style (simple, scientific, or compact) for rendering numbers.
     *
     * <ul>
     * <li>Simple notation: "12,300"
     * <li>Scientific notation: "1.23E4"
     * <li>Compact notation: "12K"
     * </ul>
     *
     * <p>
     * All notation styles will be properly localized with locale data, and all notation styles are compatible with
     * units, rounding strategies, and other number formatter settings.
     *
     * <p>
     * Pass this method the return value of a {@link Notation} factory method. For example:
     *
     * <pre>
     * NumberFormatter::with().notation(Notation::compactShort())
     * </pre>
     *
     * The default is to use simple notation.
     *
     * @param notation
     *            The notation strategy to use.
     * @return The fluent chain.
     * @see Notation
     * @draft ICU 60
     */
    Derived notation(const Notation &notation) const &;

    /**
     * Overload of notation() for use on an rvalue reference.
     *
     * @param notation
     *            The notation strategy to use.
     * @return The fluent chain.
     * @see #notation
     * @draft ICU 62
     */
    Derived notation(const Notation &notation) &&;

    /**
     * Specifies the unit (unit of measure, currency, or percent) to associate with rendered numbers.
     *
     * <ul>
     * <li>Unit of measure: "12.3 meters"
     * <li>Currency: "$12.30"
     * <li>Percent: "12.3%"
     * </ul>
     *
     * All units will be properly localized with locale data, and all units are compatible with notation styles,
     * rounding strategies, and other number formatter settings.
     *
     * Pass this method any instance of {@link MeasureUnit}. For units of measure (which often involve the
     * factory methods that return a pointer):
     *
     * <pre>
     * NumberFormatter::with().adoptUnit(MeasureUnit::createMeter(status))
     * </pre>
     *
     * Currency:
     *
     * <pre>
     * NumberFormatter::with().unit(CurrencyUnit(u"USD", status))
     * </pre>
     *
     * Percent:
     *
     * <pre>
     * NumberFormatter::with().unit(NoUnit.percent())
     * </pre>
     *
     * See {@link #perUnit} for information on how to format strings like "5 meters per second".
     *
     * The default is to render without units (equivalent to NoUnit.base()).
     *
     * @param unit
     *            The unit to render.
     * @return The fluent chain.
     * @see MeasureUnit
     * @see Currency
     * @see NoUnit
     * @see #perUnit
     * @draft ICU 60
     */
    Derived unit(const icu::MeasureUnit &unit) const &;

    /**
     * Overload of unit() for use on an rvalue reference.
     *
     * @param unit
     *            The unit to render.
     * @return The fluent chain.
     * @see #unit
     * @draft ICU 62
     */
    Derived unit(const icu::MeasureUnit &unit) &&;

    /**
     * Like unit(), but takes ownership of a pointer.  Convenient for use with the MeasureFormat factory
     * methods, which return pointers that need ownership.  Example:
     *
     * <pre>
     * NumberFormatter::with().adoptUnit(MeasureUnit::createMeter(status))
     * </pre>
     *
     * @param unit
     *            The unit to render.
     * @return The fluent chain.
     * @see #unit
     * @see MeasureUnit
     * @draft ICU 60
     */
    Derived adoptUnit(icu::MeasureUnit *unit) const &;

    /**
     * Overload of adoptUnit() for use on an rvalue reference.
     *
     * @param unit
     *            The unit to render.
     * @return The fluent chain.
     * @see #adoptUnit
     * @draft ICU 62
     */
    Derived adoptUnit(icu::MeasureUnit *unit) &&;

    /**
     * Sets a unit to be used in the denominator. For example, to format "3 m/s", pass METER to the unit and SECOND to
     * the perUnit.
     *
     * Pass this method any instance of {@link MeasureUnit}.  Since MeasureUnit factory methods return pointers, the
     * {@link #adoptPerUnit} version of this method is often more useful.
     *
     * The default is not to display any unit in the denominator.
     *
     * If a per-unit is specified without a primary unit via {@link #unit}, the behavior is undefined.
     *
     * @param perUnit
     *            The unit to render in the denominator.
     * @return The fluent chain
     * @see #unit
     * @draft ICU 61
     */
    Derived perUnit(const icu::MeasureUnit &perUnit) const &;

    /**
     * Overload of perUnit() for use on an rvalue reference.
     *
     * @param perUnit
     *            The unit to render in the denominator.
     * @return The fluent chain.
     * @see #perUnit
     * @draft ICU 62
     */
    Derived perUnit(const icu::MeasureUnit &perUnit) &&;

    /**
     * Like perUnit(), but takes ownership of a pointer.  Convenient for use with the MeasureFormat factory
     * methods, which return pointers that need ownership.  Example:
     *
     * <pre>
     * NumberFormatter::with()
     *      .adoptUnit(MeasureUnit::createMeter(status))
     *      .adoptPerUnit(MeasureUnit::createSecond(status))
     * </pre>
     *
     * @param perUnit
     *            The unit to render in the denominator.
     * @return The fluent chain.
     * @see #perUnit
     * @see MeasureUnit
     * @draft ICU 61
     */
    Derived adoptPerUnit(icu::MeasureUnit *perUnit) const &;

    /**
     * Overload of adoptPerUnit() for use on an rvalue reference.
     *
     * @param perUnit
     *            The unit to render in the denominator.
     * @return The fluent chain.
     * @see #adoptPerUnit
     * @draft ICU 62
     */
    Derived adoptPerUnit(icu::MeasureUnit *perUnit) &&;

    /**
     * Specifies the rounding strategy to use when formatting numbers.
     *
     * <ul>
     * <li>Round to 3 decimal places: "3.142"
     * <li>Round to 3 significant figures: "3.14"
     * <li>Round to the closest nickel: "3.15"
     * <li>Do not perform rounding: "3.1415926..."
     * </ul>
     *
     * <p>
     * Pass this method the return value of one of the factory methods on {@link Rounder}. For example:
     *
     * <pre>
     * NumberFormatter::with().rounding(Rounder::fixedFraction(2))
     * </pre>
     *
     * <p>
     * In most cases, the default rounding strategy is to round to 6 fraction places; i.e.,
     * <code>Rounder.maxFraction(6)</code>. The exceptions are if compact notation is being used, then the compact
     * notation rounding strategy is used (see {@link Notation#compactShort} for details), or if the unit is a currency,
     * then standard currency rounding is used, which varies from currency to currency (see {@link Rounder#currency} for
     * details).
     *
     * @param rounder
     *            The rounding strategy to use.
     * @return The fluent chain.
     * @see Rounder
     * @draft ICU 60
     */
    Derived rounding(const Rounder &rounder) const &;

    /**
     * Overload of rounding() for use on an rvalue reference.
     *
     * @param rounder
     *            The rounding strategy to use.
     * @return The fluent chain.
     * @see #rounding
     * @draft ICU 62
     */
    Derived rounding(const Rounder& rounder) &&;

    /**
     * Specifies the grouping strategy to use when formatting numbers.
     *
     * <ul>
     * <li>Default grouping: "12,300" and "1,230"
     * <li>Grouping with at least 2 digits: "12,300" and "1230"
     * <li>No grouping: "12300" and "1230"
     * </ul>
     *
     * <p>
     * The exact grouping widths will be chosen based on the locale.
     *
     * <p>
     * Pass this method an element from the {@link UGroupingStrategy} enum. For example:
     *
     * <pre>
     * NumberFormatter::with().grouping(UNUM_GROUPING_MIN2)
     * </pre>
     *
     * The default is to perform grouping according to locale data; most locales, but not all locales,
     * enable it by default.
     *
     * @param strategy
     *            The grouping strategy to use.
     * @return The fluent chain.
     * @draft ICU 61
     */
    Derived grouping(const UGroupingStrategy &strategy) const &;

    /**
     * Overload of grouping() for use on an rvalue reference.
     *
     * @param rounder
     *            The grouping strategy to use.
     * @return The fluent chain.
     * @see #grouping
     * @provisional This API might change or be removed in a future release.
     * @draft ICU 62
     */
    Derived grouping(const UGroupingStrategy& rounder) &&;

    /**
     * Specifies the minimum and maximum number of digits to render before the decimal mark.
     *
     * <ul>
     * <li>Zero minimum integer digits: ".08"
     * <li>One minimum integer digit: "0.08"
     * <li>Two minimum integer digits: "00.08"
     * </ul>
     *
     * <p>
     * Pass this method the return value of {@link IntegerWidth#zeroFillTo(int)}. For example:
     *
     * <pre>
     * NumberFormatter::with().integerWidth(IntegerWidth::zeroFillTo(2))
     * </pre>
     *
     * The default is to have one minimum integer digit.
     *
     * @param style
     *            The integer width to use.
     * @return The fluent chain.
     * @see IntegerWidth
     * @draft ICU 60
     */
    Derived integerWidth(const IntegerWidth &style) const &;

    /**
     * Overload of integerWidth() for use on an rvalue reference.
     *
     * @param style
     *            The integer width to use.
     * @return The fluent chain.
     * @see #integerWidth
     * @draft ICU 62
     */
    Derived integerWidth(const IntegerWidth &style) &&;

    /**
     * Specifies the symbols (decimal separator, grouping separator, percent sign, numerals, etc.) to use when rendering
     * numbers.
     *
     * <ul>
     * <li><em>en_US</em> symbols: "12,345.67"
     * <li><em>fr_FR</em> symbols: "12&nbsp;345,67"
     * <li><em>de_CH</em> symbols: "12’345.67"
     * <li><em>my_MY</em> symbols: "၁၂,၃၄၅.၆၇"
     * </ul>
     *
     * <p>
     * Pass this method an instance of {@link DecimalFormatSymbols}. For example:
     *
     * <pre>
     * NumberFormatter::with().symbols(DecimalFormatSymbols(Locale("de_CH"), status))
     * </pre>
     *
     * <p>
     * <strong>Note:</strong> DecimalFormatSymbols automatically chooses the best numbering system based on the locale.
     * In the examples above, the first three are using the Latin numbering system, and the fourth is using the Myanmar
     * numbering system.
     *
     * <p>
     * <strong>Note:</strong> The instance of DecimalFormatSymbols will be copied: changes made to the symbols object
     * after passing it into the fluent chain will not be seen.
     *
     * <p>
     * <strong>Note:</strong> Calling this method will override the NumberingSystem previously specified in
     * {@link #symbols(NumberingSystem)}.
     *
     * <p>
     * The default is to choose the symbols based on the locale specified in the fluent chain.
     *
     * @param symbols
     *            The DecimalFormatSymbols to use.
     * @return The fluent chain.
     * @see DecimalFormatSymbols
     * @draft ICU 60
     */
    Derived symbols(const DecimalFormatSymbols &symbols) const &;

    /**
     * Overload of symbols() for use on an rvalue reference.
     *
     * @param symbols
     *            The DecimalFormatSymbols to use.
     * @return The fluent chain.
     * @see #symbols
     * @draft ICU 62
     */
    Derived symbols(const DecimalFormatSymbols &symbols) &&;

    /**
     * Specifies that the given numbering system should be used when fetching symbols.
     *
     * <ul>
     * <li>Latin numbering system: "12,345"
     * <li>Myanmar numbering system: "၁၂,၃၄၅"
     * <li>Math Sans Bold numbering system: "𝟭𝟮,𝟯𝟰𝟱"
     * </ul>
     *
     * <p>
     * Pass this method an instance of {@link NumberingSystem}. For example, to force the locale to always use the Latin
     * alphabet numbering system (ASCII digits):
     *
     * <pre>
     * NumberFormatter::with().adoptSymbols(NumberingSystem::createInstanceByName("latn", status))
     * </pre>
     *
     * <p>
     * <strong>Note:</strong> Calling this method will override the DecimalFormatSymbols previously specified in
     * {@link #symbols(DecimalFormatSymbols)}.
     *
     * <p>
     * The default is to choose the best numbering system for the locale.
     *
     * <p>
     * This method takes ownership of a pointer in order to work nicely with the NumberingSystem factory methods.
     *
     * @param symbols
     *            The NumberingSystem to use.
     * @return The fluent chain.
     * @see NumberingSystem
     * @draft ICU 60
     */
    Derived adoptSymbols(NumberingSystem *symbols) const &;

    /**
     * Overload of adoptSymbols() for use on an rvalue reference.
     *
     * @param symbols
     *            The NumberingSystem to use.
     * @return The fluent chain.
     * @see #adoptSymbols
     * @draft ICU 62
     */
    Derived adoptSymbols(NumberingSystem *symbols) &&;

    /**
     * Sets the width of the unit (measure unit or currency).  Most common values:
     *
     * <ul>
     * <li>Short: "$12.00", "12 m"
     * <li>ISO Code: "USD 12.00"
     * <li>Full name: "12.00 US dollars", "12 meters"
     * </ul>
     *
     * <p>
     * Pass an element from the {@link UNumberUnitWidth} enum to this setter. For example:
     *
     * <pre>
     * NumberFormatter::with().unitWidth(UNumberUnitWidth::UNUM_UNIT_WIDTH_FULL_NAME)
     * </pre>
     *
     * <p>
     * The default is the SHORT width.
     *
     * @param width
     *            The width to use when rendering numbers.
     * @return The fluent chain
     * @see UNumberUnitWidth
     * @draft ICU 60
     */
    Derived unitWidth(const UNumberUnitWidth &width) const &;

    /**
     * Overload of unitWidth() for use on an rvalue reference.
     *
     * @param width
     *            The width to use when rendering numbers.
     * @return The fluent chain.
     * @see #unitWidth
     * @draft ICU 62
     */
    Derived unitWidth(const UNumberUnitWidth &width) &&;

    /**
     * Sets the plus/minus sign display strategy. Most common values:
     *
     * <ul>
     * <li>Auto: "123", "-123"
     * <li>Always: "+123", "-123"
     * <li>Accounting: "$123", "($123)"
     * </ul>
     *
     * <p>
     * Pass an element from the {@link UNumberSignDisplay} enum to this setter. For example:
     *
     * <pre>
     * NumberFormatter::with().sign(UNumberSignDisplay::UNUM_SIGN_ALWAYS)
     * </pre>
     *
     * <p>
     * The default is AUTO sign display.
     *
     * @param style
     *            The sign display strategy to use when rendering numbers.
     * @return The fluent chain
     * @see UNumberSignDisplay
     * @provisional This API might change or be removed in a future release.
     * @draft ICU 60
     */
    Derived sign(const UNumberSignDisplay &style) const &;

    /**
     * Overload of sign() for use on an rvalue reference.
     *
     * @param style
     *            The sign display strategy to use when rendering numbers.
     * @return The fluent chain.
     * @see #sign
     * @draft ICU 62
     */
    Derived sign(const UNumberSignDisplay &style) &&;

    /**
     * Sets the decimal separator display strategy. This affects integer numbers with no fraction part. Most common
     * values:
     *
     * <ul>
     * <li>Auto: "1"
     * <li>Always: "1."
     * </ul>
     *
     * <p>
     * Pass an element from the {@link UNumberDecimalSeparatorDisplay} enum to this setter. For example:
     *
     * <pre>
     * NumberFormatter::with().decimal(UNumberDecimalSeparatorDisplay::UNUM_DECIMAL_SEPARATOR_ALWAYS)
     * </pre>
     *
     * <p>
     * The default is AUTO decimal separator display.
     *
     * @param style
     *            The decimal separator display strategy to use when rendering numbers.
     * @return The fluent chain
     * @see UNumberDecimalSeparatorDisplay
     * @provisional This API might change or be removed in a future release.
     * @draft ICU 60
     */
    Derived decimal(const UNumberDecimalSeparatorDisplay &style) const &;

    /**
     * Overload of decimal() for use on an rvalue reference.
     *
     * @param style
     *            The decimal separator display strategy to use when rendering numbers.
     * @return The fluent chain.
     * @see #sign
     * @draft ICU 62
     */
    Derived decimal(const UNumberDecimalSeparatorDisplay &style) &&;

#ifndef U_HIDE_INTERNAL_API

    /**
     * Set the padding strategy. May be added in the future; see #13338.
     *
     * @internal ICU 60: This API is ICU internal only.
     */
    Derived padding(const impl::Padder &padder) const &;

    /** @internal */
    Derived padding(const impl::Padder &padder) &&;

    /**
     * Internal fluent setter to support a custom regulation threshold. A threshold of 1 causes the data structures to
     * be built right away. A threshold of 0 prevents the data structures from being built.
     *
     * @internal ICU 60: This API is ICU internal only.
     */
    Derived threshold(int32_t threshold) const &;

    /** @internal */
    Derived threshold(int32_t threshold) &&;

    /**
     * Internal fluent setter to overwrite the entire macros object.
     *
     * @internal ICU 60: This API is ICU internal only.
     */
    Derived macros(const impl::MacroProps& macros) const &;

    /** @internal */
    Derived macros(const impl::MacroProps& macros) &&;

    /** @internal */
    Derived macros(impl::MacroProps&& macros) const &;

    /** @internal */
    Derived macros(impl::MacroProps&& macros) &&;

#endif  /* U_HIDE_INTERNAL_API */

    /**
     * Creates a skeleton string representation of this number formatter. A skeleton string is a
     * locale-agnostic serialized form of a number formatter.
     * <p>
     * Not all options are capable of being represented in the skeleton string; for example, a
     * DecimalFormatSymbols object. If any such option is encountered, an
     * {@link UnsupportedOperationException} is thrown.
     * <p>
     * The returned skeleton is in normalized form, such that two number formatters with equivalent
     * behavior should produce the same skeleton.
     * <p>
     * Sets an error code if the number formatter has an option that cannot be represented in a skeleton
     * string.
     *
     * @return A number skeleton string with behavior corresponding to this number formatter.
     * @draft ICU 62
     */
    UnicodeString toSkeleton(UErrorCode& status) const;

    /**
     * Sets the UErrorCode if an error occurred in the fluent chain.
     * Preserves older error codes in the outErrorCode.
     * @return TRUE if U_FAILURE(outErrorCode)
     * @draft ICU 60
     */
    UBool copyErrorTo(UErrorCode &outErrorCode) const {
        if (U_FAILURE(outErrorCode)) {
            // Do not overwrite the older error code
            return TRUE;
        }
        fMacros.copyErrorTo(outErrorCode);
        return U_FAILURE(outErrorCode);
    };

    // NOTE: Uses default copy and move constructors.

  protected:
    impl::MacroProps fMacros;

  private:
    // Don't construct me directly!  Use (Un)LocalizedNumberFormatter.
    NumberFormatterSettings() = default;

    friend class LocalizedNumberFormatter;
    friend class UnlocalizedNumberFormatter;
};

/**
 * A NumberFormatter that does not yet have a locale. In order to format numbers, a locale must be specified.
 *
 * @see NumberFormatter
 * @draft ICU 60
 */
class U_I18N_API UnlocalizedNumberFormatter
        : public NumberFormatterSettings<UnlocalizedNumberFormatter>, public UMemory {

  public:
    /**
     * Associate the given locale with the number formatter. The locale is used for picking the appropriate symbols,
     * formats, and other data for number display.
     *
     * <p>
     * To use the Java default locale, call Locale::getDefault():
     *
     * <pre>
     * NumberFormatter::with(). ... .locale(Locale::getDefault())
     * </pre>
     *
     * @param locale
     *            The locale to use when loading data for number formatting.
     * @return The fluent chain.
     * @draft ICU 60
     */
    LocalizedNumberFormatter locale(const icu::Locale &locale) const &;

    /**
     * Overload of locale() for use on an rvalue reference.
     *
     * @param locale
     *            The locale to use when loading data for number formatting.
     * @return The fluent chain.
     * @see #locale
     * @draft ICU 62
     */
    LocalizedNumberFormatter locale(const icu::Locale &locale) &&;

    /**
     * Default constructor: puts the formatter into a valid but undefined state.
     *
     * @draft ICU 62
     */
    UnlocalizedNumberFormatter() = default;

    // Make default copy constructor call the NumberFormatterSettings copy constructor.
    /**
     * Returns a copy of this UnlocalizedNumberFormatter.
     * @draft ICU 60
     */
    UnlocalizedNumberFormatter(const UnlocalizedNumberFormatter &other);

    /**
     * Move constructor:
     * The source UnlocalizedNumberFormatter will be left in a valid but undefined state.
     * @draft ICU 62
     */
    UnlocalizedNumberFormatter(UnlocalizedNumberFormatter&& src) U_NOEXCEPT;

    /**
     * Copy assignment operator.
     * @draft ICU 62
     */
    UnlocalizedNumberFormatter& operator=(const UnlocalizedNumberFormatter& other);

    /**
     * Move assignment operator:
     * The source UnlocalizedNumberFormatter will be left in a valid but undefined state.
     * @draft ICU 62
     */
    UnlocalizedNumberFormatter& operator=(UnlocalizedNumberFormatter&& src) U_NOEXCEPT;

  private:
    explicit UnlocalizedNumberFormatter(const NumberFormatterSettings<UnlocalizedNumberFormatter>& other);

    explicit UnlocalizedNumberFormatter(
            NumberFormatterSettings<UnlocalizedNumberFormatter>&& src) U_NOEXCEPT;

    // To give the fluent setters access to this class's constructor:
    friend class NumberFormatterSettings<UnlocalizedNumberFormatter>;

    // To give NumberFormatter::with() access to this class's constructor:
    friend class NumberFormatter;
};

/**
 * A NumberFormatter that has a locale associated with it; this means .format() methods are available.
 *
 * @see NumberFormatter
 * @draft ICU 60
 */
class U_I18N_API LocalizedNumberFormatter
        : public NumberFormatterSettings<LocalizedNumberFormatter>, public UMemory {
  public:
    /**
     * Format the given integer number to a string using the settings specified in the NumberFormatter fluent
     * setting chain.
     *
     * @param value
     *            The number to format.
     * @param status
     *            Set to an ErrorCode if one occurred in the setter chain or during formatting.
     * @return A FormattedNumber object; call .toString() to get the string.
     * @draft ICU 60
     */
    FormattedNumber formatInt(int64_t value, UErrorCode &status) const;

    /**
     * Format the given float or double to a string using the settings specified in the NumberFormatter fluent setting
     * chain.
     *
     * @param value
     *            The number to format.
     * @param status
     *            Set to an ErrorCode if one occurred in the setter chain or during formatting.
     * @return A FormattedNumber object; call .toString() to get the string.
     * @draft ICU 60
     */
    FormattedNumber formatDouble(double value, UErrorCode &status) const;

    /**
     * Format the given decimal number to a string using the settings
     * specified in the NumberFormatter fluent setting chain.
     * The syntax of the unformatted number is a "numeric string"
     * as defined in the Decimal Arithmetic Specification, available at
     * http://speleotrove.com/decimal
     *
     * @param value
     *            The number to format.
     * @param status
     *            Set to an ErrorCode if one occurred in the setter chain or during formatting.
     * @return A FormattedNumber object; call .toString() to get the string.
     * @draft ICU 60
     */
    FormattedNumber formatDecimal(StringPiece value, UErrorCode& status) const;

#ifndef U_HIDE_INTERNAL_API

    /** Internal method.
     * @internal
     */
    FormattedNumber formatDecimalQuantity(const impl::DecimalQuantity& dq, UErrorCode& status) const;

    /**
     * Internal method for testing.
     * @internal
     */
    const impl::NumberFormatterImpl* getCompiled() const;

    /**
     * Internal method for testing.
     * @internal
     */
    int32_t getCallCount() const;

#endif

    /**
     * Default constructor: puts the formatter into a valid but undefined state.
     *
     * @draft ICU 62
     */
    LocalizedNumberFormatter() = default;

    // Make default copy constructor call the NumberFormatterSettings copy constructor.
    /**
     * Returns a copy of this LocalizedNumberFormatter.
     * @draft ICU 60
     */
    LocalizedNumberFormatter(const LocalizedNumberFormatter &other);

    /**
     * Move constructor:
     * The source LocalizedNumberFormatter will be left in a valid but undefined state.
     * @draft ICU 62
     */
    LocalizedNumberFormatter(LocalizedNumberFormatter&& src) U_NOEXCEPT;

    /**
     * Copy assignment operator.
     * @draft ICU 62
     */
    LocalizedNumberFormatter& operator=(const LocalizedNumberFormatter& other);

    /**
     * Move assignment operator:
     * The source LocalizedNumberFormatter will be left in a valid but undefined state.
     * @draft ICU 62
     */
    LocalizedNumberFormatter& operator=(LocalizedNumberFormatter&& src) U_NOEXCEPT;

#ifndef U_HIDE_INTERNAL_API

    /**
     * This is the core entrypoint to the number formatting pipeline. It performs self-regulation: a static code path
     * for the first few calls, and compiling a more efficient data structure if called repeatedly.
     *
     * <p>
     * This function is very hot, being called in every call to the number formatting pipeline.
     *
     * @param results
     *            The results object. This method will mutate it to save the results.
     */
    void formatImpl(impl::UFormattedNumberData *results, UErrorCode &status) const;

#endif

    /**
     * Destruct this LocalizedNumberFormatter, cleaning up any memory it might own.
     * @draft ICU 60
     */
    ~LocalizedNumberFormatter();

  private:
    // Note: fCompiled can't be a LocalPointer because impl::NumberFormatterImpl is defined in an internal
    // header, and LocalPointer needs the full class definition in order to delete the instance.
    const impl::NumberFormatterImpl* fCompiled {nullptr};
    char fUnsafeCallCount[8] {};  // internally cast to u_atomic_int32_t

    explicit LocalizedNumberFormatter(const NumberFormatterSettings<LocalizedNumberFormatter>& other);

    explicit LocalizedNumberFormatter(NumberFormatterSettings<LocalizedNumberFormatter>&& src) U_NOEXCEPT;

    LocalizedNumberFormatter(const impl::MacroProps &macros, const Locale &locale);

    LocalizedNumberFormatter(impl::MacroProps &&macros, const Locale &locale);

    // To give the fluent setters access to this class's constructor:
    friend class NumberFormatterSettings<UnlocalizedNumberFormatter>;
    friend class NumberFormatterSettings<LocalizedNumberFormatter>;

    // To give UnlocalizedNumberFormatter::locale() access to this class's constructor:
    friend class UnlocalizedNumberFormatter;
};

/**
 * The result of a number formatting operation. This class allows the result to be exported in several data types,
 * including a UnicodeString and a FieldPositionIterator.
 *
 * @draft ICU 60
 */
class U_I18N_API FormattedNumber : public UMemory {
  public:
    /**
     * Returns a UnicodeString representation of the formatted number.
     *
     * @return a UnicodeString containing the localized number.
     * @draft ICU 60
     */
    UnicodeString toString() const;

    /**
     * Appends the formatted number to an Appendable.
     *
     * @param appendable
     *            The Appendable to which to append the formatted number string.
     * @return The same Appendable, for chaining.
     * @draft ICU 60
     * @see Appendable
     */
    Appendable &appendTo(Appendable &appendable);

    /**
     * Determine the start and end indices of the first occurrence of the given <em>field</em> in the output string.
     * This allows you to determine the locations of the integer part, fraction part, and sign.
     *
     * <p>
     * If multiple different field attributes are needed, this method can be called repeatedly, or if <em>all</em> field
     * attributes are needed, consider using populateFieldPositionIterator().
     *
     * <p>
     * If a field occurs multiple times in an output string, such as a grouping separator, this method will only ever
     * return the first occurrence. Use populateFieldPositionIterator() to access all occurrences of an attribute.
     *
     * @param fieldPosition
     *            The FieldPosition to populate with the start and end indices of the desired field.
     * @param status
     *            Set if an error occurs while populating the FieldPosition.
     * @draft ICU 60
     * @see UNumberFormatFields
     */
    void populateFieldPosition(FieldPosition &fieldPosition, UErrorCode &status);

    /**
     * Export the formatted number to a FieldPositionIterator. This allows you to determine which characters in
     * the output string correspond to which <em>fields</em>, such as the integer part, fraction part, and sign.
     *
     * <p>
     * If information on only one field is needed, consider using populateFieldPosition() instead.
     *
     * @param iterator
     *            The FieldPositionIterator to populate with all of the fields present in the formatted number.
     * @param status
     *            Set if an error occurs while populating the FieldPositionIterator.
     * @draft ICU 60
     * @see UNumberFormatFields
     */
    void populateFieldPositionIterator(FieldPositionIterator &iterator, UErrorCode &status);

#ifndef U_HIDE_INTERNAL_API

    /**
     *  Gets the raw DecimalQuantity for plural rule selection.
     *  @internal
     */
    void getDecimalQuantity(impl::DecimalQuantity& output, UErrorCode& status) const;

    /** @internal */
    const UnicodeString getPrefix(UErrorCode& status) const;

    /** @internal */
    const UnicodeString getSuffix(UErrorCode& status) const;

#endif

    // Don't allow copying of FormattedNumber, but moving is okay.
    FormattedNumber(const FormattedNumber&) = delete;
    FormattedNumber& operator=(const FormattedNumber&) = delete;

    /**
     * Move constructor:
     * Leaves the source FormattedNumber in an undefined state.
     * @draft ICU 62
     */
    FormattedNumber(FormattedNumber&& src) U_NOEXCEPT;

    /**
     * Move assignment:
     * Leaves the source FormattedNumber in an undefined state.
     * @draft ICU 62
     */
    FormattedNumber& operator=(FormattedNumber&& src) U_NOEXCEPT;

    /**
     * Destruct an instance of FormattedNumber, cleaning up any memory it might own.
     * @draft ICU 60
     */
    ~FormattedNumber();

  private:
    // Can't use LocalPointer because UFormattedNumberData is forward-declared
    const impl::UFormattedNumberData *fResults;

    // Error code for the terminal methods
    UErrorCode fErrorCode;

    /**
     * Internal constructor from data type. Adopts the data pointer.
     * @internal
     */
    explicit FormattedNumber(impl::UFormattedNumberData *results)
        : fResults(results), fErrorCode(U_ZERO_ERROR) {};

    explicit FormattedNumber(UErrorCode errorCode)
        : fResults(nullptr), fErrorCode(errorCode) {};

    // To give LocalizedNumberFormatter format methods access to this class's constructor:
    friend class LocalizedNumberFormatter;
};

/**
 * See the main description in numberformatter.h for documentation and examples.
 *
 * @draft ICU 60
 */
class U_I18N_API NumberFormatter final {
  public:
    /**
     * Call this method at the beginning of a NumberFormatter fluent chain in which the locale is not currently known at
     * the call site.
     *
     * @return An {@link UnlocalizedNumberFormatter}, to be used for chaining.
     * @draft ICU 60
     */
    static UnlocalizedNumberFormatter with();

    /**
     * Call this method at the beginning of a NumberFormatter fluent chain in which the locale is known at the call
     * site.
     *
     * @param locale
     *            The locale from which to load formats and symbols for number formatting.
     * @return A {@link LocalizedNumberFormatter}, to be used for chaining.
     * @draft ICU 60
     */
    static LocalizedNumberFormatter withLocale(const Locale &locale);

    /**
     * Call this method at the beginning of a NumberFormatter fluent chain to create an instance based
     * on a given number skeleton string.
     *
     * @param skeleton
     *            The skeleton string off of which to base this NumberFormatter.
     * @return An UnlocalizedNumberFormatter, to be used for chaining.
     * @throws SkeletonSyntaxException If the given string is not a valid number formatting skeleton.
     * @draft ICU 62
     */
    static UnlocalizedNumberFormatter fromSkeleton(const UnicodeString& skeleton, UErrorCode& status);

    /**
     * Use factory methods instead of the constructor to create a NumberFormatter.
     * @draft ICU 60
     */
    NumberFormatter() = delete;
};

}  // namespace number
U_NAMESPACE_END

#endif  // U_HIDE_DRAFT_API

#endif // __NUMBERFORMATTER_H__

#endif /* #if !UCONFIG_NO_FORMATTING */
