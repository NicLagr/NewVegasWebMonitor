<template>
  <div class="map-outer">
    <div class="map-page">
      <div
        ref="osdContainerRef"
        class="osd-host"
      />
      <map-markers
        v-if="imgNaturalW && imgNaturalH"
        ref="markersRef"
        class="map-overlay"
        :img-natural-w="imgNaturalW"
        :img-natural-h="imgNaturalH"
        :scale="scale"
        :cover-scale="coverScale"
        :overlay-style="overlayStyle"
      />
      <!-- Pip-Boy terminal frame: corner brackets around the map viewport. -->
      <div
        class="map-brackets"
        aria-hidden="true"
      >
        <span class="b tl" />
        <span class="b tr" />
        <span class="b bl" />
        <span class="b br" />
      </div>

      <!-- Custom-marker placement: fixed center reticle to aim under. -->
      <div
        v-if="placementMode"
        class="map-reticle"
        aria-hidden="true"
      >
        <span class="map-reticle__h" />
        <span class="map-reticle__v" />
        <span class="map-reticle__dot" />
      </div>

      <!-- Bottom-right control stack. -->
      <div class="map-controls">
        <button
          v-if="!placementMode"
          type="button"
          class="map-btn"
          :title="$t('pages.map.placeMarker')"
          @click="enterPlacement"
        >
          <base-icon
            icon-path="map/location.svg"
            background-color="var(--skyrim-text-secondary)"
          />
        </button>
        <button
          type="button"
          class="map-btn map-follow-player-btn"
          :class="{ 'is-active': isFollowPlayerMode }"
          :aria-pressed="isFollowPlayerMode"
          @click="toggleFollowPlayerMode"
        >
          <base-icon
            icon-path="map/player.svg"
            :background-color="isFollowPlayerMode ? 'var(--skyrim-accent-gold-light)' : 'var(--skyrim-text-dim)' "
          />
        </button>
      </div>

      <!-- Placement confirm bar. -->
      <div
        v-if="placementMode"
        class="map-place-bar"
      >
        <span class="map-place-hint">{{ $t('pages.map.placeHint') }}</span>
        <div class="map-place-actions">
          <button
            type="button"
            class="map-place-btn"
            @click="confirmPlacement"
          >
            {{ $t('pages.map.place') }}
          </button>
          <button
            type="button"
            class="map-place-btn map-place-btn--ghost"
            @click="cancelPlacement"
          >
            {{ $t('pages.map.cancel') }}
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref, watch, type StyleValue } from 'vue';
import { storeToRefs } from 'pinia';
import OpenSeadragon from 'openseadragon';
import MapMarkers from './MapMarkers.vue';
import { BaseIcon } from '@/shared/ui';
import { useMapProjection } from './composables/useMapProjection';
import { useMapPlayerStore } from '@/stores/map/useMapPlayerStore';
import { useCustomMarkerStore } from '@/stores/map/useCustomMarkerStore';
import { useWebSocketStore } from '@/stores/use-websocket-store/useWebsocketStore';

// =============================================================
// Map view configuration
// =============================================================

/**
 * Single static map image, served from public/. Drop your Mojave map here as
 * `public/mojave-map.png` (PNG/JPG/WebP all work). OpenSeadragon's simple-image
 * source auto-detects its pixel dimensions; make sure those match
 * MOJAVE_MAP_IMAGE_WIDTH/HEIGHT in useMapProjection.ts so markers line up.
 */
const MAP_IMAGE_URL = `${import.meta.env.BASE_URL}mojave-map.png`;
/** Initial zoom factor relative to the home zoom (1 = show the whole map). */
const INITIAL_ZOOM_FACTOR = 1;
/** Max zoom factor relative to home zoom. */
const MAX_ZOOM_FACTOR = 1;
/** Background color around the map. */
const BACKGROUND_COLOR = 'var(--skyrim-bg-medium)';
/**
 * Pixels to hide on each edge of the map image. The Mojave map is expected to
 * be pre-cropped, so these are 0; bump them if your image has dark borders.
 */
const MAP_CROP_X = 0;
/** Pixels to hide on the TOP edge of the map image. */
const MAP_CROP_Y_TOP = 0;
/** Pixels to hide on the BOTTOM edge of the map image. */
const MAP_CROP_Y_BOTTOM = 0;


// =============================================================
// State
// =============================================================

const osdContainerRef = ref<HTMLElement | null>(null);
const markersRef = ref<InstanceType<typeof MapMarkers> | null>(null);

const imgNaturalW = ref(0);
const imgNaturalH = ref(0);

