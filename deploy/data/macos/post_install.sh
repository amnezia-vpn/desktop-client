#!/bin/bash

APP_NAME=AmneziaVPN
PLIST_NAME=$APP_NAME.plist
LAUNCH_DAEMONS_PLIST_NAME=/Library/LaunchDaemons/$PLIST_NAME
LOG_FOLDER=/var/log/$APP_NAME
LOG_FILE="$LOG_FOLDER/post-install.log"
APP_PATH=/Applications/$APP_NAME.app

if launchctl list "$APP_NAME-service" &> /dev/null; then
    launchctl unload $LAUNCH_DAEMONS_PLIST_NAME
    rm -f $LAUNCH_DAEMONS_PLIST_NAME
fi

tar xzf	$APP_PATH/$APP_NAME.tar.gz -C $APP_PATH
rm -f	$APP_PATH/$APP_NAME.tar.gz

rm -rf	$LOG_FOLDER
mkdir -p $LOG_FOLDER

echo "`date` Script started" > $LOG_FILE

killall -9 $APP_NAME-service 2>> $LOG_FILE

mv -f $APP_PATH/$PLIST_NAME $LAUNCH_DAEMONS_PLIST_NAME 2>> $LOG_FILE
chown root:wheel $LAUNCH_DAEMONS_PLIST_NAME
launchctl load $LAUNCH_DAEMONS_PLIST_NAME

echo "`date` Service status: $?" >> $LOG_FILE
echo "`date` Script finished" >> $LOG_FILE

#rm -- "$0"
