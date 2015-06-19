@semantics = @semantics or {}

class semantics.Devanagari
  @VOWELS = ['अ', 'आ', 'इ', 'ई', 'उ', 'ऊ', 'ए', 'ऐ', 'ओ', 'औ']
  @CONSONANT_ROWS = [
    ['क', 'ख', 'ग', 'घ', 'ङ'],
    ['च', 'छ', 'ज', 'झ', 'ञ'],
    ['ट', 'ठ', 'ड', 'ढ', 'ण'],
    ['त', 'थ', 'द', 'ध', 'न'],
    ['प', 'फ', 'ब', 'भ', 'म'],
    ['य', 'र', 'ल', 'व'],
    ['श', 'ष', 'स', 'ह'],
  ]
  @DIGITS = ['०', '१', '२', '३', '४', '५', '६', '७', '८', '९']
  @SIGNS = {
    'अ': ''
    'आ': '\u093E'
    'इ': '\u093F'
    'ई': '\u0940'
    'उ': '\u0941'
    'ऊ': '\u0942'
    'ऋ': '\u0943'
    'ऌ': '\u0962'
    'ऍ': '\u0946'
    'ऍ': '\u0946'
    'ए': '\u0947'
    'ऐ': '\u0948'
    'ऑ': '\u094A'
    'ओ': '\u094B'
    'औ': '\u094C'
    'ॠ': '\u0944'
    'ॡ': '\u0963'
    'ँ': 'ँ'
    'ं': 'ं'
    'ः': 'ः'
  }
  @VIRAMA = '\u094D'

  @CONSONANTS = [].concat.apply [], @CONSONANT_ROWS
  @ALPHABET = @VOWELS.concat @CONSONANTS
  @ALL = @ALPHABET.concat @DIGITS

  @REVERSE_SIGNS = {}
  for vowel, sign of @SIGNS
    @REVERSE_SIGNS[sign] = vowel

  @get_segment: ->
    while true
      consonant = _.sample @CONSONANTS
      vowel = @SIGNS[_.sample @VOWELS]
      if consonant not in ['ङ', 'ञ'] or vowel == ''
        return consonant + vowel
