<template>
  <base-preview
    :data="data"
    :stats="stats"
  >
    <template #icon>
      <base-icon
        icon-path="lorc/swap-bag.svg"
        :size="48"
      />
    </template>
  </base-preview>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { useI18n } from 'vue-i18n';
import { BasePreview } from '@/shared/ui/items';
import { BaseIcon } from '@/shared/ui';
import { getStackWeight, getStackValue } from '@/shared/lib/utils/getDescriptionValues';
import type { MiscItem } from '@/stores/inventory/types';

const { t } = useI18n();

const props = defineProps<{
  data: MiscItem;
}>();

const stats = computed(() => [
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
