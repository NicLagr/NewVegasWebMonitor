import { createApp } from 'vue';
import { createPinia } from 'pinia';
import '@/shared/lib/styles/skyrim-theme.scss';
import './style.css';
import App from './app/App.vue';
import i18n from './i18n';

// Push a guard entry immediately so the Android back gesture is intercepted
// even before Vue mounts (prevents instant app close on first back swipe).
if (!history.state?.pwaBackGuard) {
  history.pushState({ pwaBackGuard: true }, '');
}

// Register service worker for PWA only in production (avoid dev caching).
// The registration uses `updateViaCache: 'none'` so the browser always fetches
// a fresh `sw.js` instead of reusing a stale HTTP-cached copy. This matters on
// installed PWAs, where the browser can otherwise keep an old service worker
// around even after a new GitHub Pages deploy.
if (import.meta.env.PROD) {
  const intervalMs = 60 * 60 * 1000; // periodic re-check while app is open
  const swUrl = `${import.meta.env.BASE_URL}sw.js`;
  const swScope = import.meta.env.BASE_URL;

  void import('workbox-window').then(({ Workbox }) => {
    const workbox = new Workbox(swUrl, {
      scope: swScope,
      type: 'classic',
      updateViaCache: 'none',
    });

    workbox.addEventListener('activated', (event) => {
      if (event.isUpdate || event.isExternal) {
        window.location.reload();
      }
    });

    const update = () => {
      workbox.update().catch(() => {
        /* offline / network error — ignore */
      });
    };

    workbox
      .register({ immediate: true })
      .then((registration) => {
        if (!registration) return;

        // Check right after registration.
        update();

        // Periodic check while the tab/PWA stays open.
        setInterval(update, intervalMs);

        // And — most importantly — whenever the user comes back to the
        // app (PWA resumed from background, tab regains focus, bfcache).
        const onVisible = () => {
          if (document.visibilityState === 'visible') update();
        };

        document.addEventListener('visibilitychange', onVisible);
        window.addEventListener('focus', update);
        window.addEventListener('pageshow', update);
      })
      .catch(() => {
        /* registration error — ignore */
      });
  });
}

const app = createApp(App);

app.use(createPinia());
app.use(i18n);
app.mount('#app');
