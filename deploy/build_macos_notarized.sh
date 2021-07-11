#!/bin/bash
echo "Build script started ..."

set -o errexit -o nounset

# Hold on to current directory
PROJECT_DIR=$(pwd)
DEPLOY_DIR=$PROJECT_DIR/deploy

mkdir -p $DEPLOY_DIR/build
BUILD_DIR=$DEPLOY_DIR/build

echo "Project dir: ${PROJECT_DIR}" 
echo "Build dir: ${BUILD_DIR}"

APP_NAME=AmneziaVPN
APP_FILENAME=$APP_NAME.app
APP_DOMAIN=org.amneziavpn.package
PLIST_NAME=$APP_NAME.plist

OUT_APP_DIR=$BUILD_DIR/client
BUNDLE_DIR=$OUT_APP_DIR/$APP_FILENAME

DEPLOY_DATA_DIR=$PROJECT_DIR/deploy/data/macos
INSTALLER_DATA_DIR=$BUILD_DIR/installer/packages/$APP_DOMAIN/data
INSTALLER_BUNDLE_DIR=$BUILD_DIR/installer/$APP_FILENAME

PRO_FILE_PATH=$PROJECT_DIR/$APP_NAME.pro
QMAKE_STASH_FILE=$PROJECT_DIR/.qmake_stash
DMG_FILENAME=$PROJECT_DIR/${APP_NAME}_unsigned.dmg

# Seacrh Qt
if [ -z "${QT_VERSION+x}" ]; then
QT_VERSION=5.15.2;
QIF_VERSION=4.1
QT_BIN_DIR=$HOME/Qt/$QT_VERSION/clang_64/bin
QIF_BIN_DIR=$QT_BIN_DIR/../../../Tools/QtInstallerFramework/$QIF_VERSION/bin
fi

echo "Using Qt in $QT_BIN_DIR"
echo "Using QIF in $QIF_BIN_DIR"


# Checking env
$QT_BIN_DIR/qmake -v
make -v
clang -v

# Build App
echo "Building App..."
cd $BUILD_DIR

$QT_BIN_DIR/qmake $PROJECT_DIR/AmneziaVPN.pro 'CONFIG+=release CONFIG+=x86_64'
make -j `sysctl -n hw.ncpu`

# Build and run tests here

echo "____________________________________"
echo "............Deploy.................."
echo "____________________________________"

# Package 
echo "Packaging ..."

#cd $DEPLOY_DIR

