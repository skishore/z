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

  signal: (callback) ->
    Module["Dialog_#{callback}"] bindings.engine

  _on_input: (char) ->
    (do @active) and (@accepts_input char) and (@on_input char)


class @DialogManager
  @on_input: (char) ->
    if @_next?
      return true
    if @_current?
      @_redraw 'current' if @_current._on_input char
      return true
    false

  @reset: ->
    @_current = null
    @_next = null
    Session.set 'dialog.active', false
    Session.set 'dialog.text', undefined
    Session.set 'dialog.last', undefined
    Session.set 'dialog.current', undefined

  @set_page: (page) ->
    if @_current?
      @_next = page
      @_redraw 'last'
    else
      @_current = page
      @_redraw 'current'
      Session.set 'dialog.active', true

  @set_text: (text) ->
    Session.set 'dialog.text', text

  @_animate: =>
    $('.dialog > .scroller > *:last-child').css 'top', '150%'
    @_current = @_next
    @_next = null
    @_redraw 'current'
    height = @_current.constructor.height
    move('.dialog > .scroller').set('margin-top', "-#{height}")
                               .duration('0.4s').end ->
      $('.dialog > .scroller').attr 'style', ''
      do $('.dialog > .scroller > *:first-child').empty
      $('.dialog > .scroller > *:last-child').css 'top', '50%'
      Session.set 'dialog.last', undefined

  @_redraw: (target) ->
    Session.set "dialog.#{target}",
      name: @_current.constructor.template
      height: @_current.constructor.height
      data: do @_current.get_data


Template.dialog.helpers {
  text: -> Session.get 'dialog.text'
  last: ->
    data = Session.get 'dialog.last'
    if data?
      Meteor.setTimeout DialogManager._animate, 0
    data
  current: ->
    Session.get 'dialog.current'
}


do DialogManager.reset
