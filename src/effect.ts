import {Color, Glyph} from './lib';
import {flatten, int, nonnull, range, sample} from './lib';
import {Point, Direction, LOS} from './geo';

//////////////////////////////////////////////////////////////////////////////

interface Board {
  getGlyphAt: (pos: Point) => Glyph,
  getUnderlyingGlyphAt: (pos: Point) => Glyph,
};

//////////////////////////////////////////////////////////////////////////////

enum FT { Fire, Ice, Hit, Summon, Withdraw };

interface Event { type: FT, point: Point, frame: int };

interface Particle {point: Point, glyph: Glyph};

interface Frame extends Array<Particle> {};

class Effect {
  events: Event[];
  frames: Frame[];

  constructor(frames?: Frame[]) {
    this.events = [];
    this.frames = frames ?? [];
  }

  and(other: Effect): Effect {
    return Effect.Parallel([this, other]);
  }

  then(other: Effect): Effect {
    return Effect.Serial([this, other]);
  }

  delay(n: int): Effect {
    return Effect.Pause(n).then(this);
  }

  scale(s: number): Effect {
    const result = new Effect();
    const {events, frames} = result;

    const source = this.frames.length;
    const target = int(s * source);

    this.events.forEach(x => events.push({...x, frame: int(s * x.frame)}));

    for (let i = 0; i < source; i++) {
      const start = int(s * (i + 0));
      const limit = int(s * (i + 1));
      const frame = nonnull(this.frames[i]);
      for (let j = start; j < limit; j++) frames.push(frame);
    }
    return result;
  }

  mutAddEvent(event: Event): void {
    this.events.push(event);
    this.events.sort((a, b) => a.frame - b.frame);
  }

  mutAddParticle(frame: int, particle: Particle): void {
    while (frame >= this.frames.length) this.frames.push([]);
    nonnull(this.frames[frame]).push(particle);
  }

  static Constant(particle: Particle, n: int): Effect {
    return new Effect(Array(n).fill([particle]));
  }

  static Pause(n: int): Effect {
    return new Effect(Array(n).fill([]));
  }

  static Parallel(effects: Effect[]): Effect {
    const result = new Effect();
    const {events, frames} = result;
    for (const effect of effects) {
      effect.events.forEach(x => events.push(x));
      effect.frames.forEach((x, i) => {
        if (i >= frames.length) frames.push([]);
        x.forEach(y => nonnull(frames[i]).push(y));
      });
    }
    events.sort((a, b) => a.frame - b.frame);
    return result;
  };

  static Serial(effects: Effect[]): Effect {
    let offset: int = 0;
    const result = new Effect();
    const {events, frames} = result;
    for (const effect of effects) {
      effect.events.forEach(x => events.push({...x, frame: int(x.frame + offset)}));
      effect.frames.forEach(x => frames.push(x));
      offset += int(effect.frames.length);
    }
    return result;
  }
};

//////////////////////////////////////////////////////////////////////////////

type Sparkle = [int, string, Color?][];

const add_sparkle =
    (effect: Effect, sparkle: Sparkle, frame: int, point: Point): int => {
  sparkle.forEach(x => {
    const [delay, chars, color] = x;
    for (let i = 0; i < delay; i++, frame++) {
      const index = Math.floor(Math.random() * chars.length);
      const glyph = new Glyph(nonnull(chars[index]), color);
      effect.mutAddParticle(frame, {glyph, point});
    }
  });
  return frame;
};

const random_delay = (n: int): int => {
  let count = int(1);
  const limit = Math.floor(1.5 * n);
  while (count < limit && Math.floor(Math.random() * n)) count++;
  return count;
};

const ray_character = (source: Point, target: Point): string => {
  const dx = target.x - source.x;
  const dy = target.y - source.y;
  if (Math.abs(dx) > 2 * Math.abs(dy)) return '-';
  if (Math.abs(dy) > 2 * Math.abs(dx)) return '|';
  return ((dx > 0) === (dy > 0)) ? '\\' : '/';
};

const OverlayEffect = (effect: Effect, particle: Particle): Effect => {
  const constant = Effect.Constant(particle, int(effect.frames.length));
  return Effect.Parallel([effect, constant]);
};

const UnderlayEffect = (effect: Effect, particle: Particle): Effect => {
  const constant = Effect.Constant(particle, int(effect.frames.length));
  return Effect.Parallel([constant, effect]);
};

