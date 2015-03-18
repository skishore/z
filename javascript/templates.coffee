Session.set 'log', undefined
Session.set 'status.cur_health', undefined
Session.set 'status.max_health', undefined

Template.log.helpers {
  display: ->
    if (Session.get 'dialog.last')? or (Session.get 'dialog.current')?
      return 'block'
    if (Session.get 'log')?.length == 0 then 'none' else 'block'
  lines: -> Session.get 'log'
}

Template.status.helpers {
  cur_health: -> Session.get 'status.cur_health'
  max_health: -> Session.get 'status.max_health'
}
