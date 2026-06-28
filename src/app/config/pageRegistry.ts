import type { Component } from 'vue';
import type {
  PageConfig,
  PagesRegistry,
  CategorySubscriptionConfig,
  PageSubscriptionConfig,
} from './types';
import {
  TheStats,
  TheWeapons,
  TheApparel,
  TheMisc,
  ThePotions,
  TheFood,
  TheKeys,
  TheBooks,
  TheQuests,
  TheMap,
  TheRadio,
} from '@/pages';

const INVENTORY_FREQUENCY = 200; // ms

export type { PageConfig, PagesRegistry, CategorySubscriptionConfig } from './types';

export const pagesRegistry: PagesRegistry = {
  character: {
    stats: {
      component: TheStats,
      subscriptions: [
        {
          id: 'character.stats',
          fields: {
            health: 'ActorValue::kHealth',
            ap: 'ActorValue::kActionPoints',
            rads: 'ActorValue::kRadiationRads',
            healthBase: 'ActorValue::kHealth::Base',
            apBase: 'ActorValue::kActionPoints::Base',
            radsMax: 'ActorValue::kRadiationRads::Max',
            level: 'Player::Level',
            xp: 'Player::XP::Current',
            xpNext: 'Player::XP::Next',
            inventoryWeight: 'Player::InventoryWeight',
            carryWeight: 'Player::CarryWeight',
            caps: 'Inventory::Caps',
            karma: 'Player::Karma',
          },
        },
      ],
    },
  },

  inventory: {
    weapons: {
      component: TheWeapons,
      subscriptions: [
        {
          id: 'inventory.weapons',
          fields: {
            items: 'Inventory::Items::Weapons',
            ammo: 'Inventory::Items::Ammo',
          },
          settings: {
            frequency: INVENTORY_FREQUENCY,
          },
        },
      ],
    },
    apparel: {
      component: TheApparel,
      subscriptions: [
        {
          id: 'inventory.apparel',
          fields: {
            items: 'Inventory::Items::Apparel',
          },
          settings: {
            frequency: INVENTORY_FREQUENCY,
          },
        },
      ],
    },
    food: {
      component: TheFood,
      subscriptions: [
        {
          id: 'inventory.food',
          fields: {
            items: 'Inventory::Items::Food',
          },
          settings: {
            frequency: INVENTORY_FREQUENCY,
          },
        },
      ],
    },
    potions: {
      component: ThePotions,
      subscriptions: [
        {
          id: 'inventory.potions',
          fields: {
            items: 'Inventory::Items::Potions',
          },
          settings: {
            frequency: INVENTORY_FREQUENCY,
          },
        },
      ],
    },
    keys: {
      component: TheKeys,
      subscriptions: [
        {
          id: 'inventory.keys',
          fields: {
            items: 'Inventory::Items::Keys',
          },
          settings: {
            frequency: INVENTORY_FREQUENCY,
          },
        },
      ],
    },
    books: {
      component: TheBooks,
      subscriptions: [
        {
          id: 'inventory.books',
          fields: {
            items: 'Inventory::Items::Books',
          },
          settings: {
            frequency: INVENTORY_FREQUENCY,
          },
        },
      ],
    },
    misc: {
      component: TheMisc,
      subscriptions: [
        {
          id: 'inventory.misc',
          fields: {
            items: 'Inventory::Items::Misc',
            gems: 'Inventory::Items::SoulGems',
          },
          settings: {
            frequency: INVENTORY_FREQUENCY,
          },
        },
      ],
    },
  },

  quests: {
    questsList: {
      component: TheQuests,
      subscriptions: [
        {
          id: 'quests.questsList',
          fields: {
            quests: 'Player::Quests',
          },
          settings: {
            frequency: INVENTORY_FREQUENCY,
          },
        },
      ],
    },
    misc: {
      component: TheBooks,
      subscriptions: [
        {
          id: 'inventory.books',
          fields: {
            items: 'Inventory::Items::Books',
          },
          settings: {
            frequency: INVENTORY_FREQUENCY,
          },
        },
      ],
    },
    radio: {
      component: TheRadio,
      subscriptions: [
        {
          id: 'radio.status',
          fields: {
            on: 'Game::Radio::On',
            station: 'Game::Radio::Station',
          },
          settings: {
            frequency: 1000,
          },
        },
      ],
    },
  },

  map: {
    view: {
      component: TheMap,
      subscriptions: [
        {
          id: 'map.player',
          fields: {
            position: 'Player::Position',
          },
          settings: {
            frequency: 50,
          },
        },
        {
          id: 'map.hotspots',
          fields: {
            hot: 'Map::Markers::Locations',
          },
          settings: {
            frequency: 1000,
          },
        },
        {
          id: 'map.questMarkers',
          fields: {
            marker: 'Map::Markers::Quests',
          },
          settings: {
            frequency: 1000,
          },
        },
      ],
    },
  },
};

/**
 * Tabs that require a separate category subscription to populate their subTabs dynamically.
 * Key is the tab id, value is the subscription configuration.
 */
export const TAB_CATEGORY_SUBSCRIPTIONS: Record<string, CategorySubscriptionConfig> = {
  inventory: {
    subscriptionId: 'inventory.categories',
    fields: {
      categories: 'Inventory::Categories',
    },
  },
};

export function getTabCategorySubscription(tabId: string): CategorySubscriptionConfig | null {
  return TAB_CATEGORY_SUBSCRIPTIONS[tabId] ?? null;
}

export function getPageConfig(tab: string, subTab: string): PageConfig | null {
  return pagesRegistry[tab]?.[subTab] ?? null;
}

/**
 * Resolve all subscriptions a page wants active. Pages declare them via
 * the `subscriptions` array; most have one entry, the map page has several
 * concurrent streams at different frequencies.
 */
export function getPageSubscriptions(tab: string, subTab: string): PageSubscriptionConfig[] {
  return getPageConfig(tab, subTab)?.subscriptions ?? [];
}

export function getPageComponent(tab: string, subTab: string): Component | null {
  return getPageConfig(tab, subTab)?.component ?? null;
}
