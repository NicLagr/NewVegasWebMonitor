<template>
  <div class="inventory-list">
    <div class="list-wrapper">
      <div class="list">
        <!-- Items list -->
        <template v-if="displayItems.length > 0">
          <slot
            v-for="(item, index) in displayItems"
            :key="item.formId || index"
            :item="item"
            :active="modelValue === item.formId"
            :is-favorite="favorites.has(item.formId)"
            :on-select="() => handleItemClick(item.formId)"
          >
            <inventory-item
              v-if="'name' in item"
              :name="item.name || $t('shared.ui.inventoryItem.unknown')"
              :is-favorite="favorites.has(item.formId)"
              :active="modelValue === item.formId"
              :quantity="'count' in item ? item.count : 0"
              @click="handleItemClick(item.formId)"
            >
              <template #icon>
                <pipboy-icon
                  :icon-path="(item as { iconPath?: string }).iconPath"
                  :size="28"
                >
                  <base-icon
                    :icon-path="previewIconPath"
                    :size="28"
                  />
                </pipboy-icon>
              </template>
            </inventory-item>
          </slot>
        </template>

        <!-- Empty state -->
        <div
          v-else
          class="no-data"
        >
          <slot name="empty">
            {{ emptyMessage }}
          </slot>
        </div>
      </div>

      <!-- Action toolbar -->
      <div class="inventory-toolbar">
        <template
          v-for="(actionItem, index) in enabledActions"
          :key="index"
        >
          <div
            v-if="isActionGroup(actionItem)"
            class="d-flex gap-sm"
          >
            <button
              v-for="action in actionItem.group"
              :key="action.id"
              class="btn btn-icon toolbar-btn"
              :class="[
                action.class,
                { favorite: action.id === 'favorite' && isActiveItemFavorite },
              ]"
              :disabled="!modelValue"
              @click="handleActionClick(action.event)"
            >
              <base-icon
                :icon-path="action.icon"
                :size="20"
              />
            </button>
          </div>
          <button
            v-else
            class="btn btn-icon toolbar-btn"
            :class="[
              actionItem.class,
              {
                favorite: actionItem.id === 'favorite' && isActiveItemFavorite,
              },
            ]"
            :disabled="!modelValue"
            @click="handleActionClick(actionItem.event)"
          >
            <base-icon
              :icon-path="actionItem.icon"
              :size="20"
            />
          </button>
        </template>
      </div>
    </div>

    <div class="item-preview">
      <slot name="preview">
        <!-- Optional preview content goes here -->
        <base-preview
          v-if="activeItem"
          :data="activeItem"
          :stats="activeItemStats"
          :effects="previewEffects"
          :description="previewDescription"
        >
          <template #icon>
            <pipboy-icon
              :icon-path="previewItemIconPath"
              :size="96"
            >
              <base-icon
                :icon-path="previewIconPath"
                :size="96"
              />
            </pipboy-icon>
          </template>
        </base-preview>
      </slot>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { BaseIcon, PipboyIcon } from '@/shared/ui';
import { InventoryItem } from '@/shared/ui/items';
import { BasePreview } from '@/shared/ui/items';
import { useFavoritesStore } from '@/stores/favorites/useFavoritesStore';
import type { ItemEnchantmentEffect } from '@/shared/lib/types/common';
import type { PreviewStats } from '@/shared/ui/items/types/types';
import type { ListItem } from '@/shared/lib/types/types';

interface ToolbarAction {
  id: string;
  event: string;
  icon: string;
  class?: string;
}

interface ToolbarActionGroup {
  group: ToolbarAction[];
}

type ToolbarActionItem = ToolbarAction | ToolbarActionGroup;

function isActionGroup(item: ToolbarActionItem): item is ToolbarActionGroup {
  return 'group' in item;
}

interface Props {
  modelValue?: string | null;
  items: ListItem[];
  emptyMessage?: string;
  actions?: ToolbarActionItem[];
  activeItem?: ListItem | null;
  activeItemStats?: PreviewStats[];
  previewEffects?: ItemEnchantmentEffect[];
  previewIconPath?: string;
  /** Per-item Pip-Boy icon path (no ext); falls back to previewIconPath. */
  previewItemIconPath?: string;
  previewDescription?: string;
}

const props = withDefaults(defineProps<Props>(), {
  modelValue: null,
  emptyMessage: () => '...',
  actions: () => [
    { id: 'favorite', event: 'favorite', icon: 'delapouite/round-star.svg' },
    { id: 'drop', event: 'drop', icon: 'delapouite/trash-can.svg' },
  ],
  activeItem: null,
  activeItemStats: () => [],
  previewEffects: () => [],
  previewIconPath: 'lorc/cog.svg',
  previewItemIconPath: '',
  previewDescription: '',
});

const emit = defineEmits<{
  'update:modelValue': [value: string | null];
  favorite: [];
  drop: [];
  'item-double-click': [formId: string];
}>();

const favorites = useFavoritesStore();

const isActiveItemFavorite = computed(() =>
  props.modelValue ? favorites.has(props.modelValue) : false
);

// Favorited items sort to the top (app-local; FNV has no in-game favorites).
const displayItems = computed(() => {
  const items = props.items ?? [];
  const fav = items.filter((i) => favorites.has(i.formId));
  const rest = items.filter((i) => !favorites.has(i.formId));
  return [...fav, ...rest];
});

const enabledActions = computed((): ToolbarActionItem[] => {
  return props.actions || [];
});

function handleItemClick(formId: string) {
  if (props.modelValue !== formId) {
    // First click on new item - select it
    emit('update:modelValue', formId);
  } else {
    // Repeat click on already selected item - trigger action
    emit('item-double-click', formId);
  }
}

function handleActionClick(actionEvent: string) {
  emit(actionEvent as any);
}
</script>

<style scoped lang="scss">
/*
 * Toolbar buttons use .btn .btn-icon from the design system; only
 * the colour-token override behaviour for nested icons (favorite /
 * hotkey states) is component-specific.
 */

.inventory-list {
  display: flex;
  height: 100%;
  max-height: 100%;
  overflow: hidden;
  gap: var(--spacing-md);
}

.list-wrapper {
  flex: 0 0 var(--inventory-list-wrapper-width, 60%);
  min-width: 0;
  min-height: 0;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  gap: var(--spacing-md);
}

.list {
  min-width: 0;
  overflow: hidden auto;
}

.item-preview {
  flex: 1 1 0%;
  min-width: 0;
  overflow: hidden;
}

.inventory-toolbar {
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: var(--spacing-md);
}

.toolbar-btn {
  --pip-text-accent: var(--pip-text-secondary);

  @media (hover: hover) {
    &:hover:not(:disabled) {
      --pip-text-accent: var(--pip-text-primary);
    }

    &.favorite:hover:not(:disabled),
    &.hotkey-bound:hover:not(:disabled) {
      --pip-text-accent: var(--pip-accent-gold);
    }
  }

  &.favorite,
  &.hotkey-bound {
    --pip-text-accent: var(--pip-accent-gold);
  }
}
</style>
