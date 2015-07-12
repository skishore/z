reset = ->
  # TODO(skishore): This reset function is a hack that manually destructs
  # all sorts of internal state of the stage. Each stage implementation
  # should provide a cleanup method instead.
  do $('.surface .pixi-text').remove
  do DialogManager.reset
  Session.set 'tilist.cursor', null
  Session.set 'tilist.hotkeys', null
  base.stage?.loop = ->
  base.stage?._graphics.draw = ->
  do base.stage?._graphics.context.removeChildren
  base.stage = new base.modes[Session.get 'menu.mode']

Template.menu.events {
  'click .button': (e) ->
    classes = ($(e.target).attr 'class').split ' '
    for cls in classes
      if cls != 'button' and cls of base.modes
        Session.set 'menu.mode', cls
}

Template.menu.helpers {
  buttons: ->
    result = []
    mode = Session.get 'menu.mode'
    if mode != 'main'
      result.push {class: 'main', text: 'Main'}
    if mode != 'tilist'
      result.push {class: 'tilist', text: 'Tilist'}
    result
}


Meteor.startup ->
  if Meteor.isClient
    Session.set 'menu.mode', 'main'
    Deps.autorun reset