/**
 * Current absolute scale (CSS px per natural image px) and translate of the
 * image inside the viewport. Synced from OSD on every viewport update so the
 * marker SVG overlay can mirror the image transform exactly.
 */
const scale = ref(0);
const translateX = ref(0);
const translateY = ref(0);
const containerWidth = ref(0);
const containerHeight = ref(0);
const isFollowPlayerMode = ref(false);

const playerStore = useMapPlayerStore();
const { displayPosition } = storeToRefs(playerStore);
const { projectWorldToImage, projectImageToWorld } = useMapProjection();
const customMarkerStore = useCustomMarkerStore();
const wsStore = useWebSocketStore();

const overlayStyle = computed<StyleValue>(() => ({
  width: `${imgNaturalW.value}px`,
  height: `${imgNaturalH.value}px`,
  transform: `translate3d(${translateX.value}px, ${translateY.value}px, 0) scale(${scale.value})`,
  transformOrigin: '0 0',
}));

const coverScale = computed(() => {
  if (!containerWidth.value || !containerHeight.value || !imgNaturalW.value || !imgNaturalH.value) {
    return 1;
  }
  return Math.max(
    containerWidth.value / imgNaturalW.value,
    containerHeight.value / imgNaturalH.value
  );
});

// =============================================================
// OpenSeadragon viewer
// =============================================================

let viewer: OpenSeadragon.Viewer | null = null;

// Custom-marker placement: a fixed reticle stays at the screen center while the
// user pans/zooms the map under it, then confirms. This avoids fighting the
// map's pan gesture (the old long-press did).
const placementMode = ref(false);

function enterPlacement(): void {
  isFollowPlayerMode.value = false; // free panning while aiming the reticle
  placementMode.value = true;
}

function cancelPlacement(): void {
  placementMode.value = false;
}

function confirmPlacement(): void {
  placementMode.value = false;
  if (!viewer || !imgNaturalW.value) return;
  const item = viewer.world.getItemAt(0);
  if (!item) return;
  // The reticle sits at the viewport center, so its map location is the
  // image-coordinate of the current viewport center.
  const center = viewer.viewport.getCenter(true);
  const img = item.viewportToImageCoordinates(center);
  const world = projectImageToWorld(img.x, img.y);
  if (!world) return;
  customMarkerStore.setMarker(world);
  // Mirror onto the in-game Pip-Boy world map (custom destination marker).
  wsStore.sendCommand({ command: 'player_marker_set', x: world.x, y: world.y });
}

type OsdTileSource = NonNullable<OpenSeadragon.Options['tileSources']>;

function detectTileSource(): OsdTileSource {
  // Single static image. OSD's simple-image source loads the whole picture and
  // derives its natural dimensions — no DZI tiling / prefetch pipeline needed.
  return { type: 'image', url: MAP_IMAGE_URL } as unknown as OsdTileSource;
}

function syncOverlayTransform(): void {
  if (!viewer || !imgNaturalW.value) return;
  const item = viewer.world.getItemAt(0);
  if (!item) return;

  // Project the image's (0,0) and (W,0) onto the viewer's CSS pixel space to
  // recover the affine transform (uniform scale + translate).
  const p0v = item.imageToViewportCoordinates(0, 0, true);
  const pXv = item.imageToViewportCoordinates(imgNaturalW.value, 0, true);
  const p0 = viewer.viewport.pixelFromPoint(p0v, true);
  const pX = viewer.viewport.pixelFromPoint(pXv, true);

  const s = (pX.x - p0.x) / imgNaturalW.value;
  if (s > 0 && Number.isFinite(s)) {
    if (Math.abs(scale.value - s) > 1e-6) {
      scale.value = s;
    }
    if (Math.abs(translateX.value - p0.x) > 1e-3) {
      translateX.value = p0.x;
    }
    if (Math.abs(translateY.value - p0.y) > 1e-3) {
      translateY.value = p0.y;
    }
  }
}

function syncContainerSize(): void {
  const cont = osdContainerRef.value;
  if (!cont) return;
  const w = cont.clientWidth;
  const h = cont.clientHeight;
  if (containerWidth.value !== w) {
    containerWidth.value = w;
  }
  if (containerHeight.value !== h) {
    containerHeight.value = h;
  }
}

