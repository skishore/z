class @DialogPage
  # These static variables should be overridden by subclasses.
  # @name will always be set to the class name by CoffeeScript.
  @height = '0px'

  accepts_input: (char) ->
    # Returns true if the dialog accepts the given input.
    assert false, "#{@constructor.name}.accepts_input is not implemented!"

  active: ->
    # Returns true if the dialog is still active.
    assert false, "#{@constructor.name}.active is not implemented!"

  get_data: ->
    # Returns the data needed to instantiate this dialog's Handlebars template.
    assert false, "#{@constructor.name}.get_data is not implemented!"

  on_input: (char) ->
    # Takes a single-character input and return true if the dialog changed.
    assert false, "#{@constructor.name}.on_input is not implemented!"


class @DialogManager
  @_current: null
  @_registry: {}

  @animate: =>
    $('.dialog > .scroller > *:last-child').css 'top', '150%'
    do @instantiate_random_dialog
    height = @_current.constructor.height
    move('.dialog > .scroller').set('margin-top', "-#{height}")
                               .duration('0.4s').end ->
      $('.dialog > .scroller').attr 'style', ''
      do $('.dialog > .scroller > *:first-child').empty
      $('.dialog > .scroller > *:last-child').css 'top', '50%'
      Session.set 'dialog.last', undefined

  @instantiate: (dialog_name) ->
    @_current = new @_registry[dialog_name]
    @redraw 'current'

  @instantiate_random_dialog: ->
    @instantiate _.sample _.keys @_registry

  @on_input: (char) ->
    if do @_current?.active and @_current.accepts_input char
      if @_current.on_input char
        @redraw (if do @_current.active then 'current' else 'last')


  @redraw: (target) ->
    Session.set "dialog.#{target}",
      name: @_current.constructor.template
      height: @_current.constructor.height
      data: do @_current.get_data

  @register: (dialog_subclass, name) ->
    assert name.length > 0, 'Tried to register empty name'
    assert name not of @_registry, "Duplicate dialog: #{name}"
    @_registry[name] = dialog_subclass


Template.dialog.helpers {
  last: ->
    data = Session.get 'dialog.last'
    if data?
      Meteor.setTimeout DialogManager.animate, 0
    data
  current: ->
    Session.get 'dialog.current'
}

Session.set 'dialog.last', undefined
Session.set 'dialog.current', undefined
