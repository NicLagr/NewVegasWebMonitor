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