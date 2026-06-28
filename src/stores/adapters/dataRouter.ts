import { useCharacterStore } from '@/stores/character/useCharacterStore';
import { useInventoryStore } from '@/stores/inventory/useInventoryStore';
import { useNavigationStore } from '@/stores/use-navigation-store/useNavigationStore';
import { useGameStatusStore } from '@/stores/game/useGameStatusStore';
import { useMapHotspotsStore } from '@/stores/map/useMapHotspotsStore';
import { useMapPlayerStore } from '@/stores/map/useMapPlayerStore';
import { useQuestStore } from '@/stores/quests/useQuestStore';
import { useRadioStore } from '@/stores/radio/useRadioStore';
import type { RouterResult } from './types';
import { isCharacterStatsData, isWeaponsData, isApparelData, isFoodData, isPotionsData, isKeysData, isBooksData, isInventoryCategories, isMiscData, isQuestsData, isRadioData, isGameStatusData, isMapHotspotsData, isMapQuestMarkersData, isPlayerPositionData } from './typeGuards';
export class DataRouter {
  static routeDataById(subscriptionId: string, data: unknown): RouterResult {
    const characterStore = useCharacterStore();
    const inventoryStore = useInventoryStore();
    try {
      if (isCharacterStatsData(data, subscriptionId)) {
        characterStore.setStats(data);
        return { success: true, message: 'Data routed to character store' };
      }

      if (isWeaponsData(data, subscriptionId)) {
        inventoryStore.setWeapons(data);
        return { success: true, message: 'Data routed to inventory store (weapons)' };
      }

      if (isApparelData(data, subscriptionId)) {
        inventoryStore.setApparel(data);
        return { success: true, message: 'Data routed to inventory store (apparel)' };
      }

      if (isFoodData(data, subscriptionId)) {
        inventoryStore.setFood(data);
        return { success: true, message: 'Data routed to inventory store (food)' };
      }

      if (isPotionsData(data, subscriptionId)) {
        inventoryStore.setPotions(data);
        return { success: true, message: 'Data routed to inventory store (potions)' };
      }

      if (isKeysData(data, subscriptionId)) {
        inventoryStore.setKeys(data);
        return { success: true, message: 'Data routed to inventory store (keys)' };
      }

      if (isBooksData(data, subscriptionId)) {
        inventoryStore.setBooks(data);
        return { success: true, message: 'Data routed to inventory store (books)' };
      }

      if (isMiscData(data, subscriptionId)) {
        inventoryStore.setMisc(data);
        return { success: true, message: 'Data routed to inventory store (misc)' };
      }

      if (isInventoryCategories(data, subscriptionId)) {
        const navigationStore = useNavigationStore();
        const subTabs = (data.categories || []).map((cat) => ({
          id: cat.categoryId.toLowerCase(),
          label: cat.name,
        }));

        // Order subTabs according to navigation store ordering map (if present),
        // using the same logic as `currentSubTabs` (ordered by map, then append rest).
        const order = (navigationStore.subTabsOrderMap as any)?.inventory ?? (navigationStore.subTabsOrderMap as any)?.value?.inventory ?? [];
        const ordered: typeof subTabs = [];
        const remaining = [...subTabs];

        if (Array.isArray(order) && order.length) {
          order.forEach((id: string) => {
            const idx = remaining.findIndex((s) => s.id === id);
            if (idx === -1) return;
            const [sub] = remaining.splice(idx, 1);
            ordered.push(sub);
          });
        }

        if (remaining.length) ordered.push(...remaining);

        navigationStore.setTabSubTabs('inventory', ordered);
        return { success: true, message: 'Data routed to navigation store (inventory categories)' };
      }

      if (isQuestsData(data, subscriptionId)) {
        useQuestStore().setQuests(data);
        return { success: true, message: 'Data routed to quests store' };
      }

      if (isRadioData(data, subscriptionId)) {
        useRadioStore().setRadio(data);
        return { success: true, message: 'Data routed to radio store' };
      }

      if (isGameStatusData(data, subscriptionId)) {
        useGameStatusStore().setStatus(data.status);
        return { success: true, message: 'Data routed to game status store' };
      }

      if (isMapHotspotsData(data, subscriptionId)) {
        useMapHotspotsStore().setHotspots(data);
        return { success: true, message: 'Data routed to map store (hotspots)' };
      }

      if (isMapQuestMarkersData(data, subscriptionId)) {
        useMapHotspotsStore().setQuestMarkers(data);
        return { success: true, message: 'Data routed to map store (quest markers)' };
      }

      if (isPlayerPositionData(data, subscriptionId)) {
        useMapPlayerStore().setPosition(data.position);
        return { success: true, message: 'Data routed to map store (player)' };
      }

      console.warn('[DataRouter] Unknown subscription ID received:', subscriptionId);
      return { success: false, message: `Unknown subscription ID: ${subscriptionId}` };
    } catch (err) {
      console.error('[DataRouter] Failed to route data by ID:', err);
      return {
        success: false,
        message: 'Failed to route data',
        error: err instanceof Error ? err : new Error(String(err)),
      };
    }
  }
}

