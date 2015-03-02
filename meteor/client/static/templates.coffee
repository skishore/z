Template.log.helpers {
  dialog: -> Session.get 'dialog.active'
  display: ->
    if (Session.get 'dialog.active')
      return 'block'
    if (Session.get 'log')?.length == 0 then 'none' else 'block'
  lines: -> Session.get 'log'
}

Template.status.helpers {
  cur_health: -> Session.get 'status.cur_health'
  max_health: -> Session.get 'status.max_health'
}
