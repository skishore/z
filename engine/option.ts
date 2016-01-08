// When calculating pathfinding, how much it costs to move one step on
// an open floor tile.
export const kAStarFloorCost = 10;

// When applying the pathfinding heuristic, straight steps (NSEW) are
// considered a little cheaper than diagonal ones so that straighter paths
// are preferred over equivalent but uglier zig-zagging ones.
export const kAStarStraightCost = 9;

/// When calculating pathfinding, how much it costs to move one step on a
/// tile already occupied by an actor. For pathfinding, we consider occupied
/// tiles as accessible but expensive. The idea is that by the time the
/// pathfinding monster gets there, the occupier may have moved, so the tile
/// is "sorta" empty, but still not as desirable as an actually empty tile.
export const kAStarOccupiedCost = 60;

/// When calculating pathfinding, how much it costs cross a currently-closed
/// door. Instead of considering them completely impassable, we just have them
/// be expensive, because it still may be beneficial for the monster to get
/// closer to the door (for when the hero opens it later).
export const kAStarDoorCost = 80;
