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

  active: ->
    # Returns true if the dialog is still active.
    true

  on_input: (char) ->
    # Takes a single-character input and return true if the dialog changed.
    false

  get_data: ->
    # Returns the data needed to instantiate this dialog's Handlebars template.
    assert false, "#{@constructor.name}.get_data is not implemented!"

  # TODO(skishore): The following methods should be in a combat-dialog-specific
  # base class. We should also provide implementations of these methods for
  # two kinds of combat dialogs: subset selection  and ordered-subset selection.

  can_attack: (sid) ->
    # Returns true if the player can attack the given enemy.
    assert false, "#{@constructor.name}.can_attack is not implemented!"

  get_label: (sid) ->
    # Returns the label for the given enemy.
    #assert false, "#{@constructor.name}.get_label is not implemented!"
    return undefined

  get_num_enemies: ->
    # Returns the number of enemies needed to play this semantic game.
    assert false, "#{@constructor.name}.get_num_enemies is not implemented!"

  on_attack: (sid) ->
    # Handler method for when an enemy is killed. Returns a DialogAttackResult.
    assert false, "#{@constructor.name}.on_attack is not implemented!"

  _on_input: (char) ->
    (do @active) and (@on_input char)