const ExplosionEffect = (point: Point): Effect => {
  const color = '400';
  const base = [
    [{point, glyph: new Glyph('*', color)}],
    [
      {point, glyph: new Glyph('+', color)},
      {point: point.add(Direction.n), glyph: new Glyph('|', color)},
      {point: point.add(Direction.s), glyph: new Glyph('|', color)},
      {point: point.add(Direction.e), glyph: new Glyph('-', color)},
      {point: point.add(Direction.w), glyph: new Glyph('-', color)},
    ],
    [
      {point: point.add(Direction.n),  glyph: new Glyph('-', color)},
      {point: point.add(Direction.s),  glyph: new Glyph('-', color)},
      {point: point.add(Direction.e),  glyph: new Glyph('|', color)},
      {point: point.add(Direction.w),  glyph: new Glyph('|', color)},
      {point: point.add(Direction.ne), glyph: new Glyph('\\', color)},
      {point: point.add(Direction.sw), glyph: new Glyph('\\', color)},
      {point: point.add(Direction.nw), glyph: new Glyph('/', color)},
      {point: point.add(Direction.se), glyph: new Glyph('/', color)},
    ],
  ];
  return (new Effect(base)).scale(4);
};

const ImplosionEffect = (point: Point): Effect => {
  const color = '400';
  const base = [
    [
      {point, glyph: new Glyph('*', color)},
      {point: point.add(Direction.ne), glyph: new Glyph('/', color)},
      {point: point.add(Direction.sw), glyph: new Glyph('/', color)},
      {point: point.add(Direction.nw), glyph: new Glyph('\\', color)},
      {point: point.add(Direction.se), glyph: new Glyph('\\', color)},
    ],
    [
      {point, glyph: new Glyph('#', color)},
      {point: point.add(Direction.ne), glyph: new Glyph('/', color)},
      {point: point.add(Direction.sw), glyph: new Glyph('/', color)},
      {point: point.add(Direction.nw), glyph: new Glyph('\\', color)},
      {point: point.add(Direction.se), glyph: new Glyph('\\', color)},
    ],
    [{point, glyph: new Glyph('*', color)}],
    [{point, glyph: new Glyph('#', color)}],
  ];
  return (new Effect(base)).scale(3);
};

const RayEffect = (source: Point, target: Point, speed: int): Effect => {
  const color = '400';
  const result: Frame[] = [];
  const line = LOS(source, target);
  if (line.length <= 2) return new Effect(result);

  const beam = new Glyph(ray_character(source, target), color);
  const mod = int((line.length - 2 + speed) % speed);
  const start = int(mod ? mod : mod + speed);
  for (let i = start; i < line.length - 1; i = int(i + speed)) {
    result.push(range(i).map(j => ({point: nonnull(line[j + 1]), glyph: beam})));
  }
  return new Effect(result);
};

const SummonEffect = (source: Point, target: Point, glyph: Glyph): Effect => {
  const color = '400';
  const base: Frame[] = [];
  const line = LOS(source, target);
  const ball = new Glyph('*', color);
  for (let i = 1; i < line.length - 1; i++) {
    base.push([{point: nonnull(line[i]), glyph: ball}]);
  }
  const masked = UnderlayEffect(new Effect(base), {point: target, glyph});
  return Effect.Serial([masked, ExplosionEffect(target)]);
};

const WithdrawEffect = (source: Point, target: Point, glyph: Glyph): Effect => {
  const base = RayEffect(source, target, 4);
  const hide = {point: target, glyph};
  const impl = UnderlayEffect(ImplosionEffect(target), hide);
  const full = base.frames[base.frames.length - 1];
  if (!full) return impl;

  return Effect.Serial([
    base,
    Effect.Parallel([(new Effect([full])).scale(int(impl.frames.length)), impl]),
    UnderlayEffect(new Effect(base.frames.slice().reverse()), hide),
  ]);
};

const SwitchEffect = (source: Point, target: Point, glyph: Glyph): Effect => {
  return Effect.Serial([
    WithdrawEffect(source, target, glyph),
    Effect.Constant({point: target, glyph}, 4),
    SummonEffect(source, target, glyph),
  ]);
};

const EmberEffect = (board: Board, source: Point, target: Point): Effect => {
  const effect = new Effect();
  const line = LOS(source, target);

  const trail = (): Sparkle => [
    [random_delay(0), '*^^',   '400'],
    [random_delay(1), '*^',    '420'],
    [random_delay(2), '**^',   '440'],
    [random_delay(3), '**^#%', '420'],
    [random_delay(4), '#%',    '400'],
  ];
  const flame = (): Sparkle => [
    [random_delay(0), '*^^',   '400'],
    [random_delay(1), '*^',    '420'],
    [random_delay(2), '**^#%', '440'],
    [random_delay(3), '*^#%',  '420'],
    [random_delay(4), '*^#%',  '400'],
  ];

  for (let i = 1; i < line.length - 1; i++) {
    const frame = int(Math.floor((i - 1) / 2));
    add_sparkle(effect, trail(), frame, nonnull(line[i]));
  }
  let hitFrame: int = 0;
  for (const direction of [Direction.none].concat(Direction.all)) {
    const norm = direction.distanceTaxicab(Direction.none);
    const frame = int(2 * norm + Math.floor((line.length - 1) / 2));
    const finish = add_sparkle(effect, flame(), frame, target.add(direction));
    if (norm === 0) hitFrame = finish;
  }
  effect.mutAddEvent({type: FT.Hit, point: target, frame: hitFrame});
  return effect;
};

