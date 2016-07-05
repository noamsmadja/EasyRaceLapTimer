class PilotRaceLap < ActiveRecord::Base
  acts_as_paranoid

  belongs_to :race_session
  belongs_to :pilot

  after_create :filter_fastest_lap
  after_create :filter_mark_latest

  def formated_lap_time
    return ((self.lap_time / 1000.0) / 60.0).round(4)
  end

  def filter_fastest_lap
    t_fastest_lap = false
	  t = RaceSession.find(self.race_session_id).pilot_race_laps_valid.order("lap_time ASC").first
  	if t.id == self.id
      t_fastest_lap = true
      SoundFileWorker.perform_async("sfx_fastet_lap")
      return
  	end

    # it was not the fastest lap in the race, but it might be a personal best time?
    if t_fastest_lap == false
      t = RaceSession.find(self.race_session_id).pilot_race_laps_valid.where(pilot_id: self.pilot_id).order("lap_time ASC").first
      if t.id == self.id
        SoundFileWorker.perform_async("sfx_personal_fastet_lap")
      end
    end
  end

  def filter_mark_latest
    RaceSession.find(self.race_session_id).pilot_race_laps.each do |l|
      l.update_attribute(:latest,false)
    end

    self.update_attribute(:latest,true)
  end

  def mark_invalidated
    self.update_attribute(:invalidated,true)
  end

  def undo_invalidated
    self.update_attribute(:invalidated,false)
  end
  
  def mark_splitted
    if self.splitted == false
     self.update_attribute(:lap_time,self.lap_time/2)
    end
    self.update_attribute(:splitted,true)
    @pilot = Pilot.find(self.pilot_id)
    prl = RaceSessionAdapter.new(self.race_session).track_lap_time(pilot.transponder_token,lap_time)
        PilotRaceLap.where(pilot_id: self.pilot_id).where(lap_time: self.lap_time).where(race_session_id: self.race_session_id).where.not(lap_num: self.lap_num).last.update_attribute(:split_from,self.id)
  end

  def undo_splitted
    if PilotRaceLap.where(split_from: self.id).count > 0
     PilotRaceLap.where(split_from: self.id).last.destroy
    end
    self.update_attribute(:lap_time,self.lap_time*2)
    self.update_attribute(:splitted,false)
  end

  def to_json
    t = Hash.new
    t[:id] = self.id
    t[:created_at] = self.created_at
    t[:created_at_timestamp] = self.created_at.to_i
    t[:lap_num] = self.lap_num
    t[:lap_time] = self.lap_time
    t[:pilot_id] = self.pilot_id
    t[:pilot] = Hash.new
    t[:pilot][:name] = self.pilot.name
    t[:pilot][:quad] = self.pilot.quad
    t[:pilot][:team] = self.pilot.team
    t[:race_session_id] = self.race_session_id
    t[:race_session] = Hash.new
    t[:race_session][:title] = self.race_session.title
    t[:race_session][:mode] = self.race_session.mode
    t[:invalidated] = self.invalidated;
    t[:splitted] = self.splitted;
    t[:split_from] = self.split_from;
    return t
  end
end
