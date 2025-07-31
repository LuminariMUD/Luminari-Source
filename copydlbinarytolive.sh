TIMEDATE=$(date +"%m-%d-%Y-%H-%M")
mv /home/krynn/live/bin/circle /home/krynn/live/bin/circle.$TIMEDATE
cp ../bin/circle /home/krynn/live/bin
echo Copied binary to aod reawakening live port
cp /home/krynn/live/changelog /home/krynn/live/lib/text/news
echo Moved changelog over to news file

