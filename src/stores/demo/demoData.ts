/**
 * Mock dataset for Demo Mode — lets the app be explored without a running game
 * (e.g. for portfolio visitors). The keys are the same subscription IDs the
 * live WebSocket backend sends, so the data flows through the normal DataRouter
 * → stores path with zero special-casing. Marker coordinates are real Mojave
 * world coords so they land correctly on the calibrated map.
 */
const W = 'interface/icons/pipboyimages/weapons/';
const I = 'interface/icons/pipboyimages/items/';
const A = 'interface/icons/pipboyimages/apparel/';

export function buildDemoData(): Record<string, Record<string, unknown>> {
  return {
    system: {
      features: ['player', 'player.quests', 'inventory', 'map', 'game.radio'],
      language: 'en',
    },
    'game.status': {
      status: {
        canAct: true,
        controlsEnabled: true,
        dead: false,
        inCombat: false,
        inDialogue: false,
        inMainMenu: false,
        loading: false,
        paused: false,
      },
    },
    'character.stats': {
      level: 14,
      xp: 18400,
      xpNext: 21000,
      caps: 2375,
      karma: 250,
      health: 285,
      healthBase: 340,
      ap: 95,
      apBase: 110,
      rads: 60,
      radsMax: 1000,
      carryWeight: 210,
      inventoryWeight: 168,
      special: { strength: 6, perception: 7, endurance: 5, charisma: 4, intelligence: 8, agility: 6, luck: 5 },
      limbs: { head: 100, torso: 82, leftArm: 64, rightArm: 100, leftLeg: 41, rightLeg: 90 },
      effects: [
        { name: 'Buffout', hidden: false },
        { name: 'Well Rested', hidden: false },
        { name: 'Four Eyes', hidden: true },
      ],
    },
    'inventory.categories': {
      categories: [
        { categoryId: 'WEAPONS', name: 'Weapons' },
        { categoryId: 'APPAREL', name: 'Apparel' },
        { categoryId: 'AID', name: 'Aid' },
        { categoryId: 'NOTES', name: 'Notes' },
        { categoryId: 'MISC', name: 'Misc' },
      ],
    },
    'inventory.weapons': {
      items: [
        { formId: '0x0010A001', categoryType: 'Weapon', name: '10mm Pistol', count: 1, value: 250, weight: 3, iconPath: W + 'weapons_10mm_pistol', weaponType: 'OneHandSword', damage: 16, condition: 88, isEquipped: true, equippedHand: 'right' },
        { formId: '0x0010A002', categoryType: 'Weapon', name: '9mm Pistol', count: 1, value: 89, weight: 3, iconPath: W + 'weapons_9mm_pistol', weaponType: 'OneHandSword', damage: 13, condition: 54, isEquipped: false },
        { formId: '0x0010A003', categoryType: 'Weapon', name: 'Combat Knife', count: 1, value: 40, weight: 1, iconPath: W + 'weapons_combat_knife', weaponType: 'OneHandAxe', damage: 14, condition: 100, isEquipped: false },
        { formId: '0x0010A004', categoryType: 'Weapon', name: 'Hunting Rifle', count: 1, value: 300, weight: 7.5, iconPath: W + 'weapons_hunting_rifle', weaponType: 'TwoHandSword', damage: 55, condition: 72, isEquipped: false },
        { formId: '0x0010A005', categoryType: 'Weapon', name: 'Frag Grenade', count: 4, value: 25, weight: 0.5, iconPath: W + 'weapons_frag_grenade', weaponType: 'HandToHandMelee', damage: 50, condition: 100, isEquipped: false },
      ],
      ammo: [
        { formId: '0x0010B001', categoryType: 'Ammo', name: '10mm Round', count: 142, value: 1, weight: 0, iconPath: I + 'items_10mm', isEquipped: true },
        { formId: '0x0010B002', categoryType: 'Ammo', name: '.308 Round', count: 36, value: 3, weight: 0, iconPath: I + 'items_308' },
      ],
    },
    'inventory.apparel': {
      items: [
        { formId: '0x0020A001', categoryType: 'Apparel', name: 'Leather Armor', count: 1, value: 160, weight: 15, iconPath: A + 'apparel_leather_armor', armorType: 'Light', bodySlots: [], damageThreshold: 8, condition: 76, isEquipped: true },
        { formId: '0x0020A002', categoryType: 'Apparel', name: 'NCR Trooper Armor', count: 1, value: 330, weight: 18, iconPath: A + 'apparel_ncr_trooper_armor', armorType: 'Medium', bodySlots: [], damageThreshold: 13, condition: 61, isEquipped: false },
        { formId: '0x0020A003', categoryType: 'Apparel', name: 'Authority Glasses', count: 1, value: 50, weight: 0, iconPath: A + 'apparel_authority_glasses', armorType: 'Light', bodySlots: [], damageThreshold: 0, condition: 100, isEquipped: true },
      ],
    },
    'inventory.potions': {
      items: [
        { formId: '0x0030A001', categoryType: 'Potion', name: 'Stimpak', count: 8, value: 75, weight: 0, iconPath: I + 'item_super_stimpak', effects: [{ name: 'Restore Health', magnitude: 30, duration: 0, description: 'Restore Health 30', descriptionTemplate: 'Restore Health 30' }] },
        { formId: '0x0030A002', categoryType: 'Potion', name: 'RadAway', count: 3, value: 100, weight: 0, iconPath: I + 'items_radaway', effects: [{ name: 'Remove Radiation', magnitude: 150, duration: 0, description: 'Remove Radiation 150', descriptionTemplate: 'Remove Radiation 150' }] },
        { formId: '0x0030A003', categoryType: 'Potion', name: 'Buffout', count: 2, value: 60, weight: 0, iconPath: I + 'items_buffout', effects: [{ name: 'Strength', magnitude: 2, duration: 240, description: 'Strength 2 (240s)', descriptionTemplate: 'Strength 2 (240s)' }, { name: 'Endurance', magnitude: 3, duration: 240, description: 'Endurance 3 (240s)', descriptionTemplate: 'Endurance 3 (240s)' }] },
        { formId: '0x0030A004', categoryType: 'Potion', name: 'Mentats', count: 1, value: 50, weight: 0, iconPath: I + 'items_mentats', effects: [{ name: 'Intelligence', magnitude: 2, duration: 300, description: 'Intelligence 2 (300s)', descriptionTemplate: 'Intelligence 2 (300s)' }, { name: 'Perception', magnitude: 2, duration: 300, description: 'Perception 2 (300s)', descriptionTemplate: 'Perception 2 (300s)' }] },
        { formId: '0x0030A005', categoryType: 'Potion', name: 'Sunset Sarsaparilla', count: 5, value: 5, weight: 1, iconPath: I + 'item_sunset_sarsaparilla', effects: [{ name: 'Restore Health', magnitude: 10, duration: 0, description: 'Restore Health 10', descriptionTemplate: 'Restore Health 10' }] },
      ],
    },
    'inventory.books': {
      items: [
        { formId: '0x0040A001', categoryType: 'Book', name: "Doc Mitchell's Note", count: 1, value: 0, weight: 0, iconPath: I + 'item_note', description: 'Welcome back to the land of the living. Take it easy for a while — you took a bullet to the head. Come find me if anything feels off.' },
        { formId: '0x0040A002', categoryType: 'Book', name: 'Lottery Ticket', count: 1, value: 0, weight: 0, iconPath: I + 'item_note', description: 'Nipton Lottery — you are a WINNER! Collect your prize at the town hall.' },
      ],
    },
    'inventory.misc': {
      items: [
        { formId: '0x0050A001', categoryType: 'Misc', name: 'Scrap Metal', count: 12, value: 4, weight: 1, iconPath: I + 'items_scrap_metal' },
        { formId: '0x0050A002', categoryType: 'Misc', name: 'Wonderglue', count: 3, value: 10, weight: 0.5, iconPath: I + 'items_wonderglue' },
        { formId: '0x0050A003', categoryType: 'Misc', name: 'Sensor Module', count: 2, value: 30, weight: 1, iconPath: I + 'items_sensor_module' },
        { formId: '0x0050A004', categoryType: 'Misc', name: 'Sunset Sarsaparilla Star Cap', count: 18, value: 0, weight: 0, iconPath: I + 'items_sunset_sarsaparilla_star_bottle_cap' },
      ],
    },
    'map.player': {
      position: { x: -67900, y: 2234, z: 0, angle: 45, cell: 'Goodsprings', isInterior: false, worldspace: 'WastelandNV' },
    },
    'map.hotspots': {
      hot: [
        { type: 'Town', typeId: 1, refId: '0x000DC983', name: 'Goodsprings', x: -67900, y: 2234, isVisible: true, canFastTravel: true },
        { type: 'Town', typeId: 1, refId: '0x0010C0B0', name: 'Primm', x: -56752, y: -52992, isVisible: true, canFastTravel: true },
        { type: 'Town', typeId: 1, refId: '0x0010C0B1', name: 'Novac', x: 22205, y: -32624, isVisible: true, canFastTravel: true },
        { type: 'Town', typeId: 1, refId: '0x0010C0B2', name: 'Nipton', x: -32672, y: -93824, isVisible: true, canFastTravel: true },
        { type: 'Settlement', typeId: 2, refId: '0x0010C0B3', name: 'Mojave Outpost', x: -83001, y: -101871, isVisible: true, canFastTravel: true },
        { type: 'Settlement', typeId: 2, refId: '0x0010C0B4', name: 'REPCONN Headquarters', x: 1468, y: 41852, isVisible: true, canFastTravel: true },
        { type: 'Town', typeId: 1, refId: '0x0010C0B5', name: "Freeside's North Gate", x: -9670, y: 111810, isVisible: true, canFastTravel: true },
        { type: 'Settlement', typeId: 2, refId: '0x0010C0B6', name: 'Camp McCarran', x: -19736, y: 75896, isVisible: true, canFastTravel: true },
      ],
    },
    'quests.questsList': {
      quests: [
        {
          type: 'quest', formId: '0x0014E456', questFormId: '0x0014E456', questEditorId: 'VMS01',
          name: 'Ain’t That a Kick in the Head', nameRaw: 'Ain’t That a Kick in the Head',
          description: 'Recover from your injuries with help from Doc Mitchell, then set out after the man who shot you.',
          descriptionRaw: '', descriptionStage: 10, questType: 'Main', isMisc: false,
          isActive: true, isRunning: true, isCompleted: false, currentStage: 10, currentInstanceId: 1,
          steps: [
            { index: 0, text: 'Get patched up by Doc Mitchell', textRaw: '', completed: true, failed: false, state: 'completed', stateRaw: 2, instanceId: 1 },
            { index: 1, text: 'Track down the man who shot you', textRaw: '', completed: false, failed: false, state: 'active', stateRaw: 1, instanceId: 2 },
          ],
        },
        {
          type: 'quest', formId: '0x0015A123', questFormId: '0x0015A123', questEditorId: 'VMS03',
          name: 'They Went That-a-Way', nameRaw: 'They Went That-a-Way',
          description: 'Follow the trail of the Platinum Chip’s courier through Primm, Nipton and Novac toward New Vegas.',
          descriptionRaw: '', descriptionStage: 20, questType: 'Main', isMisc: false,
          isActive: false, isRunning: true, isCompleted: false, currentStage: 20, currentInstanceId: 1,
          steps: [
            { index: 0, text: 'Ask about the man in the checkered suit', textRaw: '', completed: true, failed: false, state: 'completed', stateRaw: 2, instanceId: 1 },
            { index: 1, text: 'Reach Boulder City', textRaw: '', completed: false, failed: false, state: 'active', stateRaw: 1, instanceId: 2 },
          ],
        },
      ],
    },
    'radio.status': {
      on: true,
      currentRefId: '0x0010F001',
      stations: [
        { refId: '0x0010F001', name: 'Radio New Vegas', receivable: true },
        { refId: '0x0010F002', name: 'Mojave Music Radio', receivable: true },
        { refId: '0x0010F003', name: 'Black Mountain Radio', receivable: true },
      ],
    },
  };
}

export default buildDemoData;
