subscription = {}


reset = ->
  if not do subscription.ready
    return
  # TODO(skishore): This reset function is a hack that manually destructs
  # all sorts of internal state of the stage. Each stage implementation
  # should provide a cleanup method instead.
  do DialogManager.reset
  Session.set 'pixi_text.elements', undefined
  Session.set 'tilist.cursor', undefined
  Session.set 'tilist.hotkeys', undefined
  base.stage?.loop = ->
  base.stage?._graphics.draw = ->
  for type, layer of base.stage?._graphics.layers
    do layer.removeChildren
  mode = Session.get 'menu.mode'
  Meteor.setTimeout -> base.stage = new base.modes[mode]


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
    if mode != 'gen'
      result.push {class: 'gen', text: 'Perlin'}
    if mode != 'tilist'
      result.push {class: 'tilist', text: 'Tilist'}
    result
}


Meteor.startup ->
  if Meteor.isClient
    subscription = Meteor.subscribe 'maps'
    Session.set 'menu.mode', base.starting_mode
    Deps.autorun reset
