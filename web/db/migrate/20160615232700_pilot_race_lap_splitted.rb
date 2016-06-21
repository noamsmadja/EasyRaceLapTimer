class PilotRaceLapSplitted < ActiveRecord::Migration
  def change
    add_column :pilot_race_laps, :splitted, :boolean, default:false, index: true
    add_column :pilot_race_laps, :split_from, :integer, index: true
  end
end
