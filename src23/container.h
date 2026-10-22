#ifndef YENDOR23_CONTAINER_H
#define YENDOR23_CONTAINER_H

#include <stdbool.h>
#include <stdint.h>

#include "savegame.h"

/*
 * The records behind the bags, boxes and backpacks. A container item's extra word is the number of its record in the save's item-instance table (section 3: 1296 records of 34 bytes, an inventory
 * group: a u16 weight of what is inside and eight slots). The records are handed out by two header words: the first never-used record number (word_36E0D, header +0x1AE: 2 at a new game in
 * Chapter 2, 3 in Chapter 3) and the head of the list of records that were let go (word_36E0F, +0x1B0: 0 = none) -- a free record's first word holds the next free one.
 *
 *   containerAllocate   LoadNextContainerInChain (yendor2.asm:39144, Chapter 3 :39439): a container that is put down with no record of its own (extra 0: one taken from a chest) gets the head of the
 *                       free list, else the next never-used number; the record is cleared. A number past the table is returned all the same (nothing is written there).
 *   containerFree       PrepareGroundItemSlotWrite (:18049): the record is cleared, its first word set to the old head, and it becomes the head.
 *   containerWeight     The weight of a container's contents: the record's first word (0 for record 0 -- an item without a record -- or a number past the table).
 */
enum { ContainerHeaderNextRecord = 0x1AE, ContainerHeaderFreeHead = 0x1B0, ContainerRecordCount = 1296 };

unsigned containerAllocate(SaveGame *save);
void containerFree(SaveGame *save, unsigned record);
unsigned containerWeight(SaveGame *save, unsigned record);

#endif
