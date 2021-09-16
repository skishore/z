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
    result[i] = {x: i % cols, y: Math.floor(i / cols)};
  }
  return result;
};

const aquarius = (): FontConfig => {
  const data = 'fonts/aquarius_8x8.png';
  const [width, height, rows, cols] = [8, 8, 16, 16];
  const chars = printable(cols);
  return {name: 'Aquarius', data, width, height, rows, cols, chars};
};

const unifont = (): FontConfig => {
  const data = 'fonts/unifont_8x16.png';
  const [width, height, rows, cols] = [8, 16, 16, 16];
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

const glyph = (font: Font, codepoint: int): string => {
  const {config, data} = font;
  const hex = '0123456789ABCDEF';
  assert(hex.length === 16);

  const {x: cx, y: cy} = nonnull(config.chars[codepoint]);
  const lines = range(config.height).map(y => {
    const bits = range(config.width).map(x => {
      const px = cx * config.width + x;
      const py = cy * config.height + y;
      return data.data[data.channels * (data.width * py + px)] === 0xff;
    });
    const bytes = range(Math.floor((config.width + 7) / 8)).map(i => {
      const byte = range(8).reduce(
        (acc, j) => acc | (bits[8 * i + j] ? (1 << (7 - j)) : 0), 0);
      assert(0 <= byte && byte <= 0xff);
      return `${hex[Math.floor(byte / 16)]}${hex[byte % 16]}`;
    });
    return bytes.join('');
  });

  const header = `
STARTCHAR char${codepoint}
ENCODING ${codepoint}
SWIDTH ${120 * config.width} 0
DWIDTH ${config.width} 0
BBX ${config.width} ${config.height} 0 0
BITMAP
  `;
  const parts = [header];
  lines.forEach(x => parts.push(x));
  parts.push('ENDCHAR');
  return parts.map(x => x.trim()).join('\n');
};

const bdf = (font: Font): string => {
  const {config} = font;
  const foundry = 'Misc';
  const weight = 'Medium';
  const slant = 'R';
  const set_width = 'SemiCondensed';
  const {name, height, width} = config;

  const codepoints = Object.keys(config.chars).map(x => parseInt(x, 10));
  codepoints.sort((x, y) => x - y);

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
CHARS ${codepoints.length}
  `;
  const parts = [header];
  codepoints.forEach(x => parts.push(glyph(font, x)));
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

const main = (font: Font) => {
  const {config, data} = font;
  assert(data.data.length === data.width * data.height * data.channels);
  assert(data.width === config.width * config.cols);
  assert(data.height === config.height * config.rows);
  if (false) {
    const message = 'Hello!';
    for (let i = 0; i < message.length; i++) {
      show(font, message.charCodeAt(i));
    }
  }
  console.log(bdf(font));
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

load(aquarius(), main);

export {aquarius, unifont};
