type IntLiteral = -1 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 60;
type int = number & ({__type__: 'int'} | IntLiteral);

const int = (x: number): int => (x | 0) as int;

const assert = (x: boolean, message?: () => string) => {
  if (x) return;
  throw new Error(message ? message() : 'Assertion failed!');
};

const nonnull = <T>(x: T | null | undefined, message?: () => string): T => {
  if (x !== null && x !== undefined) return x;
  throw new Error(message ? message() : 'Unexpected null!');
};

const only = <T>(xs: T[]): T => {
  assert(xs.length === 1);
  return xs[0]!;
};

//////////////////////////////////////////////////////////////////////////////

const show = (c: int): string => {
  return `RGBA: (${c & 0xff}, ${(c >> 8) & 0xff}, ` +
         `${(c >> 16) & 0xff}, ${(c >> 24) & 0xff})`;
};

const transformIcon = (data: ImageData): ImageData => {
  const pixels = new Int32Array(data.data.buffer);
  assert(pixels.length === data.width * data.height);
  const black = int(0xff000000);
  const white = int(0xffffffff);

  const colors: int[] = [];
  for (let y = 0; y < data.height; y++) {
    for (let x = 0; x < data.width; x++) {
      const index = x + y * data.width;
      const pixel = pixels[index]!;
      if (pixel !== white) colors.push(int(pixel));
    }
  }
  if (colors.length === 0) return data;

  const is_gray = (c: int): boolean => {
    const threshold = 4;
    const r = (c >> 0)  & 0xff;
    const g = (c >> 8)  & 0xff;
    const b = (c >> 16) & 0xff;
    return Math.abs(r - g) < threshold &&
           Math.abs(g - b) < threshold &&
           Math.abs(b - r) < threshold;
  };

  const first = nonnull(colors[0]);
  const counts: Map<int, int> = new Map();
  for (const color of colors) {
    if (is_gray(color)) continue;
    const count = counts.get(color) ?? 0;
    counts.set(color, int(count + 1));
  }

  let best_color: int = first;
  let best_count: int = -1;
  for (const color of colors) {
    const count = counts.get(color) ?? 0;
    if (count > best_count) {
      best_color = color;
      best_count = count;
    }
  }

  const score_color = (c: int): number => {
    const r = (c >> 0)  & 0xff;
    const g = (c >> 8)  & 0xff;
    const b = (c >> 16) & 0xff;
    return r + g + b;
  };
  const mid = Math.floor(0.50 * colors.length);
  const sorted = colors.slice().sort((x, y) => score_color(x) - score_color(y));
  const bound  = score_color(nonnull(sorted[mid]));
  const if_strict = colors.filter(x => score_color(x) <  bound).length;
  const if_weak   = colors.filter(x => score_color(x) <= bound).length;
  const strict = Math.abs(if_strict - mid) < Math.abs(if_weak - mid);

  for (let y = 0; y < data.height; y++) {
    for (let x = 0; x < data.width; x++) {
      const index = x + y * data.width;
      const pixel = int(pixels[index]!);
      pixels[index] = ((): int => {
        if (pixel === white) return black;
        const score = score_color(pixel);
        const dark = strict ? score < bound : score <= bound;
        return dark ? best_color : black;
      })();
    }
  }
  return data;
};

const main = (filename: string, size: int) => {
  const canvas = nonnull(document.getElementsByTagName('canvas')[0]);
  const context = nonnull(canvas.getContext('2d'));

  const image = new Image();
  image.src = filename;
  image.onload = () => {
    canvas.width = image.width;
    canvas.height = image.height;
    context.drawImage(image, 0, 0);

    const base = context.getImageData(0, 0, 1, image.height);
    const w = image.width;
    const h = (() => {
      for (let i = 0; i < image.height; i++) {
        const offset = 4 * i;
        for (let j = 0; j < 4; j++) {
          if (base.data[offset + j] !== base.data[j]) return i;
        }
      }
      return image.height;
    })();
    console.log(`Size: ${w} x ${h}`);

    assert((w - 1) % (size + 1) === 0);
    assert((h - 1) % (size + 1) === 0);
    const cols = (w - 1) / (size + 1);
    const rows = (h - 1) / (size + 1);
    console.log(`Grid: ${cols} x ${rows}`);

    for (let row = 0; row < rows; row++) {
      for (let col = 0; col < cols; col++) {
        const dx = 1 + (size + 1) * col;
        const dy = 1 + (size + 1) * row;
        const test_a = context.getImageData(dx - 1, dy + 1, 1, 1);
        const test_b = context.getImageData(dx + size, dy + 1, 1, 1);
        const okay = (() => {
          for (let j = 0; j < 4; j++) {
            if (test_a.data[j] !== base.data[j]) return false;
            if (test_b.data[j] !== base.data[j]) return false;
          }
          return true;
        })();
        if (!okay) continue;

        const data = context.getImageData(dx, dy, size, size);
        const result = transformIcon(data);
        context.putImageData(result, dx, dy);
      }
    }
  };
};

main(`images/oras-icons.png`, int(32));
