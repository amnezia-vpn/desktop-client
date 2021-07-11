#!/bin/bash

APP_NAME=AmneziaVPN
LOG_FOLDER=/var/log/$APP_NAME
LOG_FILE="$LOG_FOLDER/post-install.log"
APP_PATH=/opt/$APP_NAME

if ! test -f $LOG_FOLDER; then
        sudo mkdir $LOG_FOLDER
        echo "AmneziaVPN log dir created at /var/log/"
fi

if ! test -f $LOG_FILE; then
        touch $LOG_FILE
        echo "AmneziaVPN log file created at /var/log/AmneziaVPN/post-install.log"
fi

date > $LOG_FILE
echo "Script started" >> $LOG_FILE
sudo killall -9 $APP_NAME 2>> $LOG_FILE

if sudo systemctl is-active --quiet $APP_NAME; then
	sudo systemctl stop $APP_NAME >> $LOG_FILE
	sudo systemctl disable $APP_NAME >> $LOG_FILE
	sudo rm -rf /etc/systemd/system/$APP_NAME.service >> $LOG_FILE
fi

sudo cp $APP_PATH/service/$APP_NAME.service /etc/systemd/system/ >> $LOG_FILE

sudo ln -s $APP_PATH/client/lib/* /usr/lib/ >> $LOG_FILE

sudo systemctl start $APP_NAME >> $LOG_FILE
sudo systemctl enable $APP_NAME >> $LOG_FILE
sudo ln -s $APP_PATH/client/bin/$APP_NAME /usr/sbin/ >> $LOG_FILE


echo "user desktop creation loop started" >> $LOG_FILE
getent passwd {1000..6000} | while IFS=: read -r name password uid gid gecos home shell; do
        echo "name: $name"
        if ! test -f /home/$name/.icons; then
                mkdir /home/$name/.icons/ >> $LOG_FILE
        fi

        cp -f $APP_PATH/client/share/icons/AmneziaVPN_Logo.png /home/$name/.icons/ >> $LOG_FILE
        cp $APP_PATH/client/$APP_NAME.desktop /home/$name/Desktop/ >> $LOG_FILE

        sudo chown $name:$name /home/$name/.local/share/gvfs-metadata/home* >> $LOG_FILE
        sudo -u $name dbus-launch gio set /home/$name/Desktop/AmneziaVPN.desktop "metadata::trusted" yes >> $LOG_FILE
        sudo chown $name:$name /home/$name/Desktop/AmneziaVPN.desktop >> $LOG_FILE
done
echo "user desktop creation loop ended" >> $LOG_FILE

date >> $LOG_FILE
echo "Service status:" >> $LOG_FILE
sudo systemctl status $APP_NAME >> $LOG_FILE
date >> $LOG_FILE
echo "Script finished" >> $LOG_FILE
exit 0
