#!/bin/bash -ex


QT_BIN_DIR='/Users/admin/Qt/5.14.2/clang_64/bin'
QIF_BIN_DIR='/Users/admin/Qt/Tools/QtInstallerFramework/4.0/bin'

APP_NAME=AmneziaVPN
APP_FILENAME=$APP_NAME.app
APP_DOMAIN=org.amneziavpn.package
PLIST_NAME=$APP_NAME.plist

LAUNCH_DIR=$(pwd)
TOP_DIR=$LAUNCH_DIR/..
RELEASE_DIR=$TOP_DIR/../$APP_NAME-build
OUT_APP_DIR=$RELEASE_DIR/client/release
BUNDLE_DIR=$OUT_APP_DIR/$APP_FILENAME
DEPLOY_DATA_DIR=$LAUNCH_DIR/data/macos
INSTALLER_DATA_DIR=$RELEASE_DIR/installer/packages/$APP_DOMAIN/data

PRO_FILE_PATH=$TOP_DIR/$APP_NAME.pro
QMAKE_STASH_FILE=$TOP_DIR/.qmake_stash
TARGET_FILENAME=$TOP_DIR/$APP_NAME.dmg

cleanBuild() 
{
    rm -rf $RELEASE_DIR
    rm -rf $QMAKE_STASH_FILE
}


cleanBuild

cd $TOP_DIR
$QT_BIN_DIR/qmake $PRO_FILE_PATH 'CONFIG+=release CONFIG+=x86_64'
make -j `sysctl -n hw.ncpu`

$QT_BIN_DIR/macdeployqt $OUT_APP_DIR/$APP_FILENAME -always-overwrite
cp -av $RELEASE_DIR/server/release/$APP_NAME-service.app/Contents/macOS/$APP_NAME-service	$BUNDLE_DIR/Contents/macOS
cp -Rv $LAUNCH_DIR/data/macos/* 								$BUNDLE_DIR/Contents/macOS

mkdir -p $INSTALLER_DATA_DIR
cp -av $LAUNCH_DIR/installer 									$RELEASE_DIR
cp -av $DEPLOY_DATA_DIR/post_install.sh 							$INSTALLER_DATA_DIR/post_install.sh
cp -av $DEPLOY_DATA_DIR/post_uninstall.sh							$INSTALLER_DATA_DIR/post_uninstall.sh
cp -av $DEPLOY_DATA_DIR/$PLIST_NAME								$INSTALLER_DATA_DIR/$PLIST_NAME

rm -f $BUNDLE_DIR/Contents/macOS/post_install.sh $BUNDLE_DIR/Contents/macOS/post_uninstall.sh
chmod a+x $INSTALLER_DATA_DIR/post_install.sh $INSTALLER_DATA_DIR/post_uninstall.sh

cd $BUNDLE_DIR 
tar czf $INSTALLER_DATA_DIR/$APP_NAME.tar.gz ./

cd $RELEASE_DIR/installer
$QIF_BIN_DIR/binarycreator --offline-only -v -c config/macos.xml -p packages -f $APP_NAME
hdiutil create -volname $APP_NAME -srcfolder $APP_NAME.app -ov -format UDZO $TARGET_FILENAME

cleanBuild

cd $LAUNCH_DIR

echo "Finished, see $APP_NAME.dmg in '$TOP_DIR'"
