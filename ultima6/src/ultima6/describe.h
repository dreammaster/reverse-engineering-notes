#ifndef ULTIMA6_ULTIMA6_DESCRIBE_H
#define ULTIMA6_ULTIMA6_DESCRIBE_H

#include "ultima6/world.h"

#include <string>

namespace Ultima6 {
namespace Ultima6 {

/**
 * How the game words what is on the map (get_object_name, sub_2FED1, sub_2FCAE): the description of a tile from
 * LOOK.LZD with its plural or singular ending chosen, an article ("a ", "an ", "the " from the two top bits of TILEFLAG's
 * fourth block, 0x40 / 0x80 / 0xC0) or a count in front.
 */
namespace Describe {

/** The tile's name with the `\plural` / `/singular` endings resolved. */
std::string tileName(const World &world, int tile, bool plural);
std::string article(const World &world, int tile);
/** The names of the 21 object types that stack (sub_1B687): 1 = a word quantity (gold), 2 = a byte quantity, 0 = not stackable. */
int quantityKind(const World &world, int type);

/** "a torch", "3 torches", "the Avatar": the prefix and the name. */
std::string object(const World &world, const World::Obj &o);

} // namespace Describe

} // namespace Ultima6
} // namespace Ultima6

#endif
