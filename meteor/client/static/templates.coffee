Template.log.helpers {
  display: -> if (Session.get 'log')?.length == 0 then 'none' else 'block'
  lines: -> Session.get 'log'
}

Template.status.helpers {
  cur_health: -> Session.get 'status.cur_health'
  max_health: -> Session.get 'status.max_health'
}
