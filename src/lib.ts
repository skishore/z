type IntLiteral = -1 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
                  9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 60;
type int = number & ({__type__: 'int'} | IntLiteral);

const assert = (x: boolean, fn?: () => string): void => {
  if (!x) throw new Error(fn && fn());
};

const flatten = <T>(xss: T[][]): T[] => {
  const result: T[] = [];
  xss.forEach(xs => xs.forEach(x => result.push(x)));
  return result;
};

const int = (x: number): int => (x | 0) as int;

const nonnull = <T>(x: T): NonNullable<T> => {
  if (x === null || x === undefined) throw new Error();
  return x as NonNullable<T>;
};

const only = <T>(xs: T[]): T => {
  assert(xs.length === 1);
  return xs[0]!;
};

const permuteHelper = <T>(xs: T[], n: int, start: int, result: T[][]): T[][] => {
  if (start === n) {
    result.push(xs.slice(0, n));
    return result;
  }
  for (let i = start; i < xs.length; i++) {
    const tmp = xs[i]!;
    xs[i] = xs[start]!;
    xs[start] = tmp;
    permuteHelper(xs, n, int(start + 1), result);
    xs[start] = xs[i]!;
    xs[i] = tmp;
  }
  return result;
};

const permute = <T>(xs: T[], n: int): T[][] => {
  return n <= xs.length ? permuteHelper(xs, n, 0, []) : [];
};

const range = (n: int): int[] => {
  const result: int[] = [];
  for (let i = int(0); i < n; i++) {
    result.push(i);
  }
  return result;
};

const sample = <T>(xs: T[]): T => {
  assert(xs.length > 0);
  return xs[Math.floor(Math.random() * xs.length)]!;
};

const weighted = <T>(xs: [int, T][]): T => {
  const total = xs.reduce((acc, x) => acc + (x[0] | 0), 0);
  assert(total > 0);
  let value = Math.floor(Math.random() * total) + 1;
  for (const [weight, choice] of xs) {
    value -= weight;
    if (value <= 0) return choice;
  }
  assert(false);
  return xs[xs.length - 1]![1];
};

//////////////////////////////////////////////////////////////////////////////
// Glyph helper to speed up blessed.js formatting.

type Digit = ('0' | '1' | '2' | '3' | '4' | '5');
type Color = 'black' | 'gray' | `${Digit}${Digit}${Digit}`;

const WideCharOffset = 0xff00 - 0x20;

const Color = (text: string, fg?: Color | null, bg?: Color | null): string => {
  const index = (color: Color) => {
    if (color === 'black') return 0;
    if (color === 'gray') return 16 + 216 + 5;
    const base = '0'.charCodeAt(0);
    const r = color.charCodeAt(0) - base;
    const g = color.charCodeAt(1) - base;
    const b = color.charCodeAt(2) - base;
    return 16 + b + 6 * g + 36 * r;
  };
  if (fg) text = `\x1b[38;5;${index(fg)}m${text}\x1b[39m`;
  if (bg) text = `\x1b[48;5;${index(bg)}m${text}\x1b[49m`;
  return text;
}

class Glyph {
  private readonly ch: string;
  private readonly fg: Color | null;
  private readonly bg: Color | null;
  private readonly text: string;

  constructor(ch: string, fg?: Color | null, bg?: Color | null) {
    assert(ch.length === 1);
    this.ch = ch;
    this.fg = fg || null;
    this.bg = bg || null;
    this.text = Glyph.getColoredText(this.ch, this.fg, this.bg);
  }

  recolor(fg?: Color | null, bg?: Color | null): Glyph {
    return new Glyph(this.ch, fg, bg);
  }

  toShortString(): string { return Color(this.ch, this.fg, this.bg); }

  toString(): string { return this.text; }

  private static getColoredText(
      ch: string, fg: Color | null, bg: Color | null): string {
    if (ch === ' ')  return Color('  ', fg, bg);
    if (ch === '\n') return Color('\n', fg, bg);
    const wide = String.fromCodePoint(ch.codePointAt(0)! + WideCharOffset);
    return Color(wide, fg, bg);
  }
};

//////////////////////////////////////////////////////////////////////////////

export {Color, Glyph};
export {assert, flatten, int, nonnull, only, permute, range, sample, weighted};