function centerOnPlayer(immediately = true): void {
  if (!viewer || !isFollowPlayerMode.value) return;
  const dp = displayPosition.value;
  if (!dp) return;
  const projected = projectWorldToImage(dp);
  if (!projected) return;
  const item = viewer.world.getItemAt(0);
  if (!item) return;

  const targetCenter = item.imageToViewportCoordinates(projected.x, projected.y, true);
  const currentCenter = viewer.viewport.getCenter(true);
  const dx = targetCenter.x - currentCenter.x;
  const dy = targetCenter.y - currentCenter.y;
  if (dx * dx + dy * dy < 1e-10) return;

  viewer.viewport.panTo(targetCenter, immediately);
  // Keep OSD constraints in charge near edges: when the player is close to a
  // border we prefer clamped panning over exposing off-map areas.
  viewer.viewport.applyConstraints(true);
  syncOverlayTransform();
}

function stopFollowPlayerByUser(): void {
  if (!isFollowPlayerMode.value) return;
  isFollowPlayerMode.value = false;
}

function toggleFollowPlayerMode(): void {
  isFollowPlayerMode.value = !isFollowPlayerMode.value;
  if (isFollowPlayerMode.value) {
    centerOnPlayer(true);
  }
}

function applyHomeBounds(): void {
  if (!viewer) return;
  // Clamp current zoom to a sane initial level relative to home (cover).
  const home = viewer.viewport.getHomeZoom();
  const target = home * INITIAL_ZOOM_FACTOR;
  viewer.viewport.zoomTo(target, undefined, true);
  viewer.viewport.applyConstraints(true);
  syncOverlayTransform();
}

function setupViewer(): void {
  const host = osdContainerRef.value;
  if (!host) return;

  const tileSources = detectTileSource();

  viewer = OpenSeadragon({
    element: host,
    tileSources,
    prefixUrl: '',
    showNavigator: false,
    showNavigationControl: false,
    showSequenceControl: false,
    showFullPageControl: false,
    showHomeControl: false,
    showZoomControl: false,
    showRotationControl: false,
    // Fit the WHOLE map inside the viewport (contain, not cover). The Mojave
    // map is square and the panel is wide, so letterbox space on the sides is
    // expected — visibilityRatio < 1 allows that instead of forcing a crop.
    homeFillsViewer: false,
    visibilityRatio: 0.5,
    constrainDuringPan: true,
    minZoomImageRatio: 0.5,
    maxZoomPixelRatio: MAX_ZOOM_FACTOR,
    animationTime: 0.3,
    springStiffness: 1,
    // false = OSD keeps the previous LOD visible until the next one is fully
    // loaded. With `true` it would clear and re-render immediately, exposing
    // the placeholderFillStyle as a visible flash during fast zoom-out.
    immediateRender: false,
    preserveImageSizeOnResize: true,
    blendTime: 0,
    alwaysBlend: false,
    // Used only as the very last resort when no tile of any LOD is available.
    placeholderFillStyle: BACKGROUND_COLOR,
    // Load lower-res tiles a bit earlier so during a fast zoom there is
    // always *something* sharp-ish to show before the target LOD arrives.
    minPixelRatio: 0.5,
    // Larger cache + more parallel network slots = far less popping during
    // rapid zoom/pan. Numbers are conservative; tune up if memory allows.
    imageLoaderLimit: 8,
    maxImageCacheCount: 2048,
    // Pre-fetch tiles around the current viewport so panning/zooming has
    // them ready instead of starting a request only when they enter view.
    preload: true,
    // Avoid sub-pixel half-transparent seams between adjacent tiles.
    subPixelRoundingForTransparency:
      OpenSeadragon.SUBPIXEL_ROUNDING_OCCURRENCES.ALWAYS as unknown as object,
    smoothTileEdgesMinZoom: Infinity,
    gestureSettingsMouse: {
      clickToZoom: false,
      dblClickToZoom: false,
      flickEnabled: true,
      scrollToZoom: true,
      pinchToZoom: true,
      dragToPan: true,
    },
    gestureSettingsTouch: {
      clickToZoom: false,
      dblClickToZoom: false,
      flickEnabled: true,
      scrollToZoom: false,
      pinchToZoom: true,
      dragToPan: true,
    },
    gestureSettingsPen: {
      clickToZoom: false,
      dblClickToZoom: false,
      flickEnabled: true,
      pinchToZoom: true,
      dragToPan: true,
    },
  });

  viewer.addHandler('open', () => {
    if (!viewer) return;
    const item = viewer.world.getItemAt(0);
    if (!item) return;
    const size = item.getContentSize();
    imgNaturalW.value = size.x;
    imgNaturalH.value = size.y;

    // Reposition the item so only the non-dark cropped area is considered
    // the "world" by OSD. This makes homeFillsViewer, visibilityRatio and
    // constrainDuringPan all work relative to the cropped region, which
    // naturally prevents the user from panning into the dark edges.
    if (MAP_CROP_X > 0 || MAP_CROP_Y_TOP > 0 || MAP_CROP_Y_BOTTOM > 0) {
      const W = size.x;
      const croppedW = W - 2 * MAP_CROP_X;
      const croppedH = size.y - MAP_CROP_Y_TOP - MAP_CROP_Y_BOTTOM;
      // In OSD viewport space the item default width is 1.0. We scale it so
      // the cropped region spans exactly 1.0 viewport unit.
      const newWidth = W / croppedW;
      item.setWidth(newWidth);
      item.setPosition(
        new OpenSeadragon.Point(-MAP_CROP_X / croppedW, -MAP_CROP_Y_TOP / croppedW),
      );
      // Also clip rendering so OSD won't bother loading tiles for the dark area.
      item.setClip(new OpenSeadragon.Rect(MAP_CROP_X, MAP_CROP_Y_TOP, croppedW, croppedH));
    }

    syncContainerSize();
    applyHomeBounds();
    centerOnPlayer(true);
  });

  viewer.addHandler('update-viewport', syncOverlayTransform);
  viewer.addHandler('resize', () => {
    syncContainerSize();
    syncOverlayTransform();
    centerOnPlayer(true);
  });

  // Any explicit user pan/zoom gesture exits follow mode back to free view.
  viewer.addHandler('canvas-drag', stopFollowPlayerByUser);
  viewer.addHandler('canvas-scroll', stopFollowPlayerByUser);
  viewer.addHandler('canvas-pinch', stopFollowPlayerByUser);

  viewer.addHandler('canvas-click', (event) => {
    if (!viewer) return;
    if (!event.quick) return;
    // While aiming the placement reticle, taps shouldn't select markers.
    if (placementMode.value) return;
    const item = viewer.world.getItemAt(0);
    if (!item) return;
    const viewportPoint = viewer.viewport.pointFromPixel(event.position);
    const imgPoint = item.viewportToImageCoordinates(viewportPoint);
    // Let the marker overlay try to handle the tap (selection / fast-travel).
    // Only deselect when the tap landed on empty map area.
    const hit = markersRef.value?.handleClickAt(imgPoint.x, imgPoint.y) ?? false;
    if (!hit) {
      markersRef.value?.clearSelection();
    }
  });
}

