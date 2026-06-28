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
        <apparel-icon
          :body-slots="data.bodySlots"
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
import { ApparelIcon } from '@/entities/ui';
import { getRoundValue, getStackWeight, getStackValue } from '@/shared/lib/utils/getDescriptionValues';
import type { ApparelItem } from '@/stores/inventory/types';

const { t } = useI18n();

const props = withDefaults(
  defineProps<{
    data?: ApparelItem | null;
  }>(),
  {
    data: null,
  }
);

const stats = computed(() => [
  {
    label: t('pages.inventory.apparel.damageThreshold'),
    value: getRoundValue(props.data?.damageThreshold),
  },
  {
    label: t('pages.inventory.apparel.condition'),
    value: props.data?.condition != null ? `${getRoundValue(props.data.condition)}%` : '-',
  },
  {
    label: t('pages.inventory.apparel.weight'),
    value: getStackWeight(props.data?.weight, props.data?.count),
  },
  {
    label: t('pages.inventory.apparel.value'),
    value: getStackValue(props.data?.value, props.data?.count),
  },
]);
</script>
