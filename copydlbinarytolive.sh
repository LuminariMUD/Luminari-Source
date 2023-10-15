TIMEDATE=$(date +"%m-%d-%Y-%H-%M")
mv /home/aod/reawakening/live/bin/circle /home/aod/reawakening/live/bin/circle.$TIMEDATE
cp ../bin/circle /home/aod/reawakening/live/bin
echo Copied binary to aod reawakening live port
cp /home/aod/reawakening/live/changelog /home/aod/reawakening/live/lib/text/news
echo Moved changelog over to news file

