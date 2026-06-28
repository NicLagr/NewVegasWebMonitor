<template>
  <base-preview
    :data="data"
    :stats="stats"
    :effects="data?.enchantment?.effects"
  >
    <template #icon>
      <pipboy-icon
        v-if="data"
        :icon-path="data.iconPath"
        :size="48"
      >
        <weapon-icon
          :weapon-type="data.weaponType"
          :size="48"
        />
      </pipboy-icon>
    </template>
  </base-preview>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { useI18n } from 'vue-i18n';
import { BasePreview } from '@/shared/ui/items';
import { PipboyIcon } from '@/shared/ui';
import { WeaponIcon } from '@/entities/ui';
import { getRoundValue, getStackWeight, getStackValue } from '@/shared/lib/utils/getDescriptionValues';
import type { WeaponItem } from '@/stores/inventory/types';

const { t } = useI18n();

const props = withDefaults(
  defineProps<{
    data?: WeaponItem | null;
  }>(),
  {
    data: null,
  }
);

const stats = computed(() => [
  {
    label: t('pages.inventory.weapons.damage'),
    value: getRoundValue(props.data?.damage),
  },
  {
    label: t('pages.inventory.weapons.condition'),
    value: props.data?.condition != null ? `${getRoundValue(props.data.condition)}%` : '-',
  },
  {
    label: t('pages.inventory.weapons.weight'),
    value: getStackWeight(props.data?.weight, props.data?.count),
  },
  {
    label: t('pages.inventory.weapons.value'),
    value: getStackValue(props.data?.value, props.data?.count),
  },
]);
</script>
