import { InventoryItem } from '@/stores/inventory/types';
import { QuestListEntry } from '@/stores/quests/types';

export interface CategoryItem {
  categoryId: string;
  count: number;
  name: string;
}

export interface CategoriesData {
  categories: CategoryItem[];
}

export type ListItem = InventoryItem | QuestListEntry;