$QT_BIN_DIR/macdeployqt $OUT_APP_DIR/$APP_FILENAME -always-overwrite
cp -av $BUILD_DIR/service/server/$APP_NAME-service.app/Contents/macOS/$APP_NAME-service $BUNDLE_DIR/Contents/macOS
cp -Rv $PROJECT_DIR/deploy/data/macos/* $BUNDLE_DIR/Contents/macOS
rm -f $BUNDLE_DIR/Contents/macOS/post_install.sh $BUNDLE_DIR/Contents/macOS/post_uninstall.sh

if [ "${MAC_CERT_PW+x}" ]; then

CERTIFICATE_P12=$DEPLOY_DIR/PrivacyTechAppleCertDeveloperId.p12
WWDRCA=$DEPLOY_DIR/WWDRCA.cer
KEYCHAIN=amnezia.build.keychain
TEMP_PASS=tmp_pass

security create-keychain -p $TEMP_PASS $KEYCHAIN || true
security default-keychain -s $KEYCHAIN
security unlock-keychain -p $TEMP_PASS $KEYCHAIN

security default-keychain
security list-keychains

security import $WWDRCA -k $KEYCHAIN -T /usr/bin/codesign || true
security import $CERTIFICATE_P12 -k $KEYCHAIN -P $MAC_CERT_PW -T /usr/bin/codesign || true

security set-key-partition-list -S apple-tool:,apple: -k $TEMP_PASS $KEYCHAIN
security find-identity -p codesigning

echo "Signing App bundle..."
/usr/bin/codesign --deep --force --verbose --timestamp -o runtime --sign "Developer ID Application: Privacy Technologies OU (X7UJ388FXK)" $BUNDLE_DIR
/usr/bin/codesign --verify -vvvv $BUNDLE_DIR || true
spctl -a -vvvv $BUNDLE_DIR || true

echo "Notatizing App bundle..."
/usr/bin/ditto -c -k --keepParent $BUNDLE_DIR $PROJECT_DIR/Bundle_to_notarize.zip
xcrun altool --notarize-app -f $PROJECT_DIR/Bundle_to_notarize.zip -t osx --primary-bundle-id "$APP_DOMAIN" -u "$APPLE_DEV_EMAIL" -p $APPLE_DEV_PASSWORD
rm $PROJECT_DIR/Bundle_to_notarize.zip
sleep 600
xcrun stapler staple $BUNDLE_DIR
xcrun stapler validate $BUNDLE_DIR
spctl -a -vvvv $BUNDLE_DIR || true

fi

echo "Packaging installer..."
mkdir -p $INSTALLER_DATA_DIR
cp -av $PROJECT_DIR/deploy/installer $BUILD_DIR
cp -av $DEPLOY_DATA_DIR/post_install.sh $INSTALLER_DATA_DIR/post_install.sh
cp -av $DEPLOY_DATA_DIR/post_uninstall.sh $INSTALLER_DATA_DIR/post_uninstall.sh
cp -av $DEPLOY_DATA_DIR/$PLIST_NAME $INSTALLER_DATA_DIR/$PLIST_NAME

chmod a+x $INSTALLER_DATA_DIR/post_install.sh $INSTALLER_DATA_DIR/post_uninstall.sh

cd $BUNDLE_DIR 
tar czf $INSTALLER_DATA_DIR/$APP_NAME.tar.gz ./

echo "Building installer..."
$QIF_BIN_DIR/binarycreator --offline-only -v -c $BUILD_DIR/installer/config/macos.xml -p $BUILD_DIR/installer/packages -f $INSTALLER_BUNDLE_DIR

if [ "${MAC_CERT_PW+x}" ]; then
echo "Signing installer bundle..."
security unlock-keychain -p $TEMP_PASS $KEYCHAIN
/usr/bin/codesign --deep --force --verbose --timestamp -o runtime --sign "Developer ID Application: Privacy Technologies OU (X7UJ388FXK)" $INSTALLER_BUNDLE_DIR
/usr/bin/codesign --verify -vvvv $INSTALLER_BUNDLE_DIR || true

echo "Notatizing installer bundle..."
/usr/bin/ditto -c -k --keepParent $INSTALLER_BUNDLE_DIR $PROJECT_DIR/Installer_bundle_to_notarize.zip
xcrun altool --notarize-app -f $PROJECT_DIR/Installer_bundle_to_notarize.zip -t osx --primary-bundle-id "$APP_DOMAIN" -u "$APPLE_DEV_EMAIL" -p $APPLE_DEV_PASSWORD
rm $PROJECT_DIR/Installer_bundle_to_notarize.zip
sleep 600
xcrun stapler staple $INSTALLER_BUNDLE_DIR
xcrun stapler validate $INSTALLER_BUNDLE_DIR
spctl -a -vvvv $INSTALLER_BUNDLE_DIR || true

fi

echo "Building DMG installer..."
hdiutil create -volname $APP_NAME -srcfolder $BUILD_DIR/installer/$APP_NAME.app -ov -format UDZO $DMG_FILENAME

if [ "${MAC_CERT_PW+x}" ]; then
echo "Signing DMG installer..."
security unlock-keychain -p $TEMP_PASS $KEYCHAIN
/usr/bin/codesign --deep --force --verbose --timestamp -o runtime --sign "Developer ID Application: Privacy Technologies OU (X7UJ388FXK)" $DMG_FILENAME
/usr/bin/codesign --verify -vvvv $DMG_FILENAME || true

echo "Notatizing DMG installer..."
xcrun altool --notarize-app -f $DMG_FILENAME -t osx --primary-bundle-id $APP_DOMAIN -u $APPLE_DEV_EMAIL -p $APPLE_DEV_PASSWORD
sleep 600
xcrun stapler staple $DMG_FILENAME
xcrun stapler validate $DMG_FILENAME

fi

echo "Finished, artifact is $DMG_FILENAME"

# restore keychain
security default-keychain -s login.keychain
