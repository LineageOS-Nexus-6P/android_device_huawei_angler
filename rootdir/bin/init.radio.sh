#!/vendor/bin/sh

# QCRIL expects a writable copy of the stock emergency/carrier database.
# Preserve an initialized database, but recover the empty file left by an
# interrupted or pre-integration boot.
if [ -f /vendor/qcril.db ] && [ ! -s /data/misc/radio/qcril.db ]; then
    cp /vendor/qcril.db /data/misc/radio/qcril.db
fi

if [ -f /data/misc/radio/qcril.db ]; then
    chown radio:radio /data/misc/radio/qcril.db
    chmod 0660 /data/misc/radio/qcril.db
fi

echo 1 > /data/misc/radio/db_check_done
