if Meteor.isClient
  return

child_process = Npm.require 'child_process'

get_backup = ->
  "#{process.env.PWD}/backup"


Meteor.methods {
  backup: ->
    child_process.spawn 'mongodump', ['--port', '3001', '--out', do get_backup]
    true

  restore: ->
    base.collection.remove {}
    child_process.spawn 'mongorestore', ['--port', '3001', do get_backup]
    true

  wipe: ->
    base.collection.remove {}
}
