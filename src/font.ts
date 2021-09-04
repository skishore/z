import {assert, int, range, Point} from './lib';

//////////////////////////////////////////////////////////////////////////////

interface FontConfig {
  name: string,
  width: int,
  height: int,
  rows: int,
  cols: int,
  map: {[codepoint: number]: Point},
};

const aquarius = (): FontConfig => {
  const rows = [
    ` !"#$%&'()*+,-./`,
    '0123456789:;<=>?',
    '@ABCDEFGHIJKLMNO',
    'PQRSTUVWXYZ[ ]^_',
    '`abcdefghijklmno',
    'pqrstuvwxyz{|}~ ',
  ];
  const map: {[codepoint: number]: Point} = {};
  rows.forEach((row, i) => {
    assert(row.length === 16);
    for (let j = 0; j < row.length; j++) {
      const codepoint = (() => {
        if (row[j] === ' ') {
          if (i === 0) return ' '.charCodeAt(0);
          if (i === 3) return '\\'.charCodeAt(0);
          return 0;
        }
        return row.charCodeAt(j);
      })();
      if (codepoint) map[codepoint] = new Point(j, i + 2);
    }
  });
  return {name: 'Aquarius', width: 8, height: 8, rows: 16, cols: 16, map};
};

//////////////////////////////////////////////////////////////////////////////

interface ImageData {
  width: int,
  height: int,
  channels: int,
  data: Uint8Array,
};

const glyph = (data: ImageData, config: FontConfig, codepoint: int): string => {
  const hex = '0123456789ABCDEF';
  assert(hex.length === 16);

  const {x: cx, y: cy} = config.map[codepoint]!;
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

const bdf = (data: ImageData, config: FontConfig): string => {
  const foundry = 'Misc';
  const weight = 'Medium';
  const slant = 'R';
  const set_width = 'SemiCondensed';
  const {name, height, width} = config;

  const codepoints = Object.keys(config.map).map(x => parseInt(x, 10));
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
  codepoints.forEach(x => parts.push(glyph(data, config, x)));
  parts.push('ENDFONT');
  return parts.map(x => x.trim()).join('\n');
};

const show = (data: ImageData, config: FontConfig, codepoint: int) => {
  const unknown = '?'.codePointAt(0)!;
  const {x: cx, y: cy} = config.map[codepoint] || config.map[unknown]!;

  const bits = range(config.height).map(y => range(config.width).map(x => {
    const px = cx * config.width + x;
    const py = cy * config.height + y;
    return data.data[data.channels * (data.width * py + px)] === 0xff;
  }));

  console.log(bits.map(x => x.map(y => y ? '##' : '  ').join('')).join('\n'));
};

const main = (data: ImageData, config: FontConfig) => {
  assert(data.data.length === data.width * data.height * data.channels);
  assert(data.width === config.width * config.cols);
  assert(data.height === config.height * config.rows);
  if (false) {
    const message = 'Hello!';
    for (let i = 0; i < message.length; i++) {
      show(data, config, message.charCodeAt(i));
    }
  }
  console.log(bdf(data, config));
};

//////////////////////////////////////////////////////////////////////////////

declare const console: any;
declare const require: any;

const pngparse = require('../extern/pngparse');

pngparse.parseFile('fonts/aquarius_8x8.png',
  (error: Error, data: ImageData) => {
    if (error) throw error;
    main(data, aquarius());
});

export {};