// =============================================================
// Lifecycle
// =============================================================

onMounted(() => {
  setupViewer();
});

watch(displayPosition, () => {
  centerOnPlayer(true);
});

onBeforeUnmount(() => {
  if (viewer) {
    viewer.destroy();
    viewer = null;
  }
  // Note: blob URLs in the shared `mapTileBlobUrls` cache are intentionally
  // NOT revoked here. They live for the lifetime of the page so that
  // re-entering the Map tab is instant.
});
</script>

<style scoped lang="scss">
.map-outer {
  position: relative;
  flex: 1 1 auto;
  width: 100%;
  height: 100%;
  min-height: 0;
  padding: var(--spacing-sm);
  background-color: var(--skyrim-bg-dark);
}

/* Pip-Boy terminal screen: themed bezel + inner CRT vignette + faint glow. */
.map-page {
  position: absolute;
  inset: var(--spacing-sm);
  overflow: hidden;
  background-color: v-bind(BACKGROUND_COLOR);
  border: var(--border-normal) solid var(--skyrim-border-medium);
  border-radius: var(--radius-sm);
  box-shadow:
    inset 0 0 40px rgb(0 0 0 / 45%),
    0 0 12px var(--skyrim-border-glow);
  touch-action: none;
  user-select: none;
  -webkit-user-select: none;
}

.osd-host {
  position: absolute;
  inset: 0;
}

.map-overlay {
  position: absolute;
  top: 0;
  left: 0;
  pointer-events: none;
}

/* Terminal corner brackets framing the screen. */
.map-brackets {
  position: absolute;
  inset: 0;
  pointer-events: none;
  z-index: 5;

  .b {
    position: absolute;
    width: 14px;
    height: 14px;
    border: var(--border-thick) solid var(--skyrim-border-accent);
    opacity: 0.8;
  }

  .tl { top: 4px; left: 4px; border-right: none; border-bottom: none; }
  .tr { top: 4px; right: 4px; border-left: none; border-bottom: none; }
  .bl { bottom: 4px; left: 4px; border-right: none; border-top: none; }
  .br { bottom: 4px; right: 4px; border-left: none; border-top: none; }
}

