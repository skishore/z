class @HindiToEnglishShortAnswerGame extends DialogPage
  @template = 'short_answer_translit'
  @height = '2.8em'
  @trap_input: true

  constructor: (show_guides) ->
    n = _.random 2, 4
    @hindi = ((do semantics.Devanagari.get_segment) for i in [0...n])
    @english = (semantics.HindiToEnglish.unsafe hindi for hindi in @hindi)
    # The user's current entry for each Hindi transliteration task.
    @entries = ('' for i in @hindi)
    @guides = (show_guides for i in @hindi)
    @mistake = undefined
    @i = 0

  accepts_input: (char) ->
    /^[a-z ]$/i.test char

  active: ->
    @i < @hindi.length

  get_data: ->
    data = {segments: []}
    for hindi, i in @hindi
      data.segments.push
        segment: hindi
        entry: @_encode @_get_entry_data i
        class: if i == @mistake then 'wrong' \
               else if @entries[i] == @english[i] then 'correct'
        width: (Math.floor 100/@hindi.length) + '%'
    data

  on_input: (char) ->
    if char != @english[@i][@entries[@i].length]
      if @guides[@i]
        return false
      @entries[@i] = ''
      @guides[@i] = true
      @mistake = @i
      # Signal that the player made a mistake in the dialog.
      @signal 'OnTaskError'
      return true
    @entries[@i] += char
    @mistake = undefined
    if @entries[@i] == @english[@i]
      @i += 1
      # Signal that the player completed the task or the whole page.
      @signal (if do @active then 'OnTaskCompletion' else 'OnPageCompletion')
    true

  _encode: (data) ->
    if data.cursor?
      data.cursor = semantics.HindiToEnglish.english_to_display data.cursor
    data.guide = semantics.HindiToEnglish.english_to_display data.guide
    data.text = semantics.HindiToEnglish.english_to_display data.text
    data

  _get_entry_data: (i) ->
    cursor = undefined
    guide = if @guides[i] then (@english[i].slice @entries[i].length) else ''
    cursor = undefined
    if i == @i
      cursor = guide[0] or ''
      guide = guide.slice 1
    text: @entries[i]
    cursor: cursor
    show_cursor: cursor?
    guide: guide
