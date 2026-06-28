import { defineStore } from 'pinia';
import { ref } from 'vue';

export type ThemeColor = 'amber' | 'green' | 'blue';

const THEME_KEY = 'nvwm-theme';
const HIDDEN_FX_KEY = 'nvwm-show-hidden-effects';
const THEMES: ThemeColor[] = ['amber', 'green', 'blue'];

function loadTheme(): ThemeColor {
  try {
    const v = localStorage.getItem(THEME_KEY) as ThemeColor | null;
    if (v && THEMES.includes(v)) return v;
  } catch {
    /* ignore */
  }
  return 'amber'; // classic FNV Pip-Boy default
}

function loadShowHiddenEffects(): boolean {
  try {
    return localStorage.getItem(HIDDEN_FX_KEY) === 'true';
  } catch {
    return false;
  }
}

export const useSettingsStore = defineStore('settings', () => {
  const theme = ref<ThemeColor>(loadTheme());
  // When true, the STATUS screen's EFF list also shows ability/passive effects
  // (traits, crippled-limb effects) the in-game Pip-Boy normally hides.
  const showHiddenEffects = ref<boolean>(loadShowHiddenEffects());

  /** Apply the palette by setting data-theme on <html> (variables.scss reacts). */
  function apply(): void {
    document.documentElement.setAttribute('data-theme', theme.value);
  }

  function setTheme(next: ThemeColor): void {
    theme.value = next;
    try {
      localStorage.setItem(THEME_KEY, next);
    } catch {
      /* storage may be unavailable */
    }
    apply();
  }

  function setShowHiddenEffects(next: boolean): void {
    showHiddenEffects.value = next;
    try {
      localStorage.setItem(HIDDEN_FX_KEY, next ? 'true' : 'false');
    } catch {
      /* storage may be unavailable */
    }
  }

  apply(); // apply current theme as soon as the store is created

  return { theme, themes: THEMES, setTheme, showHiddenEffects, setShowHiddenEffects };
});
