from asyncio.windows_events import NULL
from ctypes import cast, POINTER
from pickle import NONE
from re import I
from tarfile import NUL
import time
from turtle import position
from numpy import interp
from comtypes import CLSCTX_ALL
from pycaw.pycaw import AudioUtilities, IAudioEndpointVolume

import serial

import ast

from pynput.keyboard import Key, Controller

#Master Volume Setup
master_volume_change_step = 140

keyboard = Controller()

#Volume Range/Increment
volumerange = [0,1]
volume_change_step = 70
volume_change_increment = (volumerange[0] - volumerange[1]) / volume_change_step
print(volume_change_increment)

global app_list_start_index
global app_list_end_index

app_list_start_index = 0
app_list_end_index = 3


while(True):
    try:
        s = serial.Serial('COM5', timeout=0.1, baudrate=4000000)
    except:
        print("Serial Error")
        time.sleep(2)
    else:
        app_list_start_index = 0
        app_list_end_index = 3
        while(1):
            try:
                line = s.readline().strip().decode('ascii')
            except:
                print("Serial Read Error")
                break
            else:
                if line != '':
                    app_list = []
                    ring_levels = ['0','0','0','0']
                    app_names = ["none","none","none"]
                    #Parse Line
                    try:
                        line_list = ast.literal_eval(line)
                    except:
                        print("Parsing Error")
                    else:
                        print(line_list)
                        #knob_switch = line_list[1]
                        master_knob_change = line_list[0]

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

                        #Master Volumes

                        devices = AudioUtilities.GetSpeakers()
                        interface = devices.Activate(
                            IAudioEndpointVolume._iid_, CLSCTX_ALL, None)
                        master_volume = cast(interface, POINTER(IAudioEndpointVolume))

                        master_volume_range = master_volume.GetVolumeRange()
                        master_volume_change_increment = (master_volume_range[0] - master_volume_range[1]) / master_volume_change_step

                        new_master_volume = master_volume.GetMasterVolumeLevel()  + (master_volume_change_increment * master_knob_change)

                        if(new_master_volume <= master_volume_range[1] and new_master_volume >= master_volume_range[0]):
                            master_volume.SetMasterVolumeLevel(new_master_volume, None)
                            print("Master: {}".format(new_master_volume))
                            ring_levels[0] = str(int(interp(new_master_volume,[master_volume_range[0],master_volume_range[1]],[0,16])))

                        #App Volumes

                        sessions = AudioUtilities.GetAllSessions()
                        
                        for session in sessions:
                            if session.Process:
                                #print(session.Process.name())
                                app_list.append(session)
                                print(str(session.Process.name()))
                        
                        line_list_index = 1
                        for i in range(app_list_start_index,app_list_end_index):
                            if i < len(app_list):
                                volume = app_list[i].SimpleAudioVolume

                                NewRange = (volumerange[0] - volumerange[1])  

                                new_volume = volume.GetMasterVolume()  + (volume_change_increment * line_list[line_list_index])
                                

                                if(new_volume <= volumerange[1] and new_volume >= volumerange[0]):
                                    volume.SetMasterVolume(new_volume, None)

                                ring_levels[line_list_index] = str(int(16 * new_volume))
                                app_names[line_list_index - 1] = str(app_list[i].Process.name())

                            line_list_index += 1

                        print(ring_levels)
                        print(app_names)
                        out_string = "Master,"
                        out_string += ring_levels[0]
                        out_string += ","
                        for i, app_name in enumerate(app_names):
                            out_string += app_name
                            out_string += ","
                            out_string += ring_levels[i + 1]
                            if i != 2:
                                out_string += ","
                        
                        out_string += ',16744448,16711935,16744448,16711935,9983,16711935,99838,16711935,9983,16711935,9983,16711935,,'

                        s.write(out_string.encode())
                        print(out_string)
    