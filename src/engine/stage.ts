import {Array2D} from '../piecemeal/array2d';
import {Nil, nil} from '../piecemeal/nil';
import {Vec} from '../piecemeal/vec';

export interface IActor {
  position: Vec;
}

export class Stage<Actor extends IActor> {
  tiles: Array2D<Tile>;
  private _actors = new Array<Actor>();
  private _actorAtTile: {[key: string]: Actor} = {};
  private _currentActorIndex = 0;
  private _defaultTileType: TileType;

  get size() { return this.tiles.size; }
  get bounds() { return this.tiles.bounds; }

  get actors() { return this._actors; }
  get currentActor() { return this._actors[this._currentActorIndex]; }

  constructor(size: Vec, defaultTileType: TileType) {
    this.tiles = Array2D.generated(size, () => new Tile(defaultTileType));
    this._defaultTileType = defaultTileType;
  }

  isSquareFree(pos: Vec) {
    return this.getTile(pos).passable && !this._actorAtTile[pos.hash];
  }

  getTile(pos: Vec) {
    return this.size.contains(pos) ? this.tiles.get(pos) :
           new Tile(this._defaultTileType);
  }

  setTile(pos: Vec, tile: Tile) { this.tiles.set(pos, tile); }

  actorAt(pos: Vec): Actor|Nil {
    const result = this._actorAtTile[pos.hash];
    return result ? result : nil;
  }

  addActor(actor: Actor) {
    this._actors.push(actor);
    this._actorAtTile[actor.position.hash] = actor;
  }

  advanceActor() {
    this._currentActorIndex =
        (this._currentActorIndex + 1) % this._actors.length;
  }

  moveActor(from: Vec, to: Vec) {
    if (!this._actorAtTile[from.hash] || this._actorAtTile[to.hash]) {
      throw `Illegal move from ${from} to ${to}.`;
    }
    const actor = this._actorAtTile[from.hash];
    delete this._actorAtTile[from.hash];
    this._actorAtTile[to.hash] = actor;
    actor.position = to;
  }

  removeActor(actor: Actor) {
    if (this._actorAtTile[actor.position.hash] !== actor) {
      throw `${actor} was not at ${actor.position}.`;
    }
    const index = this._actors.indexOf(actor);
    if (this._currentActorIndex > index) this._currentActorIndex--;
    this._actors.splice(index, 1);
    delete this._actorAtTile[actor.position.hash];
  }

  swapActors(from: Vec, to: Vec) {
    if (!this._actorAtTile[from.hash] || !this._actorAtTile[to.hash]) {
      throw `Illegal swap from ${from} to ${to}.`;
    }
    const swap = this._actorAtTile[to.hash];
    this._actorAtTile[to.hash] = this._actorAtTile[from.hash];
    this._actorAtTile[to.hash].position = to;
    this._actorAtTile[from.hash] = swap;
    swap.position = from;
  }
}

export class Tile {
  private _visible: boolean  = false;
  private _explored: boolean  = false;

  get type() { return this._type; }
  get visible() { return this._visible; }
  get explored() { return this._explored; }

  get passable() { return this._type.passable; }
  get transparent() { return this._type.transparent; }
  get traversable() { return this._type.passable || this._type.opens; }

  constructor(private _type: TileType) {}

  set visible(visible: boolean) {
    this._visible = visible;
    this._explored = this._explored || visible;
  }
}

export class TileType {
  opensTo: TileType|Nil = nil;
  closesTo: TileType|Nil = nil;

  get name() { return this._name; }
  get passable() { return this._passable; }
  get transparent() { return this._transparent; }
  get appearance() { return this._appearance; }
  get opens() { return this.opensTo instanceof TileType; }

  constructor(private _name: string, private _passable: boolean,
              private _transparent: boolean, private _appearance: any) {}
}
