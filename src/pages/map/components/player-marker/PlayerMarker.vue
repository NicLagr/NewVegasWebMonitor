<template>
  <g :transform="`translate(${player.x} ${player.y}) rotate(${player.angleDeg})`">
    <!-- Inline arrowhead (pointing 'up'/north at 0°; the parent <g> applies the
         player's facing rotation) so it can be tinted to the active theme. -->
    <polygon
      class="player-marker"
      :points="arrowPoints"
      :stroke-width="playerHalfSize * 0.16"
      stroke-linejoin="round"
    />
  </g>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import type { PlayerOverlayPosition } from '../../types';

const props = defineProps<{
  player: PlayerOverlayPosition;
  playerSize: number;
  playerHalfSize: number;
  // Kept for API compatibility with MapMarkers; the marker is now drawn inline.
  iconSymbolId?: string;
}>();

const arrowPoints = computed(() => {
  const s = props.playerHalfSize;
  // tip, back-right, center notch, back-left
  return `0,${-s} ${s * 0.72},${s * 0.82} 0,${s * 0.32} ${-s * 0.72},${s * 0.82}`;
});
</script>

<style scoped lang="scss">
.player-marker {
  pointer-events: none;
  fill: var(--pip-accent-gold);
  stroke: var(--pip-bg-dark);
}
</style>
