// Creature statistics read from GAME.EXE (the code segment table that starts with the movement class types: types, then columns of strength, dexterity,
// intelligence, experience, damage and hit points; alignment in the top bits of the column at +0x202; the flags are bits of the movement class words at +0x242, so
// the movement flags and these are the same data). Fields: object type, strength, dexterity, intelligence, hit points, damage, alignment (0 good, 1 neutral,
// 2 evil, 3 chaotic), flags (bit n = flag n + 7: 7 can talk, 8 bleeds, 9 fades away, 12 lives in water, 13 immune to tremors, 14 repels undead, 15 poisonous,
// 16 strength based, 17 double fire damage, 18 immune to magic, 19 immune to poison, 20 undead, 21 immune to sleep), experience.
static const CreatureStats CREATURES[] = {
	{364, 5, 5, 2, 10, 1, 3, 0x4404, 0}, // Acid Slug
	{429, 20, 10, 3, 30, 10, 3, 0x120A, 6}, // Alligator
	{427, 16, 15, 2, 10, 8, 3, 0x420A, 3}, // Ant, Giant
	{410, 25, 25, 25, 30, 4, 0, 0x203, 0}, // Avatar
	{344, 5, 25, 4, 5, 6, 3, 0x4A, 0}, // Bat, Giant
	{384, 8, 8, 9, 6, 3, 3, 0x203, 0}, // Peasant
	{356, 1, 28, 4, 3, 1, 1, 0x4A, 0}, // Bird
	{381, 8, 8, 9, 6, 2, 3, 0x203, 0}, // Child
	{357, 17, 6, 3, 40, 15, 3, 0x4408, 0}, // Corpser
	{428, 14, 7, 5, 10, 3, 1, 0x20A, 0}, // Cow
	{424, 27, 8, 7, 40, 20, 2, 0x202, 4}, // Cyclops
	{367, 35, 26, 31, 100, 20, 2, 0x6AC2, 10}, // Daemon
	{350, 11, 18, 5, 8, 2, 1, 0x2, 0}, // Deer
	{349, 5, 16, 8, 6, 3, 1, 0x2, 0}, // Dog
	{411, 40, 35, 27, 150, 30, 2, 0x844, 12}, // Dragon
	{369, 22, 22, 13, 50, 10, 2, 0x842, 4}, // Drake
	{387, 10, 14, 15, 8, 4, 1, 0x203, 0}, // Woman
	{385, 15, 15, 10, 8, 4, 1, 0x203, 0}, // Farmer
	{376, 20, 17, 11, 20, 6, 3, 0x203, 0}, // Fighter
	{362, 15, 32, 33, 50, 6, 2, 0x4242, 5}, // Gargoyle, Winged
	{363, 25, 19, 8, 30, 15, 2, 0x202, 10}, // Gargoyle, Worker
	{355, 2, 10, 21, 20, 1, 2, 0x4244, 0}, // Gazer
	{352, 1, 10, 9, 20, 12, 2, 0x7894, 0}, // Ghost
	{353, 2, 26, 7, 5, 1, 3, 0x8, 0}, // Gremlin
	{382, 30, 30, 9, 100, 8, 2, 0x203, 0}, // Guard
	{370, 19, 13, 8, 20, 12, 2, 0x202, 2}, // Headless
	{430, 22, 27, 7, 15, 3, 1, 0x20A, 0}, // Horse
	{431, 22, 27, 7, 15, 3, 1, 0x202, 0}, // Horse with rider
	{374, 26, 11, 2, 50, 15, 3, 0x4A04, 0}, // Hydrae
	{343, 1, 40, 1, 1, 1, 3, 0x4048, 0}, // Insects
	{383, 14, 16, 12, 8, 4, 3, 0x203, 0}, // Jester
	{409, 30, 30, 30, 255, 255, 0, 0x203, 30}, // Lord British
	{386, 12, 16, 14, 8, 4, 3, 0x203, 0}, // Musician
	{378, 10, 14, 22, 30, 4, 2, 0x203, 0}, // Mage
	{380, 12, 12, 18, 8, 4, 2, 0x203, 0}, // Merchant
	{98, 22, 9, 8, 30, 15, 3, 0x5200, 5}, // Mimic
	{372, 20, 27, 13, 30, 20, 2, 0x42, 4}, // Mongbat
	{354, 2, 25, 3, 5, 2, 1, 0x2, 0}, // Mouse
	{359, 3, 9, 3, 5, 2, 1, 0xA, 0}, // Rabbit
	{342, 5, 13, 4, 8, 6, 3, 0x110A, 0}, // Rat, Giant
	{347, 21, 20, 16, 30, 20, 3, 0x4600, 4}, // Reaper
	{360, 3, 17, 2, 5, 2, 3, 0x5508, 0}, // Rot Worm
	{426, 13, 13, 3, 20, 8, 3, 0x410A, 3}, // Scorpion, Giant
	{346, 17, 17, 9, 70, 30, 2, 0x28, 2}, // Sea Serpent
	{348, 5, 10, 4, 6, 2, 1, 0x2, 0}, // Sheep
	{412, 30, 30, 30, 30, 30, 3, 0x20, 30}, // Ship
	{413, 60, 21, 8, 200, 60, 3, 0x104, 15}, // Silver Serpent
	{368, 10, 14, 7, 20, 6, 2, 0x7A80, 0}, // Skeleton
	{375, 6, 6, 2, 15, 4, 3, 0x4504, 0}, // Slime
	{358, 3, 7, 6, 5, 2, 3, 0x102, 1}, // Snake
	{361, 9, 11, 4, 10, 8, 3, 0x102, 0}, // Spider, Giant
	{345, 24, 20, 8, 50, 20, 2, 0x4128, 0}, // Squid, Giant
	{377, 15, 18, 12, 15, 4, 3, 0x203, 0}, // Swashbuckler
	{366, 20, 25, 1, 30, 1, 3, 0x4600, 4}, // Tangle Vine
	{371, 18, 14, 9, 15, 6, 2, 0x202, 4}, // Troll
	{379, 11, 12, 12, 8, 4, 3, 0x203, 0}, // Villager
	{373, 1, 40, 20, 40, 1, 3, 0x6A14, 0}, // Wisp
	{351, 8, 16, 8, 8, 4, 3, 0x2, 0}, // Wolf
	{365, 20, 5, 1, 30, 1, 3, 0x4600, 4}, // TV Flower
	{414, 20, 20, 20, 20, 20, 3, 0x20, 20}, // Skiff
	{415, 10, 10, 10, 10, 10, 3, 0x20, 10}, // Raft
	{423, 10, 10, 10, 10, 10, 3, 0x40, 10}, // Balloon
	{388, 4, 18, 8, 6, 2, 1, 0x2, 0}, // Cat
	{263, 11, 12, 12, 8, 4, 3, 0x8, 0}, // Sinjen (Yew Stocks guy)
};

