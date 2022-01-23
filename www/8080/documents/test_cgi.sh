#!/usr/bin/env bash

BASEDIR=$(dirname $0)
BODY="fname=FIRST_NAME&lname=LAST_NAME"

export REDIRECT_STATUS="200"
export REQUEST_METHOD="POST"
export CONTENT_TYPE="application/x-www-form-urlencoded"
export CONTENT_LENGTH=${#BODY}
export SCRIPT_FILENAME="$BASEDIR/hello_form.py"

exec echo "$BODY" | /usr/bin/python2 "$BASEDIR/hello_form.py"

