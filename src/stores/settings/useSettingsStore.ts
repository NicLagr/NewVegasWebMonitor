import { defineStore } from 'pinia';
import { ref } from 'vue';

export type ThemeColor = 'amber' | 'green' | 'blue';

const THEME_KEY = 'nvwm-theme';
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

export const useSettingsStore = defineStore('settings', () => {
  const theme = ref<ThemeColor>(loadTheme());

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

  apply(); // apply current theme as soon as the store is created

  return { theme, themes: THEMES, setTheme };
});
