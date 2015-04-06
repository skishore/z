import sys

if __name__ == '__main__':
  num_common_words = int(0 if len(sys.argv) == 1 else sys.argv[1])
  common_words = set()
  result = []

  with open('data/frequency/en.txt') as frequency:
    for line in frequency:
      word = line.split(' ')[0]
      if not word.isalpha():
        continue
      common_words.add(word)
      if len(common_words) == num_common_words:
        break

  with open('data/cmudict/cmudict-0.7b') as cmudict:
    for line in cmudict:
      if line.startswith(';'):
        continue
      (word, pronunciation) = line.split('  ')
      word = word.lower()
      if not word.isalpha():
        continue
      if not word in common_words:
        continue
      result.append(line)

  output_file = 'data/gen/cmudict'
  with open(output_file, 'w') as output:
    for line in result:
      output.write(line)
