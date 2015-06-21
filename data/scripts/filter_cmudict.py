import json
import sys


class Converter(object):
  def __init__(self):
    self.symbol_map = {}
    self.combines_with_hh = {
        'M': False, 'N': False, 'R': False, 'S': False,
        'CH': True, 'D': True, 'G': True, 'JH': True, 'K': True,
        'NG': True, 'P': True, 'T': True, 'TH': True}
    with open('data/scripts/cmudict.symbols.map') as symbol_map:
      for line in symbol_map:
        tokens = line.strip().split()
        if len(tokens) < 2:
          continue
        (symbol, pronunciation) = (tokens[0], tokens[1:])
        assert symbol == '#' or symbol.isalnum(), symbol
        if symbol == '#':
          continue
        if not all(segment.isalpha() for segment in pronunciation):
          continue
        self.symbol_map[symbol] = pronunciation

  def _should_combine_with_hh(self, last_symbol):
    last_char = last_symbol[-1]
    assert (last_symbol in self.combines_with_hh) or \
           (last_char.isalnum() and not last_char.isalpha()), last_symbol
    return self.combines_with_hh.get(last_symbol)

  def convert(self, entry):
    entry = entry.split()
    if not all(symbol in self.symbol_map for symbol in entry):
      return None
    result = []
    last_symbol = 'AH0'
    for symbol in entry:
      if symbol == 'HH' and self._should_combine_with_hh(last_symbol):
        result[-1] += 'h'
      else:
        result.extend(self.symbol_map[symbol])
      last_symbol = symbol
    return result


if __name__ == '__main__':
  num_common_words = int(0 if len(sys.argv) == 1 else sys.argv[1])
  words = set()
  result = []
  converter = Converter()

  with open('data/coca/top-5000.txt') as frequency:
    for line in frequency:
      word = line.strip().split(',')[0]
      if not word.isalpha() or word != word.lower():
        continue
      words.add(word)
      if len(words) == num_common_words:
        break

  with open('data/scripts/transliteration_whitelist') as whitelist:
    for line in whitelist:
      word = line.strip()
      if not word.isalpha():
        continue
      words.add(word)

  with open('data/scripts/transliteration_blacklist') as blacklist:
    for line in blacklist:
      word = line.strip()
      if not word.isalpha():
        continue
      if word in words:
        words.remove(word)

  with open('data/cmudict/cmudict-0.7b.modified') as cmudict:
    for line in cmudict:
      if line.startswith(';'):
        continue
      (word, pronunciation) = line.split('  ')
      word = word.lower()
      if not word.isalpha():
        continue
      if not word in words:
        continue
      converted = converter.convert(pronunciation)
      if converted and len(converted) <= 8:
        result.append([word, converted])

  output_file = 'data/gen/cmudict.coffee'
  with open(output_file, 'w') as output:
    output.write('@semantics = @semantics or {}\n\n')
    output.write('semantics.ENGLISH_WORDS_WITH_TRANSLITERATIONS = [ \\\n')
    for line in result:
      output.write('%s,\n' % (json.dumps(line),))
    output.write(']')
