import sys

# TODO(skishore): Convert from CMU phonetic representation to our
# Devanagari transliteration scheme. Besides applying the substitutions
# in our symbol map, we also have to execute a few more rules:
#   - Trailing Zs should be transliterated as "s".
#   - Some consonants, like K or T, when followed by HH, should be aspirated.

if __name__ == '__main__':
  num_common_words = int(0 if len(sys.argv) == 1 else sys.argv[1])
  words = set()
  result = []

  with open('data/frequency/en.txt') as frequency:
    for line in frequency:
      word = line.split(' ')[0]
      if not word.isalpha():
        continue
      words.add(word)
      if len(words) == num_common_words:
        break

  with open('data/scripts/extra_transliterations') as extra_transliterations:
    for line in extra_transliterations:
      word = line.strip()
      if not word.isalpha():
        continue
      words.add(word)

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
      result.append(line)

  output_file = 'data/gen/cmudict'
  with open(output_file, 'w') as output:
    for line in result:
      output.write(line)
