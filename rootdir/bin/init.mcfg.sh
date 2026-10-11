#!/vendor/bin/sh

# QCRIL expects the stock modem carrier configuration on writable data.
rm -rf /data/misc/radio/modem_config
mkdir -m 770 /data/misc/radio/modem_config
cp -r /firmware/image/modem_pr/mcfg/configs/. /data/misc/radio/modem_config
echo 1 > /data/misc/radio/copy_complete
