@semantics = @semantics or {}

semantics.SIGNS = {
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

semantics.REVERSE_SIGNS = {}
for vowel, sign of semantics.SIGNS
  assert sign not of semantics.REVERSE_SIGNS
  semantics.REVERSE_SIGNS[sign] = vowel
