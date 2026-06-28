import { computed, type ComputedRef } from 'vue';

// =============================================================
// World → image projection (Fallout: New Vegas / Mojave)
// =============================================================
//
// The Mojave world map is a flat top-down render, so world→image is a simple
// UNIFORM scale with the Y axis flipped (game +Y = north = up = smaller image
// Y). No rotation/shear, so we only need a scale + an offset:
//
//   imageX =  worldX * SCALE + OFFSET_X
//   imageY = -worldY * SCALE + OFFSET_Y
//
// Calibrated to public/mojave-map.png (3500×3500): anchored on Freeside
// (world -9670, 111810) at the Strip on the image, with the scale fit so the
// far-south markers (Mojave Outpost / Nipton) sit near the bottom and the
// far-north ones (Northern Passage) near the top. The earlier per-landmark
// affine extrapolated badly and flung Vegas off the top edge.
//
// ⚠️ To nudge: if every marker is uniformly shifted, adjust OFFSET_X/OFFSET_Y;
// if they're too spread out / too cramped, adjust SCALE.
// =============================================================

/** Image px per world unit (uniform). */
const MAP_SCALE = 0.0103;
/** Image x for world x = 0. */
const MAP_OFFSET_X = 1700;
/** Image y for world y = 0. */
const MAP_OFFSET_Y = 2100;

export interface Point {
  x: number;
  y: number;
}

export interface ProjectedPoint extends Point {
  /** Normalised image coordinate in [0, 1] (x / width). */
  u: number;
  /** Normalised image coordinate in [0, 1] (y / height). */
  v: number;
}

export type MapProjectionFn = (point: Point) => ProjectedPoint | null;

export interface UseMapProjection {
  projectWorldToImage: MapProjectionFn;
  projectImageToWorld: (imgX: number, imgY: number) => Point | null;
  imageWidth: number;
  imageHeight: number;
  isReady: ComputedRef<boolean>;
}

// Natural pixel dimensions of public/mojave-map.png.
export const MOJAVE_MAP_IMAGE_WIDTH = 3500;
export const MOJAVE_MAP_IMAGE_HEIGHT = 3500;

export function projectWorldToImage(point: Point): ProjectedPoint | null {
  if (!Number.isFinite(point.x) || !Number.isFinite(point.y)) return null;

  const x = point.x * MAP_SCALE + MAP_OFFSET_X;
  const y = -point.y * MAP_SCALE + MAP_OFFSET_Y;

  return {
    x,
    y,
    u: x / MOJAVE_MAP_IMAGE_WIDTH,
    v: y / MOJAVE_MAP_IMAGE_HEIGHT,
  };
}

/**
 * Inverse of {@link projectWorldToImage}: image-pixel → game-world coordinates.
 * Used to place a custom marker by aiming the map reticle.
 */
export function projectImageToWorld(imgX: number, imgY: number): Point | null {
  if (!Number.isFinite(imgX) || !Number.isFinite(imgY)) return null;
  const x = (imgX - MAP_OFFSET_X) / MAP_SCALE;
  const y = (MAP_OFFSET_Y - imgY) / MAP_SCALE;
  return { x, y };
}

export function useMapProjection(): UseMapProjection {
  return {
    projectWorldToImage,
    projectImageToWorld,
    imageWidth: MOJAVE_MAP_IMAGE_WIDTH,
    imageHeight: MOJAVE_MAP_IMAGE_HEIGHT,
    isReady: computed(() => true),
  };
}
