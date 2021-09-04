type int = number;

const assert = (x: boolean, fn?: () => string): void => {
  if (!x) throw new Error(fn && fn());
};

const flatten = <T>(xss: T[][]): T[] => {
  const result: T[] = [];
  xss.forEach(xs => xs.forEach(x => result.push(x)));
  return result;
};

const nonnull = <T>(x: T): NonNullable<T> => {
  if (x === null || x === undefined) throw new Error();
  return x as NonNullable<T>;
}

const range = (n: int): int[] => {
  n = n | 0;
  const result = [];
  for (let i = 0; i < n; i++) {
    result.push(i);
  }
  return result;
};

//////////////////////////////////////////////////////////////////////////////
// Glyph helper to speed up blessed.js formatting.

type Glyph = string & {__type__: 'glyph'};

type Color = 'black' | 'red' | 'green' | 'yellow' | 'blue' | 'magenta' | 'cyan' | 'white';

const Glyph = (ch: string, color?: Color, light?: boolean): Glyph => {
  assert(ch.length === 1);
  if (!color) return ch as Glyph;
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
  return `\x1b[${index + (light ? 90 : 30)}m${ch}\x1b[39m` as Glyph;
};

//////////////////////////////////////////////////////////////////////////////

export {assert, flatten, int, nonnull, range, Glyph};
