count=1
while [ $count -le 5 ]; do
echo $count
count=$((count + 1))
done
echo "Finished."
sync
sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'
read c
read -s -n 1 n
read -s -n 1 t
