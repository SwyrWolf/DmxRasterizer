## Version 1.0.2
New Feature:
Added toggle for viewing DMX Texture
Fix:
DMXRasterizer sometimes not closing & becoming a background process
Minor miscellaneous improvements

## Version 1.0.1
New Feature:
"localhost" is a valid ip input
Minor changes:
adjustments to network ipv4 parsing / error handling.
Fixed network loop breaking.

## Version 1.0.0
App Overhaul:
Removed the console / terminal
Added GUI
Added ability to change listening address during runtime
Added ability to change between 3 and 9 universe modes at runtime

## Version 0.8.2
App Cleanup:
Removed the need for a working directory, shaders are now embedded

## Version 0.8.1
Input prompt:
Without any --bind <ip> given, the program will prompt you for unicast
input "1" will use 127.0.0.1 and everything else will be 0.0.0.0

## Version 0.8.0
New Feature:
9 Channel Grid Node: `-9` or `--RGB`

## Version 0.7.0
New CLI Flags:
IP Binding: `--bind <ip>` Defaults all `0.0.0.0`
Version Display `-v` or `--version`
9 Channel Grid Node: `-9` or `--RGB` (Incomplete)