TIMEDATE=$(date +"%m-%d-%Y-%H-%M")
mv /home/luminari/mud/bin/circle /home/luminari/mud/bin/circle.$TIMEDATE
cp ../bin/circle /home/luminari/mud/bin
echo Copied binary to luminari live port
cp /home/luminari/mud/changelog /home/luminari/mud/lib/text/news
echo Moved changelog over to news file
