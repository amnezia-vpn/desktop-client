#!/bin/bash

APP_NAME=AmneziaVPN
LOG_FOLDER=/var/log/$APP_NAME
LOG_FILE="$LOG_FOLDER/post-uninstall.log"
APP_PATH=/opt/$APP_NAME

if ! test -f $LOG_FILE; then
	touch $LOG_FILE
fi

date >> $LOG_FILE
echo "Uninstall Script started" >> $LOG_FILE
sudo killall -9 $APP_NAME 2>> $LOG_FILE

ls /opt/AmneziaVPN/client/lib/* | while IFS=: read -r dir; do
	sudo unlink $dir  >> $LOG_FILE
done

if sudo systemctl is-active --quiet $APP_NAME; then
	sudo systemctl stop $APP_NAME >> $LOG_FILE
fi

if sudo systemctl is-enabled --quiet $APP_NAME; then
	sudo systemctl disable $APP_NAME >> $LOG_FILE
fi

if test -f /etc/systemd/system/$APP_NAME.service; then
	sudo rm -rf /etc/systemd/system/$APP_NAME.service >> $LOG_FILE
fi

if test -f /usr/bin/$APP_NAME; then
        sudo rm -rf /usr/sbin/$APP_NAME >> $LOG_FILE
fi

if test -f $APP_PATH; then
        sudo rm -rf $APP_PATH >> $LOG_FILE
fi

if test -f /usr/sbin/$APP_NAME; then
        sudo rm -rf /usr/sbin/$APP_NAME >> $LOG_FILE
fi

getent passwd {1000..6000} | while IFS=: read -r name password uid gid gecos home shell; do
	if test -f /home/$name/Desktop/$APP_NAME\ client.desktop; then
		sudo rm -rf /home/$name/Desktop/$APP_NAME\ client.desktop >> $LOG_FILE
	fi

        if test -f /home/$name/Desktop/$APP_NAME.desktop; then
                sudo rm -rf /home/$name/Desktop/$APP_NAME.desktop >> $LOG_FILE
        fi

	if test -f /home/$name/.config/$APP_NAME.ORG; then
		sudo rm -rf /home/$name/.config/$APP_NAME.ORG >> $LOG_FILE
	fi

	if test -f /home/$name/.local/share/$APP_NAME.ORG; then
		sudo rm -rf /home/$name/.local/share/$APP_NAME.ORG >> $LOG_FILE
	fi

	if test -f /home/$name/.local/share/$APP_NAME; then
                sudo rm -rf /home/$name/.local/share/$APP_NAME >> $LOG_FILE
        fi

	if test -f /home/$name/.icons/AmneziaVPN_Logo.png; then
		sudo rm -rf /home/$name/.icons/AmneziaVPN_Logo.png >> $LOG_FILE
	fi
done

date >> $LOG_FILE
echo "Service after uninstall status:" >> $LOG_FILE
sudo systemctl status $APP_NAME >> $LOG_FILE
date >> $LOG_FILE
echo "Script finished" >> $LOG_FILE
