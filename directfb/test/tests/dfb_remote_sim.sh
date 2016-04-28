#!/bin/bash

#========================================================

while [ 1 ]; do
	echo ""
	echo "Enter a key command:"
	echo "  u - send UP key"
	echo "  d - send DOWN key"
	echo "  l - send LEFT key"
	echo "  r - send RIGHT key"
	echo "  s - send SELECT (ENTER) key"
	echo "  t - send TAB key"
	echo "  p - send PRINT key"
	echo "  + - send POWER (KP PLUS) key"
	echo "  e - send ESCAPE key (quit app)"
	echo "  q - quit (this script)"
	read
	case "$REPLY" in
		u)
			echo "Sending UP."
			echo "u" > /tmp/dfb-key
			;;
		d)
			echo "Sending DOWN."
			echo "d" > /tmp/dfb-key
			;;
		l)
			echo "Sending LEFT."
			echo "l" > /tmp/dfb-key
			;;
		r)
			echo "Sending RIGHT."
			echo "r" > /tmp/dfb-key
			;;
		s)
			echo "Sending SELECT (ENTER)."
			echo "s" > /tmp/dfb-key
			;;
        t)
            echo "Sending TAB."
            echo "t" > /tmp/dfb-key
            ;;
        p)
            echo "Sending PRINT."
            echo "p" > /tmp/dfb-key
            ;;
        "+")
            echo "Sending POWER (KP PLUS)."
            echo "+" > /tmp/dfb-key
            ;;
		e)
			echo "Sending ESCAPE."
			echo "e" > /tmp/dfb-key
			;;
		q)
			echo "Quitting."
			exit 0
			;;
		*)
			echo "Unmatched input: $REPLY"
			;;
	esac
done

#========================================================

