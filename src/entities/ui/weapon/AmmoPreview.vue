<template>
  <base-preview
    :data="data"
    :stats="stats"
  >
    <template #icon>
      <pipboy-icon
        :icon-path="data?.iconPath"
        :size="96"
      >
        <base-icon
          icon-path="lorc/arrow-cluster.svg"
          :size="96"
        />
      </pipboy-icon>
    </template>
  </base-preview>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { useI18n } from 'vue-i18n';
import { BasePreview } from '@/shared/ui/items';
import { BaseIcon, PipboyIcon } from '@/shared/ui';
import { getRoundValue, getStackWeight, getStackValue } from '@/shared/lib/utils/getDescriptionValues';
import type { AmmoItem } from '@/stores/inventory/types';

const { t } = useI18n();

const props = withDefaults(
  defineProps<{
    data?: AmmoItem | null;
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
    label: t('common.weight'),
    value: getStackWeight(props.data?.weight, props.data?.count),
  },
  {
    label: t('common.value'),
    value: getStackValue(props.data?.value, props.data?.count),
  },
]);
</script>
