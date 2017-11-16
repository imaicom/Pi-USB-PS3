# Raspberry-Pi-USB-PS3

lsusb|grep Sony
# Bus 001 Device 008: ID 054c:0268 Sony Corp. Batoh Device / PlayStation 3 Controller

sudo vi /etc/udev/rules.d/90-joystick.rules
ATTRS{idVendor}=="054c", ATTRS{idProduct}=="0268", MODE="0660", GROUP="joystick"
# Vender "054c" = "SONY" , Product "0268" = "Sony PLAYSTATION(R)3 Controller"

sudo groupadd -r joystick
sudo usermod -a -G joystick pi
sudo udevadm control --reload-rules

reboot

Press the center of the PS button
Glowing only LED1

cat /dev/input/js0
apt-get install jstest-gtk
jstest-gtk
