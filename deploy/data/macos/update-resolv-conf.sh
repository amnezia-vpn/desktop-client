#!/bin/bash

# Mac name-resolution updater based on @cl's script here:
# https://blog.netnerds.net/2011/10/openvpn-update-client-dns-on-mac-os-x-using-from-the-command-line/
# Openvpn envar parsing taken from the script in debian's openvpn package.
# Smushed together and improved by @andrewgdotcom.

# Parses DHCP options from openvpn to update resolv.conf
# To use set as 'up' and 'down' script in your openvpn *.conf:
# up /etc/openvpn/update-resolv-conf
# down /etc/openvpn/update-resolv-conf

[ "$script_type" ] || exit 0
[ "$dev" ] || exit 0

PATH=$PATH:/usr/sbin/
NMSRVRS=()
SRCHS=()

# Get adapter list
IFS=$'\n' read -d '' -ra adapters < <(networksetup -listallnetworkservices |grep -v denotes) || true

split_into_parts()
{
        part1="$1"
        part2="$2"
        part3="$3"
}

update_all_dns()
{
        for adapter in "${adapters[@]}"
        do
        echo updating dns for $adapter
        # set dns server to the vpn dns server
        if [[ "${SRCHS[@]}" ]]; then
			networksetup -setsearchdomains "$adapter" "${SRCHS[@]}"
        fi
        if [[ "${NMSRVRS[@]}" ]]; then
			networksetup -setdnsservers "$adapter" "${NMSRVRS[@]}"
		fi
        done
}

clear_all_dns()
{
        for adapter in "${adapters[@]}"
        do
        echo updating dns for $adapter
        networksetup -setdnsservers "$adapter" empty
        networksetup -setsearchdomains "$adapter" empty
        done
}

case "$script_type" in
  up)
        for optionvarname in ${!foreign_option_*} ; do
                option="${!optionvarname}"
                echo "$option"
                split_into_parts $option
                if [ "$part1" = "dhcp-option" ] ; then
                        if [ "$part2" = "DNS" ] ; then
                                NMSRVRS=(${NMSRVRS[@]} $part3)
                        elif [ "$part2" = "DOMAIN" ] ; then
                                SRCHS=(${SRCHS[@]} $part3)
                        fi
                fi
        done
        update_all_dns
        ;;
  down)
        clear_all_dns
        ;;
esac
