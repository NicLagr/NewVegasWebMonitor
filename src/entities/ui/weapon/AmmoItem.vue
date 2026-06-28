<template>
  <inventory-item
    :name="name"
    :quantity="quantity"
    :is-favorite="isFavorite"
    :active="active"
    @click="$emit('click')"
  >
    <template #icon>
      <pipboy-icon
        :icon-path="iconPath"
        :size="28"
      >
        <base-icon
          icon-path="lorc/arrow-cluster.svg"
          :size="28"
        />
      </pipboy-icon>
    </template>
    <template #status>
      <equip-status :is-equipped="isEquipped" />
    </template>
  </inventory-item>
</template>

<script setup lang="ts">
import { InventoryItem, EquipStatus } from '@/shared/ui/items';
import { BaseIcon, PipboyIcon } from '@/shared/ui';

defineProps<{
  name: string;
  iconPath?: string;
  quantity?: number;
  isFavorite?: boolean;
  isEquipped?: boolean;
  active?: boolean;
}>();

defineEmits<{
  click: [];
}>();
</script>
