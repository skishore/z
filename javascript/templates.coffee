Session.set 'log', undefined
Session.set 'status.cur_health', undefined
Session.set 'status.max_health', undefined

Template.labels.helpers {
  labels: -> Session.get 'labels'
}

Template.log.helpers {
  display: -> if (Session.get 'log')?.length > 0 then 'block' else 'none'
  lines: -> Session.get 'log'
}

Template.status.helpers {
  cur_health: -> Session.get 'status.cur_health'
  max_health: -> Session.get 'status.max_health'
}
