import {Color, Glyph} from './lib';
import {flatten, int, nonnull, range, sample} from './lib';
import {Point, Direction, LOS} from './geo';

//////////////////////////////////////////////////////////////////////////////

interface AttackEffect {
  effect: Effect,
  hitFrame: int,
};

interface Board {
  getGlyphAt: (pos: Point) => Glyph,
  getUnderlyingGlyphAt: (pos: Point) => Glyph,
};

//////////////////////////////////////////////////////////////////////////////

interface Particle {point: Point, glyph: Glyph};
interface Frame extends Array<Particle> {};
interface Effect extends Array<Frame> {};

type Sparkle = [int, string, Color?][];

const add_particle = (effect: Effect, frame: int, particle: Particle): void => {
  while (frame >= effect.length) effect.push([]);
  nonnull(effect[frame]).push(particle);
};

const add_sparkle =
    (effect: Effect, sparkle: Sparkle, frame: int, point: Point): int => {
  sparkle.forEach(x => {
    const [delay, chars, color] = x;
    for (let i = 0; i < delay; i++, frame++) {
      const index = Math.floor(Math.random() * chars.length);
      const glyph = new Glyph(nonnull(chars[index]), color);
      add_particle(effect, frame, {glyph, point});
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

const ConstantEffect = (particle: Particle, n: int): Effect => {
  return Array(n).fill([particle]);
};

const PauseEffect = (n: int): Effect => {
  return Array(n).fill([]);
};

const OverlayEffect = (effect: Effect, particle: Particle): Effect => {
  const constant = ConstantEffect(particle, int(effect.length));
  return ParallelEffect([effect, constant]);
};

const UnderlayEffect = (effect: Effect, particle: Particle): Effect => {
  const constant = ConstantEffect(particle, int(effect.length));
  return ParallelEffect([constant, effect]);
};

const ExtendEffect = (effect: Effect, n: int): Effect => {
  const result: Effect = [];
  effect.forEach(frame => range(n).forEach(_ => result.push(frame)));
  return result;
};

const ParallelEffect = (effects: Effect[]): Effect => {
  const result: Effect = [];
  effects.forEach(effect => effect.forEach((frame, i) => {
    if (i >= result.length) result.push([]);
    frame.forEach(x => nonnull(result[i]).push(x));
  }));
  return result;
};

const SerialEffect = (effects: Effect[]): Effect => {
  return flatten(effects);
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
  return ExtendEffect(base, 4);
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
  return ExtendEffect(base, 3);
};

const RayEffect = (source: Point, target: Point, speed: int): Effect => {
  const color = '400';
  const result: Effect = [];
  const line = LOS(source, target);
  if (line.length <= 2) return result;

  const beam = new Glyph(ray_character(source, target), color);
  const mod = int((line.length - 2 + speed) % speed);
  const start = int(mod ? mod : mod + speed);
  for (let i = start; i < line.length - 1; i = int(i + speed)) {
    result.push(range(i).map(j => ({point: nonnull(line[j + 1]), glyph: beam})));
  }
  return result;
};

const SummonEffect = (source: Point, target: Point, glyph: Glyph): Effect => {
  const color = '400';
  const base: Effect = [];
  const line = LOS(source, target);
  const ball = new Glyph('*', color);
  for (let i = 1; i < line.length - 1; i++) {
    base.push([{point: nonnull(line[i]), glyph: ball}]);
  }
  const masked = UnderlayEffect(base, {point: target, glyph});
  return SerialEffect([masked, ExplosionEffect(target)]);
};

const WithdrawEffect = (source: Point, target: Point, glyph: Glyph): Effect => {
  const base = RayEffect(source, target, 4);
  const hide = {point: target, glyph};
  const impl = UnderlayEffect(ImplosionEffect(target), hide);
  const full = base[base.length - 1];
  if (!full) return impl;

  return SerialEffect([
    base,
    ParallelEffect([ExtendEffect([full], int(impl.length)), impl]),
    UnderlayEffect(base.slice().reverse(), hide),
  ]);
};

const SwitchEffect = (source: Point, target: Point, glyph: Glyph): Effect => {
  return SerialEffect([
    WithdrawEffect(source, target, glyph),
    ConstantEffect({point: target, glyph}, 4),
    SummonEffect(source, target, glyph),
  ]);
};

const EmberEffect = (board: Board, source: Point, target: Point): AttackEffect => {
  const effect: Effect = [];
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
  return {effect, hitFrame};
};

const IceBeamEffect = (board: Board, source: Point, target: Point): AttackEffect => {
  const effect: Effect = [];
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
  return {effect, hitFrame};
};

const BlizzardEffect = (board: Board, source: Point, target: Point): AttackEffect => {
  const effect: Effect = [];
  const ch = ray_character(source, target);

  const points = [target];
  const used = new Set<int>();
  while (points.length < 4) {
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
    [2, '*', '555'],
    [2, '*', '044'],
    [2, '*', '004'],
    [2, '*', '555'],
    [2, '*', '044'],
    [2, '*', '004'],
  ];

  let hitFrame: int = 0;
  for (let p = 0; p < points.length; p++) {
    const d = 12 * p;
    const next = nonnull(points[p]);
    const line = LOS(source, next);
    for (let i = 1; i < line.length; i++) {
      add_sparkle(effect, trail, int(d + i), nonnull(line[i]));
    }
    const finish = add_sparkle(effect, flame, int(d + line.length - 1), next);
    if (p === 0) hitFrame = finish;
  }
  hitFrame = int(hitFrame - Math.ceil(hitFrame / 3));
  return {effect: effect.filter((_, i) => i % 3), hitFrame};
};

const HeadbuttEffect = (board: Board, source: Point, target: Point): AttackEffect => {
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
    const effect: Effect = [];
    for (let i = 1; i < line.length - 1; i++) {
      const point = nonnull(line[i]);
      add_particle(effect, int(i - 1), {point, glyph});
      add_sparkle(effect, trail(), int(i), point);
    }
    return effect;
  };

  const move_length = int(line.length - 2);
  const hold_pause = PauseEffect(move_length);
  const hold_point = line[move_length] || source;
  const hold_effect = ConstantEffect({point: hold_point, glyph}, int(32));
  let hitFrame = int(Math.max(move_length, 0));

  const towards = move_along_line(line);
  const hold    = SerialEffect([hold_pause, hold_effect]);
  const away    = move_along_line(line.reverse());

  const back_length = int(hold.length + hitFrame);
  const back_pause = PauseEffect(back_length);
  const back_effect = ConstantEffect({point: source, glyph}, int(away.length - hitFrame));
  const back = SerialEffect([back_pause, back_effect]);

  const result = UnderlayEffect(
      ParallelEffect([SerialEffect([ParallelEffect([towards, hold]), away]), back]),
      {point: source, glyph: underlying});

  hitFrame = int(hitFrame - Math.ceil(hitFrame / 2));
  return {effect: result.filter((_, i) => i % 2), hitFrame};
};

//////////////////////////////////////////////////////////////////////////////

const Attacks = {
  Blizzard: BlizzardEffect,
  Ember:    EmberEffect,
  Headbutt: HeadbuttEffect,
  IceBeam:  IceBeamEffect,
};

const Combinators = {
  Constant: ConstantEffect,
  Parallel: ParallelEffect,
  Pause:    PauseEffect,
  Serial:   SerialEffect,
};

//////////////////////////////////////////////////////////////////////////////

export {AttackEffect, Attacks, Combinators, ray_character};
export {Effect, SummonEffect, WithdrawEffect};
