#103003
Welcome to Ashenport~
2 g 100
~
wait 20
if !%actor.id% || %actor.room.vnum% != %self.vnum%
  halt
end
%send% %actor% Welcome to Ashenport!
wait 10
if !%actor.id% || %actor.room.vnum% != %self.vnum%
  halt
end
%send% %actor% To travel easily within the city, use the walkto system.
wait 10
if !%actor.id% || %actor.room.vnum% != %self.vnum%
  halt
end
%send% %actor% See HELP WALKTO for more information.
~
