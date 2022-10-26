import time
import logging
from numpy import interp
from comtypes import CLSCTX_ALL
from ctypes import cast, POINTER
from pycaw.pycaw import AudioUtilities, IAudioEndpointVolume
import serial
import ast
from pynput.keyboard import Key, Controller

#Com port of mixed
COM_PORT = 'COM5'

#Speed of Volume Change(Larger = Faster)
MASTER_VOLUME_CHANGE_STEP = 140
VOLUME_CHANGE_STEP = 70

#Range of app volume
VOLUME_RANGE = [0,1]

#LED colors
RING_MUTE_COLOR = 0xff0000
RING_COLOR = 0xfa3c07
MASTER_RING_COLOR = 0xa0e6fa
RING_ERROR_COLOR = 0x0000ff
BUTTON_COLOR = 0xa0e6fa
BUTTON_SECONDARY_COLOR = 0xfa3c07

#Display Text
NONE_DISPLAY_TEXT = "None"
MASTER_DISPLAY_TEXT = "Main"

#Setup master volume device to get master volume range and calculate master volume change step
devices = AudioUtilities.GetSpeakers()
interface = devices.Activate(
            IAudioEndpointVolume._iid_, CLSCTX_ALL, None)
master_volume = cast(interface, POINTER(IAudioEndpointVolume))
master_volume_range = master_volume.GetVolumeRange()
MASTER_VOLUME_CHANGE_INCREMENT = (master_volume_range[0] - master_volume_range[1]) / MASTER_VOLUME_CHANGE_STEP

#Calculate app volume change step
VOLUME_CHANGE_INCREMENT = (VOLUME_RANGE[0] - VOLUME_RANGE[1]) / VOLUME_CHANGE_STEP

#Keyboard object to send keypresses
keyboard = Controller()

#Starting position of app list
app_list_start_index = 0
app_list_end_index = 3

#Mute states of master volume and current app volumes
mutes = [False,False,False,False]

#Formates app name for display
def formatAppName(name):
    name = name.capitalize()
    name = name.split('.')[0]#removes the file extention
    return name