// verified identical to GAME.EXE ds:1B0B (types) and ds:1B55 (damage)
static const struct { int type, damage; } WEAPON_DAMAGE[] = {
	{4, 4}, // spiked helm
	{33, 6}, // sling
	{34, 8}, // club
	{35, 8}, // main gauche
	{36, 10}, // spear
	{37, 10}, // throwing axe
	{38, 6}, // dagger
	{39, 15}, // mace
	{40, 15}, // morning star
	{41, 10}, // bow
	{42, 12}, // crossbow
	{43, 15}, // sword
	{44, 20}, // two-handed hammer
	{45, 20}, // two-handed axe
	{46, 20}, // two-handed sword
	{47, 30}, // halberd
	{49, 8}, // boomerang
	{50, 12}, // triple crossbow
	{48, 255}, // glass sword
	{13, 4}, // spiked shield
	{221, 90}, // cannon
	{83, 4}, // flask of oil
	{54, 20}, // magic bow
	{109, 2}, // rolling pin
	{100, 6}, // scythe
	{101, 4}, // pitchfork
	{103, 4}, // pick
	{104, 4}, // shovel
	{105, 4}, // hoe
	{113, 4}, // cleaver
	{114, 4}, // knife
	{141, 4}, // decorative sword
	{412, 30}, // ship
	{91, 0}, // Zu Ylem
	{78, 4}, // staff
	{79, 30}, // lightning wand
	{80, 20}, // fire wand
};
// verified identical to GAME.EXE ds:1A87 (types) and ds:1ABB (armour class)
static const struct { int type, armour; } ARMOUR[] = {
	{1, 1}, // leather helm
	{2, 2}, // chain coif
	{3, 3}, // iron helm
	{4, 3}, // spiked helm
	{5, 2}, // winged helm
	{6, 2}, // brass helm
	{7, 3}, // Gargoyle Helm
	{8, 5}, // magic helm
	{9, 2}, // wooden shield
	{10, 3}, // curved heater
	{11, 3}, // winged shield
	{12, 3}, // kite shield
	{13, 2}, // spiked shield
	{14, 2}, // black shield
	{15, 4}, // door shield
	{16, 5}, // magic shield
	{17, 1}, // cloth armour
	{18, 2}, // leather armor
	{19, 3}, // ring mail
	{20, 4}, // scale mail
	{21, 5}, // chain mail
	{22, 7}, // plate mail
	{23, 10}, // magic armour
	{35, 1}, // main gauche
	{24, 2}, // spiked collar
	{256, 5}, // protection ring
};
