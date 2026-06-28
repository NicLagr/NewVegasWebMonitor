<template>
  <div
    class="inv-item"
    :class="{ 'inv-item--favorite': isFavorite, 'inv-item--active': active }"
  >
    <!-- Optional leading icon (per-item Pip-Boy icon w/ type fallback) -->
    <div
      v-if="$slots.icon"
      class="inv-icon"
    >
      <slot name="icon" />
    </div>

    <!-- Main content -->
    <div class="inv-info">
      <div class="inv-row">
        <span class="inv-name">{{ name }}</span>
        <!-- Status indicator slot -->
        <div class="inv-status">
          <slot name="status" />
        </div>
      </div>
      <slot name="description" />
    </div>

    <!-- Quantity -->
    <div
      v-if="quantity && quantity > 1"
      class="inv-quantity"
    >
      {{ quantity }}
    </div>
  </div>
</template>

<script setup lang="ts">
defineProps<{
  name: string;
  quantity?: number;
  isFavorite?: boolean;
  active?: boolean;
}>();
</script>

<style scoped lang="scss">
.inv-item {
  position: relative;
  display: flex;
  align-items: center;
  gap: var(--spacing-sm);
  background-color: var(--pip-bg-light);
  border: 1px solid var(--pip-border-dark);
  cursor: pointer;
  transition: all var(--transition-fast);
  padding: 0;

  &.inv-item--favorite {
    box-shadow: inset 3px 0 0 0 var(--pip-accent-gold);
  }

  &.inv-item--active {
    background-color: var(--tab-bg-active);
    border-color: var(--pip-accent-gold-dim);

    .inv-name {
      color: var(--pip-text-primary);
    }
  }
}

.inv-icon {
  flex: 0 0 auto;
  display: flex;
  align-items: center;
  justify-content: center;
  width: 28px;
  height: 28px;
  margin-left: var(--spacing-sm);
}

/* Tighten the text inset when a leading icon is present. */
.inv-item:has(.inv-icon) .inv-info {
  padding-left: var(--spacing-sm);
}

.inv-status {
  display: flex;
  align-items: center;
  justify-content: start;
  flex-shrink: 0;
  width: 32px;
  height: 32px;
}

.inv-info {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 2px;
  min-width: 0;
  padding-left: var(--spacing-md);
}

.inv-row {
  display: flex;
  flex: 1;
  align-items: center;
  gap: var(--spacing-sm);
  min-width: 0;
}

.inv-name {
  font-family: var(--font-heading);
  font-size: var(--font-size-base);
  color: var(--pip-text-secondary);
  word-break: break-word;
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.inv-icon {
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  width: 32px;
  height: 32px;
}

.inv-quantity {
  font-family: var(--font-heading);
  font-size: var(--font-size-base);
  font-weight: var(--font-weight-semibold);
  color: var(--pip-text-accent);
  padding-right: var(--spacing-sm);
  flex-shrink: 0;
  min-width: 2rem;
  text-align: right;
}

.inv-favorite-indicator {
  position: absolute;
  right: 2px;
  top: 2px;
  width: 4px;
  height: 4px;
  background-color: var(--pip-accent-gold);
  border-radius: 1px;
}
</style>