#Takes line from serial, processes the data, and writes data back to serial
def processLine(line_list):
    global app_list_start_index
    global app_list_end_index

    app_list = []#List of all active apps
    ring_levels = [0,0,0,0]#Ring levels for master and current apps

    app_names = [MASTER_DISPLAY_TEXT, #Reset on display text(for master and apps) to their defaults
                NONE_DISPLAY_TEXT,
                NONE_DISPLAY_TEXT,
                NONE_DISPLAY_TEXT]

    BUTTON_COLORs = [BUTTON_SECONDARY_COLOR, #Reset all button led colors to default
                    BUTTON_SECONDARY_COLOR,
                    BUTTON_COLOR,
                    BUTTON_COLOR,
                    BUTTON_COLOR,
                    BUTTON_SECONDARY_COLOR,
                    BUTTON_SECONDARY_COLOR,
                    BUTTON_SECONDARY_COLOR]

    RING_COLORs = [MASTER_RING_COLOR, #Reset all ring colors to default
                    RING_COLOR,
                    RING_COLOR,
                    RING_COLOR]

    print(line_list)#TODO Logging

    if line_list[4] == 1:
        if app_list_start_index > 0:
            app_list_start_index -= 1
            app_list_end_index -= 1
    if line_list[5] == 1:
        app_list_start_index += 1
        app_list_end_index += 1

    if line_list[11] == 1:
        keyboard.press(Key.media_next)
    elif line_list[11] == 0:
        keyboard.release(Key.media_next)

    if line_list[10] == 1:
        keyboard.press(Key.media_play_pause)
    elif line_list[10] == 0:
        keyboard.release(Key.media_play_pause)

    if line_list[9] == 1:
        keyboard.press(Key.media_previous)
    elif line_list[9] == 0:
        keyboard.release(Key.media_previous)

    #Master Volume
    master_knob_change = line_list[0]#Get volume change amount from serial input

    #Set up master volume device(done everytime incase the output device changes)
    devices = AudioUtilities.GetSpeakers()
    interface = devices.Activate(
        IAudioEndpointVolume._iid_, CLSCTX_ALL, None)
    master_volume = cast(interface, POINTER(IAudioEndpointVolume))

    try:
        #Set master volume to the change increment times the number of steps the encoder has moved since last update
        new_master_volume = master_volume.GetMasterVolumeLevel()  + (MASTER_VOLUME_CHANGE_INCREMENT * master_knob_change)
    except:#Catching some unknown error when setting/getting master volume/mute etc. (Might be pycaw API related?)
        print("MasterVolumeError")
        ring_levels[0] = 16
        RING_COLORs[0] = RING_ERROR_COLOR #Show error color on ring
        
    else:

        #If encoder button is pressed, toggle mute in mute list and set mute list value to master mute
        if bool(line_list[12]):
            mutes[0] = not mutes[0]
            master_volume.SetMute(mutes[0], None)
                
        #Make sure master volume stays within range
        if(new_master_volume > master_volume_range[1]):
            new_master_volume = master_volume_range[1]
        elif(new_master_volume < master_volume_range[0]):
            new_master_volume = master_volume_range[0]
        
        #Set master volume to new volume
        master_volume.SetMasterVolumeLevel(new_master_volume, None)
        #Set first ring level to master volume remapped to 0-16 int value
        ring_levels[0] = int(interp(new_master_volume,[master_volume_range[0],master_volume_range[1]],[0,16]))

        print("Master: {}".format(new_master_volume))#TODO Logging

        #If master is muted overide ring color/ to mute color and set level to full
        if master_volume.GetMute():
            ring_levels[0] = 16
            RING_COLORs[0] = RING_MUTE_COLOR                        

    #App Volumes

    #Get all session
    sessions = AudioUtilities.GetAllSessions()
    for session in sessions:
        if session.Process:#If session is valid, add it to the app list
            app_list.append(session)
            print(str(session.Process.name()))#TODO Logging

    #Index of the serial data list(skip master volume knob)
    line_list_index = 1
    #For all the currently displayed apps
    for i in range(app_list_start_index,app_list_end_index):
        if i < len(app_list):#If we are not passed the end of the app list

            #Get the app volume object
            volume = app_list[i].SimpleAudioVolume

            #Get the mute of the current app
            mutes[line_list_index] = volume.GetMute()

            #If the encoder at the current position is pressed, toggle the current app mute
            if bool(line_list[line_list_index + 12]):
                mutes[line_list_index] = not mutes[line_list_index]
            volume.SetMute(mutes[line_list_index], None)

            #Get new volume: change increment times the amount current encoder has moved since the last update
            new_volume = volume.GetMasterVolume()  + (VOLUME_CHANGE_INCREMENT * line_list[line_list_index])
            #Keep app volume in range
            if new_volume <= VOLUME_RANGE[0]:
                new_volume = VOLUME_RANGE[0]
            elif new_volume >= VOLUME_RANGE[1]:
                new_volume = VOLUME_RANGE[1]
            
            #Set app volume
            volume.SetMasterVolume(new_volume, None)
            #Set the current app ring level to a int remaped to 0-16
            ring_levels[line_list_index] = int(interp(new_volume,[VOLUME_RANGE[0],VOLUME_RANGE[1]],[0,16]))

            #Set the app name display text the reformated app name
            app_names[line_list_index] = formatAppName(str(app_list[i].Process.name()))

            #If the cuurent app is muted, set the ring color and ring level to full
            if volume.GetMute():
                ring_levels[line_list_index] = 16
                RING_COLORs[line_list_index] = RING_MUTE_COLOR  
        
        line_list_index += 1

    print(ring_levels)#TODO Logging
    print(app_names)
    
    #Write to serial

    #Construct output string
    #Format: Master Text,Master Ring Level,(App Name,App Ring Level)x3, (Ring Color)x4, (Button Color)x8,,
    out_string = ""
    for i, app_name in enumerate(app_names):
        out_string += "{},{},".format(app_name,ring_levels[i])
    for color in RING_COLORs:
        out_string += "{},".format(color)
    for color in BUTTON_COLORs:
        out_string += "{},".format(color)
    out_string += ','

    #Encode and write data to serial
    s.write(out_string.encode())
    print(out_string)#TODO Logging

#Main loop
while(True):
    try: #Try to start serial connection to specifed com port
        s = serial.Serial(COM_PORT, timeout=0.1, baudrate=4000000)
    except: #Unable to connect to serial
        print("Serial Error")
        #Wait a bit before try serial connection again
        time.sleep(2)
    else:
        #Set app list position to starting values
        app_list_start_index = 0
        app_list_end_index = 3
        #Main Serial Read Loop
        while(1):
            try:
                line = s.readline().strip().decode('ascii')#Get the line from serial
            except:#Serial connection was interupped for some reason
                print("Serial Read Error")
                break #Go back to trying to connect to serial
            else:
                if line != '': #Only process non-empty lines
                    try:
                        #Read the line from serial as python syntax. This will result in a list
                        in_line_list = ast.literal_eval(line)
                    except:
                        print("Parsing Error")#If there is a syntax error, the serial data was not formtted correctly
                    else:
                        #Preform the main prosessing
                        processLine(in_line_list)
                        