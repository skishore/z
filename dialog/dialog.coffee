class @DialogManager
  @get_page: ->
    if @_next? then @_next else @_current

  @on_input: (char) ->
    # TODO: Input should be passed to @_next if it is set. We probably want to
    # refactor the state to track current and last, not current and next.
    if @_current?.constructor.trap_input
      @_redraw 'current' if @_current._on_input char
      return true
    false

  @reset: ->
    @_current = null
    @_next = null
    Session.set 'dialog.active', false
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


if Meteor.isClient
  Template.dialog.helpers {
    display: -> if Session.get 'dialog.active' then 'block' else 'none'
    last: ->
      data = Session.get 'dialog.last'
      if data?
        Meteor.setTimeout DialogManager._animate, 0
      data
    current: ->
      Session.get 'dialog.current'
  }