/* Center placement reticle. */
.map-reticle {
  position: absolute;
  inset: 0;
  z-index: 6;
  pointer-events: none;
  display: flex;
  align-items: center;
  justify-content: center;

  &__h,
  &__v {
    position: absolute;
    background-color: var(--skyrim-accent-gold);
    box-shadow: 0 0 4px var(--skyrim-border-glow);
  }

  &__h { width: 42px; height: 2px; }
  &__v { width: 2px; height: 42px; }

  &__dot {
    position: absolute;
    width: 10px;
    height: 10px;
    border: var(--border-thick) solid var(--skyrim-accent-gold);
    border-radius: 999px;
  }
}

/* Bottom-right control stack. */
.map-controls {
  position: absolute;
  right: calc(var(--spacing-md) + env(safe-area-inset-right));
  bottom: calc(var(--spacing-md) + env(safe-area-inset-bottom));
  z-index: 7;
  display: flex;
  flex-direction: column;
  gap: var(--spacing-sm);
}

.map-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 2.25rem;
  height: 2.25rem;
  padding: 0;
  border: var(--border-thin) solid var(--skyrim-border-medium);
  border-radius: 999px;
  background-color: var(--skyrim-bg-medium);
  box-shadow: var(--shadow-medium);
  cursor: pointer;
  transition:
    background-color var(--transition-fast),
    border-color var(--transition-fast);

  &:active { transform: scale(0.96); }
  &.is-active { border-color: var(--skyrim-border-accent); }
}

/* Placement confirm bar, bottom-center. */
.map-place-bar {
  position: absolute;
  left: 50%;
  bottom: calc(var(--spacing-md) + env(safe-area-inset-bottom));
  transform: translateX(-50%);
  z-index: 7;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: var(--spacing-sm);
}

.map-place-hint {
  padding: 2px var(--spacing-sm);
  background-color: var(--skyrim-bg-dark);
  border: var(--border-thin) solid var(--skyrim-border-dark);
  border-radius: var(--radius-sm);
  color: var(--skyrim-text-secondary);
  font-family: var(--font-heading);
  font-size: var(--font-size-sm);
}

.map-place-actions {
  display: flex;
  gap: var(--spacing-sm);
}

.map-place-btn {
  padding: var(--spacing-xs) var(--spacing-lg);
  border: var(--border-thin) solid var(--skyrim-border-accent);
  border-radius: var(--radius-sm);
  background-color: var(--skyrim-bg-medium);
  color: var(--skyrim-text-accent);
  font-family: var(--font-heading);
  font-size: var(--font-size-base);
  letter-spacing: 0.06em;
  cursor: pointer;

  &:active { transform: scale(0.97); }

  &--ghost {
    border-color: var(--skyrim-border-medium);
    color: var(--skyrim-text-secondary);
  }
}

.map-prefetch-backdrop {
  cursor: progress;
}

.map-prefetch-backdrop-enter-active,
.map-prefetch-backdrop-leave-active {
  transition: opacity var(--transition-normal);
}

.map-prefetch-backdrop-enter-from,
.map-prefetch-backdrop-leave-to {
  opacity: 0;
}

.map-prefetch-backdrop__panel {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: var(--spacing-sm);
  min-width: 14rem;
  padding: var(--spacing-md) var(--spacing-lg);
  background-color: var(--skyrim-bg-medium);
  border: var(--border-thin) solid var(--skyrim-border-medium);
  border-radius: var(--radius-md);
  box-shadow: var(--shadow-medium);
  color: var(--skyrim-text-accent);
  font-family: var(--font-heading);
  font-size: var(--font-size-sm);
  letter-spacing: 0.04em;
}

.map-prefetch-backdrop__label {
  color: var(--skyrim-text-accent);
}

.map-prefetch-backdrop__bar {
  position: relative;
  width: 100%;
  height: 6px;
  overflow: hidden;
  background-color: var(--skyrim-bg-dark);
  border: var(--border-thin) solid var(--skyrim-border-dark);
  border-radius: var(--radius-sm);

  &::after {
    content: '';
    position: absolute;
    inset: 0;
    width: var(--p, 0%);
    background: linear-gradient(
      90deg,
      var(--skyrim-accent-gold-dim),
      var(--skyrim-accent-gold-light)
    );
    transition: width var(--transition-fast);
  }
}

.map-prefetch-backdrop__pct {
  color: var(--skyrim-text-secondary);
  font-family: var(--font-body);
  font-variant-numeric: tabular-nums;
}
</style>
