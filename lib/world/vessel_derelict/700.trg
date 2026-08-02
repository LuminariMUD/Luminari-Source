#70010
Blackwake bridge log discovery~
2 d 100
searchashlog~
if %self.name% /= Blackwake Derelict
  if !%actor.varexists(blackwake_log_found)%
    %send% %actor% Beneath the collapsed chart table, your hand closes around an ash-stained captain's log.
    %echoaround% %actor% %actor.name% pulls an ash-stained log from beneath the collapsed chart table.
    %load% obj 70010 %actor% inv
    set blackwake_log_found 1
    remote blackwake_log_found %actor.id%
  else
    %send% %actor% You search the collapsed chart table again, but its only intact log is already yours.
  end
else
  return 0
end
~
#70011
Blackwake quarters chart discovery~
2 d 100
searchashchart~
eval blackwake_bridge %self.south(room)%
if %blackwake_bridge.name% /= Blackwake Derelict
  if !%actor.varexists(blackwake_log_read)%
    %send% %actor% The sea chests are a jumble of ruined effects. You lack the clue needed to choose among them.
  elseif !%actor.varexists(blackwake_chart_found)%
    %send% %actor% The captain's lantern code identifies a false bottom. Inside waits the Blackwake's salt-stiff chart.
    %echoaround% %actor% %actor.name% lifts a salt-stiff chart from a false-bottomed sea chest.
    %load% obj 70011 %actor% inv
    set blackwake_chart_found 1
    remote blackwake_chart_found %actor.id%
  else
    %send% %actor% The false-bottomed sea chest is empty; you already recovered its chart.
  end
else
  return 0
end
~
#70012
Blackwake cargo salvage discovery~
2 d 100
recoverashsalvage~
eval blackwake_bridge %self.west(room)%
if %blackwake_bridge.name% /= Blackwake Derelict
  if !%actor.varexists(blackwake_chart_read)%
    %send% %actor% Buckled panels line the hold, and none offers an obvious place to begin.
  elseif !%actor.varexists(blackwake_salvage_recovered)%
    %send% %actor% Guided by the chart's tidefinder mark, you pry open a concealed panel and recover a corroded bronze gear.
    %echoaround% %actor% %actor.name% prises a bronze mechanism from a concealed cargo panel.
    %load% obj 70012 %actor% inv
    set blackwake_salvage_recovered 1
    remote blackwake_salvage_recovered %actor.id%
  else
    %send% %actor% The marked service panel gapes empty. Its tidefinder gear has already been recovered.
  end
else
  return 0
end
~
#70013
Blackwake captain log reading~
1 c 2
readashlog~
if !%actor.varexists(blackwake_log_read)%
  %send% %actor% You decipher the final entry: "Fire below. Quartermaster secured the reef soundings beneath berth three. Lantern code: low, high, low."
  %send% %actor% The entry points you toward the crew quarters. SEARCHASHCHART there to follow the captain's clue.
  set blackwake_log_read 1
  remote blackwake_log_read %actor.id%
else
  %send% %actor% The final entry still points to berth three in the crew quarters: SEARCHASHCHART.
end
~
#70014
Blackwake chart study~
1 c 2
studyashchart~
if !%actor.varexists(blackwake_chart_read)%
  %send% %actor% You flatten the salt-stiff chart. Soundings outline Blackwake reef, while a tidefinder symbol marks a concealed cargo-hold panel.
  %send% %actor% The notation is clear: enter the cargo hold and RECOVERASHSALVAGE from the marked panel.
  set blackwake_chart_read 1
  remote blackwake_chart_read %actor.id%
else
  %send% %actor% The chart still marks the concealed cargo panel: RECOVERASHSALVAGE.
end
~
$~
