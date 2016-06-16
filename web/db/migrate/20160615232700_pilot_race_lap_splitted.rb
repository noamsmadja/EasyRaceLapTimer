class PilotRaceLapSplitted < ActiveRecord::Migration
  def change
    add_column :pilot_race_laps, :splitted, :boolean, default:false, index: true
  end
end
