#!/bin/sh
# Do the cron snapshot
######################################################################

# Set our vars
PROGDIR="/usr/local/share/lpreserver"

# Source our functions
. /usr/local/share/pcbsd/scripts/functions.sh
. ${PROGDIR}/backend/functions.sh

DATASET="${1}"
KEEP="${2}"
snapStat=0

if [ -z "${DATASET}" ]; then
  exit_err "No dataset specified!"
fi

# Create the snapshot now with the "auto-" tag
echo_log "Creating snapshot on ${DATASET}"
mkZFSSnap "${DATASET}" "auto-"
if [ $? -ne 0 ] ; then
  echo_log "ERROR: Failed creating snapshot on ${DATASET}"
  queue_msg "Snapshot ERROR" "ERROR: Failed creating snapshot on ${DATASET} @ `date`\n\r`cat $CMDLOG`"
  snapStat=1
else
  queue_msg "Success creating snapshot on ${DATASET} @ `date`\n\r`cat $CMDLOG`"
fi

# Get our list of snaps
snaps=$(snaplist "${DATASET}")

# Reverse the list
for tmp in $snaps
do
   rSnaps="$tmp $rSnaps"
done

# Do any pruning
num=0
for snap in $rSnaps
do
   # Only remove snapshots which are auto-created, so we don't delete one the user
   # made specifically
   cur="`echo $snap | cut -d '-' -f 1`" 
   if [ "$cur" != "auto" ] ; then
     continue;
   fi

   num=`expr $num + 1`
   if [ $num -gt $KEEP ] ; then
      echo_log "Pruning old snapshot: $snap"
      rmZFSSnap "${DATASET}" "$snap"
      if [ $? -ne 0 ] ; then
        echo_log "ERROR: Failed pruning snapshot $snap on ${DATASET}"
        queue_msg "Snapshot ERROR" "ERROR: Failed pruning snapshot $snap on ${DATASET} @ `date`\n\r`cat $CMDLOG`"
        snapStat=1
      else
        queue_msg "Success pruning snapshot $snap on ${DATASET} @ `date`\n\r`cat $CMDLOG`"
      fi
    fi
done

# If we failed at any point, sent out a notice
if [ $snapStat -ne 0 ] ; then
   email_msg "FAILED - Automated Snapshot" "`echo_queue_msg`"
fi

# If we are successful and user wants all notifications, send out a message
if [ $snapStat -eq 0 -a "$EMAILMODE" = "ALL" ] ; then
   email_msg "Success - Automated Snapshot" "`echo_queue_msg`"
fi

# Check if we need to run a replication task for this dataset
${PROGDIR}/backend/runrep.sh ${DATASET} sync
