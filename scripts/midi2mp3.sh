#!/bin/bash

in="$1"
out=$(dirname "$in")/$(basename "$in" .mid).mp3
timidity "$in" -Ow -o - | ffmpeg -i - -acodec libmp3lame -ab 64k "$out"
