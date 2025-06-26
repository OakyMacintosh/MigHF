-- Check MacOS version
if system version > 10.4 then
    display dialog "This script requires MacOS 10.3 or lower." buttons {"OK"} default button "OK"
    return
end if

-- Set script icon (current file)
set scriptIcon to (path to me as text)
set iconFile to (scriptIcon & "icon.icns") as text

if system version < 10.1 then
    on GUIScripting_status()
    -- check to see if assistive devices is enabled
        tell application "System Events"
            set UI_enabled to UI elements enabled
        end tell
        if UI_enabled is false then
            tell application "System Preferences"
                activate
                set current pane to pane id "com.apple.preference.universalaccess"
                display dialog "This script utilizes the built-in Graphic User Interface Scripting architecture of Mac OS x which is currently disabled." & return & return & "You can activate GUI Scripting by selecting the checkbox \"Enable access for assistive devices\" in the Universal Access preference pane." with icon 1 buttons {"Cancel"} default button 1
            end tell
        end if
    end GUIScripting_status
end if