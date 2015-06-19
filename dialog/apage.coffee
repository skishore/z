@DialogAttackResult = {
  WRONG_ENEMY: 0
  RIGHT_ENEMY: 1
  COMBAT_WON: 2
}

class @DialogPage
  # These static variables should be overridden by subclasses.
  # @name will always be set to the class name by CoffeeScript.
  @template = 'the name of the Meteor template for this dialog'
  @height = 'the height of the dialog, eg. 2.2em'
  @trap_input = 'true if this dialog traps user input'

  accepts_input: (char) ->
    # Returns true if the dialog accepts the given input.
    assert false, "#{@constructor.name}.accepts_input is not implemented!"

  active: ->
    # Returns true if the dialog is still active.
    assert false, "#{@constructor.name}.active is not implemented!"

  get_label: (sid) ->
    # Used if this dialog causes the game to show enemy labels.
    return undefined

  get_data: ->
    # Returns the data needed to instantiate this dialog's Handlebars template.
    assert false, "#{@constructor.name}.get_data is not implemented!"

  on_attack: (sid) ->
    # For combat dialogs only. Returns an AttackResult. 
    assert false, "#{@constructor.name}.on_attack is not implemented!"

  on_input: (char) ->
    # Takes a single-character input and return true if the dialog changed.
    assert false, "#{@constructor.name}.on_input is not implemented!"

  signal: (callback) ->
    Module["Dialog_#{callback}"] bindings.engine

  _on_input: (char) ->
    (do @active) and (@accepts_input char) and (@on_input char)
