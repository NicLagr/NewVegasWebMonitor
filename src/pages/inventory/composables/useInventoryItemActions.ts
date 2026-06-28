import { ref, computed, watch } from 'vue';
import { useWebSocketStore } from '@/stores/use-websocket-store/useWebsocketStore';
import { useModal } from '@/shared/lib/composables/useModal';
import { useFavoritesStore } from '@/stores/favorites/useFavoritesStore';
import { DropItemsModal } from '@/shared/ui';
import type { InventoryItem } from '@/stores/inventory/types';

export function useInventoryItemActions(itemsList: () => InventoryItem[]) {
  const wsStore = useWebSocketStore();
  const favorites = useFavoritesStore();
  const { closeModal, openModal } = useModal();

  const activeItem = ref<string | null>(null);

  const activeItemData = computed(() => {
    if (!activeItem.value) return null;
    return itemsList().find(item => item.formId === activeItem.value) || null;
  });

  function toggleFavorite() {
    if (!activeItem.value) return;
    // App-local favorite (pins to top); FNV has no in-game favorite state.
    favorites.toggle(activeItem.value);
  }

  function startDrop() {
    if (!activeItemData.value || !activeItem.value) return;

    const count = activeItemData.value.count;

    // If more than 5 items, show modal for quantity selection
    if (count > 5) {
      openModal({
        component: DropItemsModal,
        props: {
          maxCount: count,
        },
        on: {
          drop: (qty: number) => {
            wsStore.sendCommand({ command: 'drop', formId: activeItem.value!, count: qty });
            closeModal();
          },
        },
      });
      return;
    }

    // If 5 or fewer, drop one
    wsStore.sendCommand({ command: 'drop', formId: activeItem.value, count: 1 });
  }

  // Automatically select the first item when the items list becomes available,
  // or fall back to the previous neighbour when the active item disappears.
  let previousList: InventoryItem[] = [];
  watch(
    () => itemsList(),
    (newList) => {
      const list = newList || [];

      if (list.length === 0) {
        previousList = [];
        return;
      }

      if (!activeItem.value) {
        activeItem.value = list[0].formId;
        previousList = list.slice();
        return;
      }

      const stillExists = list.some(item => item.formId === activeItem.value);
      if (!stillExists) {
        // Find the previous neighbour in the old list that still exists in the new list.
        const oldIndex = previousList.findIndex(item => item.formId === activeItem.value);
        let fallback: string | null = null;
        if (oldIndex > 0) {
          for (let i = oldIndex - 1; i >= 0; i--) {
            const candidate = previousList[i].formId;
            if (list.some(item => item.formId === candidate)) {
              fallback = candidate;
              break;
            }
          }
        }
        activeItem.value = fallback ?? list[0].formId;
      }

      previousList = list.slice();
    },
    { immediate: true }
  );

  return {
    activeItem,
    activeItemData,
    toggleFavorite,
    startDrop,
  };
}
