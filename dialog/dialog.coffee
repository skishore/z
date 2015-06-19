class @DialogManager
  @on_input: (char) ->
    if @_current?.constructor.trap_input
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


_helpers = {
  display: -> if Session.get 'dialog.active' then 'block' else 'none'
  text: -> Session.get 'dialog.text'
  last: ->
    data = Session.get 'dialog.last'
    if data?
      Meteor.setTimeout DialogManager._animate, 0
    data
  current: ->
    Session.get 'dialog.current'
}


if Meteor.isClient
  do DialogManager.reset
  Template.dialog.helpers _helpers
