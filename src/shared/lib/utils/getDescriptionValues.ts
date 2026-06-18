export function getRoundValue(value: number | null | undefined): string | number {
    if(!value) return '-';
    if ( value < 0) return 0;
    return Math.round(value);
}

/** Weight display — keeps up to 2 decimals (so light items like 0.3 don't round to 0). */
export function getWeightValue(value: number | null | undefined): string {
    if (value == null || !Number.isFinite(value)) return '-';
    if (value <= 0) return '0';
    return String(Math.round(value * 100) / 100);
}

/**
 * Stack-aware weight: per-unit weight, plus the stack total in parens when the
 * stack holds more than one (mirrors the Pip-Boy, which shows the stack total).
 * e.g. 5 frag grenades @ 0.5 -> "0.5 (2.5)".
 */
export function getStackWeight(value: number | null | undefined, count: number | null | undefined): string {
    const per = getWeightValue(value);
    if (per === '-' || per === '0') return per;
    const n = count ?? 1;
    if (n <= 1 || value == null) return per;
    return `${per} (${getWeightValue(value * n)})`;
}

/**
 * Stack-aware value: per-unit value, plus the stack total in parens when count > 1.
 * e.g. 5 frag grenades @ 25 -> "25 (125)".
 */
export function getStackValue(value: number | null | undefined, count: number | null | undefined): string | number {
    const per = getRoundValue(value);
    const n = count ?? 1;
    if (n <= 1 || value == null || value <= 0) return per;
    return `${per} (${getRoundValue(value * n)})`;
}