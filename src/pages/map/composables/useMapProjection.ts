import { computed, type ComputedRef } from 'vue';
import {
  REFERENCE_POINTS,
  solveAffineLeastSquares,
  type AffineMatrix,
} from './useMapCoordinates';

// =============================================================
// World → image projection (Fallout: New Vegas / Mojave)
// =============================================================
//
// Skyrim shipped a precise triangle-mesh projection baked from its game files.
// Fallout NV has no such mesh here, so we use a least-squares AFFINE transform
// fitted from the calibration landmarks in `useMapCoordinates.ts`:
//
//   imageX = a*worldX + c*worldY + e
//   imageY = b*worldX + d*worldY + f
//
// A flat top-down Mojave render is well approximated by an affine map, so this
// is accurate enough once the reference points are calibrated to your image.
// =============================================================

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
  imageWidth: number;
  imageHeight: number;
  isReady: ComputedRef<boolean>;
}

// ⚠️ Set these to the pixel dimensions of your public/mojave-map.png.
// The projection outputs pixel coordinates in this space, and the marker
// overlay assumes the loaded image has exactly these natural dimensions.
export const MOJAVE_MAP_IMAGE_WIDTH = 3500;
export const MOJAVE_MAP_IMAGE_HEIGHT = 3500;

/**
 * Affine matrix (game → image px), fitted once at module load from the
 * calibrated reference points. `null` until at least 3 points are calibrated.
 */
const MATRIX: AffineMatrix | null = (() => {
  const usable = REFERENCE_POINTS.filter(
    (r): r is { name: string; game: Point; imagePx: Point } =>
      r.game != null &&
      r.imagePx != null &&
      Number.isFinite(r.game.x) &&
      Number.isFinite(r.game.y) &&
      Number.isFinite(r.imagePx.x) &&
      Number.isFinite(r.imagePx.y)
  );
  if (usable.length < 3) return null;
  return solveAffineLeastSquares(
    usable.map((r) => r.game),
    usable.map((r) => r.imagePx)
  );
})();

export function projectWorldToImage(point: Point): ProjectedPoint | null {
  if (!MATRIX) return null;
  if (!Number.isFinite(point.x) || !Number.isFinite(point.y)) return null;

  const x = MATRIX.a * point.x + MATRIX.c * point.y + MATRIX.e;
  const y = MATRIX.b * point.x + MATRIX.d * point.y + MATRIX.f;
  if (!Number.isFinite(x) || !Number.isFinite(y)) return null;

  return {
    x,
    y,
    u: x / MOJAVE_MAP_IMAGE_WIDTH,
    v: y / MOJAVE_MAP_IMAGE_HEIGHT,
  };
}

export function useMapProjection(): UseMapProjection {
  return {
    projectWorldToImage,
    imageWidth: MOJAVE_MAP_IMAGE_WIDTH,
    imageHeight: MOJAVE_MAP_IMAGE_HEIGHT,
    isReady: computed(() => MATRIX !== null),
  };
}
