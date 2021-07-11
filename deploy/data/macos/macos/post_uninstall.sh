#!/bin/bash

APP_NAME=AmneziaVPN
PLIST_NAME=$APP_NAME.plist
LAUNCH_DAEMONS_PLIST_NAME=/Library/LaunchDaemons/$PLIST_NAME

if launchctl list "$APP_NAME-service" &> /dev/null; then
    launchctl unload $LAUNCH_DAEMONS_PLIST_NAME
    rm -f $LAUNCH_DAEMONS_PLIST_NAME
fi

rm -rf "$HOME/Library/Application Support/$APP_NAME"
rm -rf /var/log/$APP_NAME
rm -rf /Applications/$APP_NAME.app/Contents
