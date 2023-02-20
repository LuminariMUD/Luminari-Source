TIMEDATE=$(date +"%m-%d-%Y-%H-%M")
mv /home/luminari/luminari/mud/bin/circle /home/luminari/luminari/mud/bin/circle.$TIMEDATE
cp ../bin/circle /home/luminari/luminari/mud/bin
echo Copied binary to luminari live port
cp /home/luminari/luminari/mud/changelog /home/luminari/luminari/mud/lib/text/news
echo Moved changelog over to news file

