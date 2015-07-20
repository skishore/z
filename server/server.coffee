if Meteor.isClient
  return

child_process = Npm.require 'child_process'

get_backup_path = ->
  "#{process.env.PWD}/server/backup"


Meteor.methods {
  backup: ->
    path = do get_backup_path
    child_process.spawn 'mongodump', ['--port', '3001', '--out', path]
    true

  push: ->
    child_process.spawn 'server/push.sh', []
    true

  restore: ->
    path = do get_backup_path
    child_process.spawn 'mongorestore', ['--port', '3001', '--drop', path]
    true

  wipe: ->
    base.collection.remove {}
}
