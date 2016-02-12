import {Array2D} from '../piecemeal/array2d';
import {Direction} from '../piecemeal/direction';
import {Nil, nil} from '../piecemeal/nil';
import {rng} from '../piecemeal/rng';
import {Vec} from '../piecemeal/vec';

import {Flow} from './ai/flow';

// import {Actor} from './actor';
// import {Breed} from './breed';
// import {Fov} from './fov';
// import {Game} from './game';
// import {Item} from './item';
type Actor = any;
type Breed = any;
type Fov = any;
type Game = any;
type Item = any;
declare function FovConstructor(): Fov;

/// The game's live play area.
export class Stage {
  game: Game;
  tiles: Array2D<Tile>;
  items = new Array<Item>();

  private _actors = new Array<Actor>();
  private _actorAtTile = {};
  private _currentActorIndex = 0;
  private _fov: Fov;
  private _heroPaths: Flow;
  private _visibilityDirty = true;

  get size() { return this.tiles.size; }
  get bounds() { return this.tiles.bounds; }

  get actors() { return this._actors; }
  get currentActor() {return this._actors[this._currentActorIndex]; }

  Stage(size: Vec, game: Game) {
    this.game = game;
    this.tiles = Array2D.generated(size, () => new Tile());
    this._fov = FovConstructor(); // new Fov(game);
  }

  getTile(pos: Vec) { return this.tiles.get(pos); }

  setTile(pos: Vec, tile: Tile) {
    this.tiles.set(pos, tile);
    this.dirtyVisibility();
  }

  addActor(actor: Actor) {
    this._actors.push(actor);
    this._actorAtTile[actor.pos.hash] = actor;
  }

  // Called when an [Actor]'s position has changed so the stage can track it.
  moveActor(from: Vec, to: Vec) {
    if (!this._actorAtTile[from.hash] || this._actorAtTile[to.hash]) {
      throw `Illegal move from ${from} to ${to}.`;
    }
    this._actorAtTile[from.hash] = this._actorAtTile[to.hash];
    delete this._actorAtTile[from.hash];
  }

  removeActor(actor: Actor) {
    if (this._actorAtTile[actor.pos.hash] !== actor) {
      throw `${actor} was not at ${actor.pos}.`;
    }
    const index = this._actors.indexOf(actor);
    if (this._currentActorIndex > index) this._currentActorIndex--;
    this._actors.splice(index, 1);
    delete this._actorAtTile[actor.pos.hash];
  }

  advanceActor() {
    this._currentActorIndex =
        (this._currentActorIndex + 1) % this._actors.length;
  }

  actorAt(pos: Vec): Actor|Nil {
    const result = this._actorAtTile[pos.hash];
    return result instanceof Actor ? result : nil;
  }

  dirtyVisibility() {
    this._visibilityDirty = true;
  }

  refreshVisibility(hero: Actor) {
    if (this._visibilityDirty) {
      this._fov.refresh(hero.pos);
      this._visibilityDirty = false;
    }
  }

  // TODO(skishore): This is hackish and may fail to terminate.
  /// Selects a random passable tile that does not have an [Actor] on it.
  findOpenTile() {
    while (true) {
      const pos = rng.vecInRect(this.bounds);
      if (!this.getTile(pos).passable) continue;
      if (this.actorAt(pos) instanceof Actor) continue;
      return pos;
    }
  }

  /// Gets the number of tiles to walk from [pos] to the [Hero]'s current
  /// position taking into account which tiles are traversable.
  getHeroDistanceTo(pos: Vec) {
    this._refreshDistances();
    return this._heroPaths.getDistance(pos);
  }

  /// Randomly selects an open tile in the stage. Makes [tries] attempts and
  /// chooses the one most distance from some point. Assumes that [scent2] has
  /// been filled with the distance information for the target point.
  ///
  /// This is used during level creation to place stronger [Monster]s and
  /// better treasure farther from the [Hero]'s starting location.
  findDistantOpenTile(tries: number) {
    this._refreshDistances();
    let bestDistance = -1;
    let best: Vec;
    for (let i = 0; i < tries; i++) {
      const pos = this.findOpenTile();
      const distance = this._heroPaths.getDistance(pos);
      if (!(distance instanceof Nil) && distance > bestDistance) {
        [best, bestDistance] = [pos, <number>distance];
      }
    }
    return best;
  }

  spawnMonster(breed: Breed, pos: Vec) {
    const monsters = new Array<Actor>();
    /* tslint:disable:no-bitwise */
    const count = rng.triangle(
        breed.numberInGroup, (breed.numberInGroup / 2) | 0);
    /* tslint:enable */

    const addMonster = (target: Vec) => {
      const monster = <Actor>breed.spawn(game, target);
      this.addActor(monster);
      monsters.push(monster);
    };

    // Place the first monster.
    addMonster(pos);

    // If the monster appears in groups, place the rest of the groups.
    for (let i = 1; i < count; i++) {
      // Find every open tile that's neighboring a monster in the group.
      const open = new Array<Vec>();
      for (const monster of monsters) {
        for (const dir of Direction.all) {
          const neighbor = monster.pos.add(dir);
          if (this.getTile(neighbor).passable &&
              this.actorAt(neighbor) instanceof Nil) {
            open.push(neighbor);
          }
        }
      }

      if (open.length === 0) {
        // We filled the entire reachable area with monsters, so give up.
        break;
      }
      addMonster(rng.item(open));
    }
  }

  // Lazily calculates the paths from every reachable tile to the [Hero]. We
  // use this to place better and stronger things farther from the Hero. Sound
  // propagation is also based on this.
  private _refreshDistances() {
    if (this._heroPaths !== undefined &&
        this.game.hero.pos.equals(this._heroPaths.start)) return;
    this._heroPaths = new Flow(
        this, this.game.hero.pos, {canOpenDoors: true, ignoreActors: true});
  }
}

export class Tile {
  get type() { return this._type; }
  get visible() { return this._visible; }
  get explored() { return this._explored; }

  set visible(visible: boolean) {
    this._visible = visible;
    this._explored = this._explored || visible;
  }

  private _type: TileType;
  private _visible: boolean  = false;
  private _explored: boolean  = false;

  get passable() { return this._type.passable; }
  get transparent() { return this._type.transparent; }
  get traversable() { return this._type.passable || this._type.opens; }
}

export class TileType {
  get name() { return this._name; }
  get passable() { return this._passable; }
  get transparent() { return this._transparent; }
  get appearance() { return this._appearance; }

  get opens() { return this.opensTo instanceof TileType; }

  opensTo: TileType|Nil = nil;
  closesTo: TileType|Nil = nil;

  constructor(private _name: string, private _passable: boolean,
              private _transparent: boolean, private _appearance: any) {}
}
