<template>
  <div
    class="skyrim-panel animate-slide-down"
    @touchstart.passive="onTouchStart"
    @touchend.passive="onTouchEnd"
    @touchcancel.passive="onTouchCancel"
  >
    <Transition
      :name="transitionName"
      mode="out-in"
    >
      <KeepAlive>
        <component
          :is="currentComponent"
          v-if="currentComponent"
          :key="`${tab}-${subTab}`"
        />
        <div
          v-else
          :key="`${tab}-${subTab}-empty`"
          class="empty-state"
        >
          <p class="text-secondary">
            {{ $t('app.content.emptyState') }}
          </p>
        </div>
      </KeepAlive>
    </Transition>
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue';
import { usePageRouter } from '@/app/router/usePageRouter';
import { useNavigationStore } from '@/stores/use-navigation-store/useNavigationStore';

const props = defineProps<{ tab: string; subTab: string }>();

const nav = useNavigationStore();

const transitionName = computed(() => {
  return nav.transitionDirection
    ? `slide-${nav.transitionDirection}`
    : 'no-slide';
});

const touchStartX = ref<number | null>(null);
const touchStartY = ref<number | null>(null);
// Horizontal travel needed to count as a page swipe...
const THRESHOLD = 75;
// ...and it must dominate vertical travel by this ratio, so scrolling (which
// often drifts sideways) never triggers an accidental page change.
const HORIZONTAL_DOMINANCE = 1.5;

const onTouchStart = (e: TouchEvent) => {
  touchStartX.value = e.touches?.[0]?.clientX ?? null;
  touchStartY.value = e.touches?.[0]?.clientY ?? null;
};

const onTouchEnd = (e: TouchEvent) => {
  if (touchStartX.value === null || touchStartY.value === null) return;
  const endX = e.changedTouches?.[0]?.clientX ?? 0;
  const endY = e.changedTouches?.[0]?.clientY ?? 0;
  const dx = endX - touchStartX.value;
  const dy = endY - touchStartY.value;
  touchStartX.value = null;
  touchStartY.value = null;
  // Only a deliberate, mostly-horizontal swipe flips the page.
  if (Math.abs(dx) < THRESHOLD) return;
  if (Math.abs(dx) < Math.abs(dy) * HORIZONTAL_DOMINANCE) return;
  if (dx < 0) {
    nav.nextSubTab();
  } else {
    nav.prevSubTab();
  }
};

const onTouchCancel = () => {
  touchStartX.value = null;
  touchStartY.value = null;
};

const currentComponent = computed(() => {
  return usePageRouter(props.tab, props.subTab);
});
</script>

<style scoped lang="scss">
.skyrim-panel {
  position: relative;
  background-color: var(--skyrim-bg-medium);
  padding: var(--spacing-sm);
  height: 100%;
  max-height: 100%;
  overflow: hidden auto;
  display: flex;
  flex-direction: column;
}

/* Slide animations */
.slide-left-enter-active,
.slide-left-leave-active,
.slide-right-enter-active,
.slide-right-leave-active {
  transition: transform 200ms cubic-bezier(0.22, 1, 0.36, 1);
  will-change: transform;
}

.slide-left-enter-from {
  transform: translateX(100%);
}

.slide-left-enter-to {
  transform: translateX(0);
}

.slide-left-leave-from {
  transform: translateX(0);
}

.slide-left-leave-to {
  transform: translateX(-100%);
}

.slide-right-enter-from {
  transform: translateX(-100%);
}

.slide-right-enter-to {
  transform: translateX(0);
}

.slide-right-leave-from {
  transform: translateX(0);
}

.slide-right-leave-to {
  transform: translateX(100%);
}

.no-slide-enter-active,
.no-slide-leave-active {
  transition: none;
}
</style>
