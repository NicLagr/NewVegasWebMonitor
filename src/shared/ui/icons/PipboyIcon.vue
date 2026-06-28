<template>
  <div
    v-if="showPipboy"
    class="pipboy-icon"
    :style="{
      '--pi-src': `url('${src}')`,
      '--pi-size': `${size}px`,
      '--pi-color': color,
    }"
  />
  <slot v-else />
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { usePipboyIcons } from '@/shared/lib/composables/usePipboyIcons';

const props = withDefaults(
  defineProps<{
    /** In-game icon path (no ext), e.g. "interface/icons/pipboyimages/weapons/weapons_katana". */
    iconPath?: string | null;
    size?: number;
    /** Tint color (mask fill); follows the theme accent by default. */
    color?: string;
  }>(),
  {
    iconPath: null,
    size: 48,
    color: 'var(--skyrim-text-accent)',
  }
);

const { hasIcon } = usePipboyIcons();

// Only use the per-item icon when we actually shipped it; otherwise the <slot>
// (the existing type icon) renders as a fallback.
const showPipboy = computed(() => hasIcon(props.iconPath));
const src = computed(
  () => `${import.meta.env.BASE_URL}icons/pipboy/${props.iconPath}.webp`
);
</script>

<style scoped lang="scss">
/* Pip-Boy icons are white-on-alpha, so we mask + tint them to the theme color —
   matching BaseIcon, theme-aware, and preserving the icon's alpha detail. */
.pipboy-icon {
  display: block;
  width: var(--pi-size);
  height: var(--pi-size);
  background-color: var(--pi-color);
  transition: background-color var(--transition-fast);
  -webkit-mask: var(--pi-src) center / contain no-repeat;
  mask: var(--pi-src) center / contain no-repeat;
}
</style>