const IceBeamEffect = (board: Board, source: Point, target: Point): Effect => {
  const effect = new Effect();
  const line = LOS(source, target);
  const ch = ray_character(source, target);

  const trail: Sparkle = [
    [2, ch, '555'],
    [2, ch, '044'],
    [2, ch, '004'],
  ];
  const flame: Sparkle = [
    [2, '*', '555'],
    [2, '*', '044'],
    [2, '*', '004'],
    [2, '*', '555'],
    [2, '*', '044'],
    [2, '*', '004'],
  ];

  for (let i = 1; i < line.length; i++) {
    const frame = int(Math.floor((i - 1) / 2));
    add_sparkle(effect, trail, frame, nonnull(line[i]));
  }
  const frame = int(Math.floor((line.length - 1) / 2));
  const hitFrame = add_sparkle(effect, flame, frame, target);
  effect.mutAddEvent({type: FT.Hit, point: target, frame: hitFrame});
  return effect;
};

const BlizzardEffect = (board: Board, source: Point, target: Point): Effect => {
  const effect = new Effect();
  const ch = ray_character(source, target);

  const points = [target];
  const used = new Set<int>();
  while (points.length < 3) {
    const alt = target.add(sample(Direction.all));
    const key = alt.key();
    if (used.has(key)) continue;
    points.push(alt);
    used.add(key);
  }

  const trail: Sparkle = [
    [1, ch, '555'],
    [1, ch, '044'],
    [1, ch, '004'],
  ];
  const flame: Sparkle = [
    [1, '*', '555'],
    [1, '*', '044'],
    [1, '*', '004'],
    [1, '*', '555'],
    [1, '*', '044'],
    [1, '*', '004'],
  ];

  let hitFrame: int = 0;
  for (let p = 0; p < points.length; p++) {
    const d = 9 * p;
    const next = nonnull(points[p]);
    const line = LOS(source, next);
    for (let i = 1; i < line.length; i++) {
      add_sparkle(effect, trail, int(d + i), nonnull(line[i]));
    }
    const finish = add_sparkle(effect, flame, int(d + line.length - 1), next);
    if (p === 0) hitFrame = finish;
  }
  effect.mutAddEvent({type: FT.Hit, point: target, frame: hitFrame});
  return effect.scale(2 / 3);
};

const HeadbuttEffect = (board: Board, source: Point, target: Point): Effect => {
  const glyph = board.getGlyphAt(source);
  const underlying = board.getUnderlyingGlyphAt(source);
  const ch = ray_character(source, target);

  const trail = (): Sparkle => [
    [2, '#', '444'],
    [2, '#', '333'],
    [2, '#', '222'],
    [2, '#', '111'],
  ];

  const line = LOS(source, target);

  const move_along_line = (line: Point[]) => {
    const effect = new Effect();
    for (let i = 1; i < line.length - 1; i++) {
      const point = nonnull(line[i]);
      effect.mutAddParticle(int(i - 1), {point, glyph});
      add_sparkle(effect, trail(), int(i), point);
    }
    return effect;
  };

  const move_length = int(line.length - 2);
  const hold_point = line[move_length] || source;
  const hold_effect = Effect.Constant({point: hold_point, glyph}, int(32));
  const hitFrame = int(Math.max(move_length, 0));

  const to   = move_along_line(line);
  const hold = hold_effect.delay(move_length);
  const from = move_along_line(line.reverse());

  const back_length = int(hold.frames.length + hitFrame);
  const back_effect = Effect.Constant({point: source, glyph},
                                      int(from.frames.length - hitFrame));
  const back = back_effect.delay(back_length);

  const effect = UnderlayEffect(to.and(hold).then(from).and(back),
                                {point: source, glyph: underlying});

  effect.mutAddEvent({type: FT.Hit, point: target, frame: hitFrame});
  return effect.scale(1 / 2);
};

//////////////////////////////////////////////////////////////////////////////

const Effects = {
  // Attacks:
  Blizzard: BlizzardEffect,
  Ember:    EmberEffect,
  Headbutt: HeadbuttEffect,
  IceBeam:  IceBeamEffect,

  // Non-attacks:
  Summon:   SummonEffect,
  Switch:   SwitchEffect,
  Withdraw: WithdrawEffect,
};

export {Effect, Effects, Frame, FT, ray_character};
