import {assert, int, nonnull, range} from './lib';

//////////////////////////////////////////////////////////////////////////////

interface Point { x: int, y: int };
interface Chars {[codepoint: number]: Point};

interface FontConfig {
  name: string,
  data: string,
  width: int,
  height: int,
  rows: int,
  cols: int,
  chars: Chars,
};

const printable = (cols: int): Chars => {
  const result: Chars = {};
  for (let i = 32; i < 127; i++) {
    result[i] = {x: int(i % cols), y: int(Math.floor(i / cols))};
  }
  return result;
};

const aquarius = (): FontConfig => {
  const data = 'fonts/aquarius_8x8.png';
  const [width, height, rows, cols] = [8, 8, 16, 16] as [int, int, int, int];
  const chars = printable(cols);
  return {name: 'Aquarius', data, width, height, rows, cols, chars};
};

const unifont = (): FontConfig => {
  const data = 'fonts/unifont_8x16.png';
  const [width, height, rows, cols] = [8, 16, 16, 16] as [int, int, int, int];
  const chars = printable(cols);
  return {name: 'Unifont', data, width, height, rows, cols, chars};
};

//////////////////////////////////////////////////////////////////////////////

interface ImageData {
  width: int,
  height: int,
  channels: int,
  data: Uint8Array,
};

interface Font {
  config: FontConfig,
  data: ImageData,
};

const glyph = (font: Font, codepoint: int, scale: int, wide?: boolean): string => {
  const {config, data} = font;
  const hex = '0123456789ABCDEF';
  assert(hex.length === 16);
  const sx = int(scale * config.width);
  const sy = int(scale * config.height);

  const {x: cx, y: cy} = nonnull(config.chars[codepoint]);
  const lines = range(sy).map(y => {
    const bits = range(sx).map(x => {
      const px = cx * config.width + Math.floor(x / scale);
      const py = cy * config.height + Math.floor(y / scale);
      return data.data[data.channels * (data.width * py + px)] === 0xff;
    })
    const bytes = range(int(Math.floor((sx + 7) / 8))).map(i => {
      const byte = range(8).reduce(
        (acc, j) => int(acc | (bits[8 * i + j] ? (1 << (7 - j)) : 0)), 0);
      assert(0 <= byte && byte <= 0xff);
      return `${hex[Math.floor(byte / 16)]}${hex[byte % 16]}`;
    });
    return bytes.join('');
  });

  const encoding = wide ? codepoint + 0xff00 - 0x20 : codepoint;

  const header = `
STARTCHAR char${encoding}
ENCODING ${encoding}
SWIDTH ${120 * sx} 0
DWIDTH ${sx} 0
BBX ${sx} ${sy} 0 0
BITMAP
  `;
  const parts = [header];
  lines.forEach(x => parts.push(x));
  parts.push('ENDCHAR');
  return parts.map(x => x.trim()).join('\n');
};

const bdf = (font: Font, wide?: Font): string => {
  const {config} = font;
  const foundry = 'Misc';
  const weight = 'Medium';
  const slant = 'R';
  const set_width = 'SemiCondensed';
  const {height, width} = config;
  const name = wide ? `${config.name}Plus${wide.config.name}` : config.name;

  const codepoints = Object.keys(config.chars).map(x => int(parseInt(x, 10)));
  codepoints.sort((x, y) => x - y);

  const wide_chars =
    Object.keys(wide ? wide.config.chars : {})
      .map(x => int(parseInt(x, 10)))
      .filter(x => 33 <= x && x < 128);
  const last = wide_chars[wide_chars.length - 1];

  const header = `
STARTFONT 2.1
FONT -${foundry}-${name}-${weight}-${slant}-${set_width}--${config.height}-120-75-75-C-60-ISO10646-1
SIZE ${height} 75 75
FONTBOUNDINGBOX ${width} ${height} 0 0
STARTPROPERTIES 20
FONTNAME_REGISTRY ""
FOUNDRY "${foundry}"
FAMILY_NAME "${name}"
WEIGHT_NAME "${weight}"
SLANT "${slant}"
SETWIDTH_NAME "${set_width}"
ADD_STYLE_NAME ""
PIXEL_SIZE ${height}
POINT_SIZE 120
RESOLUTION_X 75
RESOLUTION_Y 75
SPACING "C"
AVERAGE_WIDTH 60
CHARSET_REGISTRY "ISO10646"
CHARSET_ENCODING "1"
DEFAULT_CHAR 0
FONT_DESCENT 0
FONT_ASCENT ${height}
COPYRIGHT "Unknown"
ENDPROPERTIES
CHARS ${codepoints.length + wide_chars.length + (last ? 1 : 0)}
  `;
  const parts = [header];
  const scale = wide ? int(config.height / wide.config.height) : 1;
  codepoints.forEach(x => parts.push(glyph(font, x, 1)));
  wide_chars.forEach(x => parts.push(glyph(nonnull(wide), x, scale, true)));

  // There's some bug in font rendering on both Mac OS X Terminal.app and on
  // Alacritty, where the last character of a font is not rendered correctly.
  //
  // Work around this bug by appending a dummy character at the end.
  if (last) {
    const part = glyph(nonnull(wide), last, scale, true);
    const code = last + 0xff00 - 0x20;
    parts.push(part.replace(new RegExp(`${code}`, 'g'), `${code + 1}`));
  }

  parts.push('ENDFONT');
  return parts.map(x => x.trim()).join('\n');
};

const show = (font: Font, codepoint: int) => {
  const {config, data} = font;
  const unknown = nonnull('?'.codePointAt(0));
  const {width, height, chars} = config;
  const {x: cx, y: cy} = nonnull(chars[codepoint] || chars[unknown]);

  const bits = range(height).map(y => range(width).map(x => {
    const px = cx * width + x;
    const py = cy * height + y;
    return data.data[data.channels * (data.width * py + px)] === 0xff;
  }));

  console.log(bits.map(x => x.map(y => y ? '##' : '  ').join('')).join('\n'));
};

const main = (font: Font, wide?: Font) => {
  const {config, data} = font;
  assert(data.data.length === data.width * data.height * data.channels);
  assert(data.width === config.width * config.cols);
  assert(data.height === config.height * config.rows);
  if (false) {
    const message = 'Hello!';
    for (let i = 0; i < message.length; i++) {
      show(font, int(message.charCodeAt(i)));
    }
  }
  console.log(bdf(font, wide));
};

//////////////////////////////////////////////////////////////////////////////

declare const console: any;
declare const require: any;

const pngparse = require('../extern/pngparse');

const load = (config: FontConfig, fn: (font: Font) => void) => {
  pngparse.parseFile(config.data, (error: Error, data: ImageData) => {
    if (error) throw error;
    fn({config, data});
  });
};

load(unifont(), font => load(aquarius(), wide => main(font, wide)));

export {};
