#1
Training dummy pending damage cap~
0 u 100
~
* Mobile Damage is synchronous. Explicitly preserve hits at or below the cap.
if %damage% > 25
  return 25
else
  return %damage%
end
~
$~
