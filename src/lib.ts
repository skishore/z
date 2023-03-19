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

type Glyph = string & {__type__: 'glyph'};

type Color = 'black' | 'red' | 'green' | 'yellow' | 'blue' | 'magenta' | 'cyan' | 'white';

const WideCharOffset = 0xff00 - 0x20;

const Color = (text: string, color: Color, light?: boolean): string => {
  const index = (() => {
    switch (color) {
      case 'black':   return 0;
      case 'red':     return 1;
      case 'green':   return 2;
      case 'yellow':  return 3;
      case 'blue':    return 4;
      case 'magenta': return 5;
      case 'cyan':    return 6;
      case 'white':   return 7;
    }
  })();
  return `\x1b[${index + (light ? 90 : 30)}m${text}\x1b[39m`;
};

const Glyph = (ch: string, color?: Color, light?: boolean): Glyph => {
  assert(ch.length === 1);
  if (ch === ' ') return '  ' as Glyph;
  if (ch === '\n') return '\n' as Glyph;
  const wide = String.fromCodePoint(ch.codePointAt(0)! + WideCharOffset);
  return (color ? Color(wide, color, light) : wide) as Glyph;
};

const Recolor = (glyph: Glyph): Glyph => {
  const index = glyph.indexOf('m');
  const wide = index < 0 ? glyph : (glyph[index + 1] || ' ');
  const code = wide.codePointAt(0)! - WideCharOffset;
  const ch = code < 0 ? wide : String.fromCodePoint(code);
  return `\x1b[41m${Glyph(ch, 'black')}\x1b[0m` as Glyph;
};

//////////////////////////////////////////////////////////////////////////////

export {Color, Glyph, Recolor};
export {assert, flatten, int, nonnull, only, range, sample, weighted};
