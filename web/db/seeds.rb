# This file should contain all the record creation needed to seed the database with its default values.
# The data can then be loaded with the rake db:seed (or created alongside the db with db:setup).
#
# Examples:
#
#   cities = City.create([{ name: 'Chicago' }, { name: 'Copenhagen' }])
#   Mayor.create(name: 'Emanuel', city: cities.first)

t = User.new
t.email = "admin@easyracelaptimer.com"
t.password = "defaultpw"
t.password_confirmation = "defaultpw"
t.save

t.add_role(:admin)

ConfigValue::set_value("lap_min_lap_time_in_seconds","10")
ConfigValue::set_value("time_between_lap_track_requests_in_seconds","4")
ConfigValue::set_value("lap_max_lap_time_in_seconds","60")

Soundfile.where(name: 'sfx_lap_beep').first_or_create
Soundfile.where(name: 'sfx_start_race').first_or_create
Soundfile.where(name: 'sfx_fastet_lap').first_or_create
