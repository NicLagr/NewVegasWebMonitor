export { default as TheMap } from './TheMap.vue';
export {
  prefetchMapTiles,
  mapTileBlobUrls,
  mapTilesPrefetchActive,
  mapTilesPrefetchProgress,
  MAP_DZI_URL,
} from './preloadMap';
export {
  MOJAVE_MAP_IMAGE_WIDTH,
  MOJAVE_MAP_IMAGE_HEIGHT,
  projectWorldToImage,
  useMapProjection,
} from './composables/useMapProjection';
export type {
  MapProjectionFn,
  ProjectedPoint,
  UseMapProjection,
} from './composables/useMapProjection';
export {
  DEFAULT_MARKER_ICON,
  MARKER_ICON_MAP,
  resolveMarkerIcon,
} from './composables/useMapMarkerIcons';
