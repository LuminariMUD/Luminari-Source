TIMEDATE=$(date +"%m-%d-%Y-%H-%M")
mv /home/luminari/dev/bin/circle /home/luminari/dev/bin/circle.$TIMEDATE
cp ../bin/circle /home/luminari/dev/bin
echo "Copied binary to luminari dev port"
cp /home/luminari/dev/changelog /home/luminari/dev/lib/text/news
echo "Moved changelog over to news file"
echo "3 second delay before starting server..."
sleep 3
echo "Starting dev server on port 4101"
cd /home/luminari/dev && ./checkmud.sh &
echo ""
echo "To change to the dev directory, run:"
echo "cd /home/luminari/dev"
