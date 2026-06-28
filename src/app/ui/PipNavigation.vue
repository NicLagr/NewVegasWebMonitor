<template>
  <header class="navigation-header">
    <nav
      ref="tabsRef"
      class="tab-bar"
      role="tablist"
      :aria-label="$t('app.navigation.mainAriaLabel')"
    >
      <button
        v-for="tab in nav.tabs"
        :key="tab.id"
        class="tab"
        :class="{ active: nav.activeTab === tab.id }"
        role="tab"
        :aria-selected="nav.activeTab === tab.id"
        @click="nav.setActiveTab(tab.id)"
      >
        {{ tab.label }}
      </button>
    </nav>

    <button
      type="button"
      class="settings-btn"
      :title="$t('settings.title')"
      :aria-label="$t('settings.title')"
      @click="openSettings"
    >
      <base-icon
        icon-path="lorc/cog.svg"
        :size="18"
        background-color="var(--pip-text-dim)"
      />
    </button>

    <nav
      v-if="visibleSubTabs.length > 0"
      ref="subtabsRef"
      class="subtab-bar animate-fade-in"
      role="tablist"
      :aria-label="$t('app.navigation.subAriaLabel')"
    >
      <button
        v-for="sub in visibleSubTabs"
        :key="sub.id"
        class="subtab"
        :class="{ active: nav.activeSubTab === sub.id }"
        role="tab"
        :aria-selected="nav.activeSubTab === sub.id"
        @click="nav.setActiveSubTab(sub.id)"
      >
        {{
          $te(`app.tabs.${nav.activeTab}.subtabs.${sub.id}`)
            ? $t(`app.tabs.${nav.activeTab}.subtabs.${sub.id}`)
            : sub.label
        }}
      </button>
    </nav>
  </header>
</template>

<script setup lang="ts">
import { computed, ref, watch, nextTick } from 'vue';
import { useNavigationStore } from '@/stores/use-navigation-store/useNavigationStore';
import { useModal } from '@/shared/lib/composables/useModal';
import { BaseIcon } from '@/shared/ui';
import SettingsModal from './SettingsModal.vue';

const tabsRef = ref<HTMLElement | null>(null);
const subtabsRef = ref<HTMLElement | null>(null);
const nav = useNavigationStore();

const { openModal, closeModal } = useModal();
function openSettings() {
  openModal({
    component: SettingsModal,
    ghostClickGuardMs: 300,
    on: { close: closeModal },
  });
}

const visibleSubTabs = computed(() => nav.getVisibleSubTabs());

// Center the active item in the horizontally scrollable container.
// If items fit, CSS `justify-content: safe center` centers them and
// scrollWidth <= clientWidth, so scrollTo is clamped to 0 (no-op).
function centerActive(container: HTMLElement | null, activeSelector: string) {
  if (!container) return;
  const activeBtn = container.querySelector(
    activeSelector
  ) as HTMLElement | null;
  if (!activeBtn) return;

  const btnCenter = activeBtn.offsetLeft + activeBtn.offsetWidth / 2;
  const targetLeft = btnCenter - container.clientWidth / 2;
  const maxLeft = container.scrollWidth - container.clientWidth;

  container.scrollTo({
    left: Math.max(0, Math.min(targetLeft, maxLeft)),
    behavior: 'smooth',
  });
}

watch(
  () => nav.activeTab,
  async () => {
    await nextTick();
    centerActive(tabsRef.value, '.tab.active');
    if (subtabsRef.value) {
      subtabsRef.value.scrollLeft = 0;
    }
  }
);

watch(
  () => nav.activeSubTab,
  async () => {
    await nextTick();
    centerActive(subtabsRef.value, '.subtab.active');
  }
);
</script>

<style scoped lang="scss">
/*
 * Tab and subtab styles come from the design system:
 *   .tab-bar, .tab, .subtab-bar, .subtab (components/tabs.scss).
 * Only the header wrapper (sticky positioning) is component-specific.
 */

.navigation-header {
  flex-shrink: 0;
  background-color: var(--pip-bg-medium);
  position: relative;
  z-index: var(--z-sticky);
}

.settings-btn {
  position: absolute;
  top: var(--spacing-xs);
  right: var(--spacing-sm);
  z-index: var(--z-raised);
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 1.75rem;
  height: 1.75rem;
  padding: 0;
  background-color: transparent;
  border: none;
  cursor: pointer;
  opacity: 0.7;
  transition: opacity var(--transition-fast);

  &:hover {
    opacity: 1;
  }
}

.tab-bar {
  justify-content: flex-start;
  padding-inline: var(--spacing-sm);
  scroll-padding-inline: var(--spacing-sm);
}

.tab-bar > .tab:first-child {
  margin-left: auto;
}

.tab-bar > .tab:last-child {
  margin-right: auto;
}

.subtab-bar {
  justify-content: flex-start;
  padding-inline: var(--spacing-sm);
  scroll-padding-inline: var(--spacing-sm);
}

.subtab-bar > .subtab:first-child {
  margin-left: auto;
}

.subtab-bar > .subtab:last-child {
  margin-right: auto;
}
</style>
